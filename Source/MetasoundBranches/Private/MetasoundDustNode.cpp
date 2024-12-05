#include "MetasoundBranches/Public/MetasoundDustNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "Math/UnrealMathUtility.h"          // For FMath functions
#include "Misc/DateTime.h"                   // For FDateTime::UtcNow()

// Required for ensuring the node is supported by all languages in engine. Must be unique per MetaSound.
#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_DustNode"

namespace Metasound
{
    // Vertex Names - define the node's inputs and outputs here
    namespace DustNodeNames
    {
        METASOUND_PARAM(InputDensity, "Modulation", "Density control signal.");
        METASOUND_PARAM(InputDensityOffset, "Density", "Probability of impulse generation.");
        METASOUND_PARAM(InputEnabled, "Enabled", "Enable or disable generation.");
        METASOUND_PARAM(InputBiPolar, "Bi-Polar", "Toggle between bipolar and unipolar impulse output.");
        METASOUND_PARAM(OutputImpulse, "Impulse Output", "Generated impulse output.");
    }

    // Operator Class - defines the way the node is described, created and executed
    class FDustOperator : public TExecutableOperator<FDustOperator>
    {
    public:
        // Constructor
        FDustOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InDensity,
            const FFloatReadRef& InDensityOffset,
            const FBoolReadRef& InEnabled,
            const FBoolReadRef& InBiPolar)
            : InputDensity(InDensity)
            , InputDensityOffset(InDensityOffset)
            , InputEnabled(InEnabled)
            , InputBiPolar(InBiPolar)
            , OutputImpulse(FAudioBufferWriteRef::CreateNew(InSettings))
            , RNGStream(InitialSeed())
            , SignalIsPositive(true)
        {
        }

        // Helper function for constructing vertex interface
        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace DustNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputEnabled), true),
                    TInputDataVertexModel<bool>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputBiPolar), true),
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDensityOffset), 0.0f),
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDensity))
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputImpulse))
                )
            );

            return Interface;
        }

        // Retrieves necessary metadata about the node
        static const FNodeClassMetadata& GetNodeInfo()
        {
            auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
                {
                    FVertexInterface NodeInterface = DeclareVertexInterface();

                    FNodeClassMetadata Metadata;

                    Metadata.ClassName = { StandardNodes::Namespace, TEXT("Dust"), StandardNodes::AudioVariant };
                    Metadata.MajorVersion = 1;
                    Metadata.MinorVersion = 0;
                    Metadata.DisplayName = METASOUND_LOCTEXT("DustNodeDisplayName", "Dust");
                    Metadata.Description = METASOUND_LOCTEXT("DustNodeDesc", "Generates randomly timed impulses with audio-rate modulation.");
                    Metadata.Author = "Charles Matthews";
                    Metadata.PromptIfMissing = PluginNodeMissingPrompt;
                    Metadata.DefaultInterface = DeclareVertexInterface();
                    Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Custom", "Branches") };
                    Metadata.Keywords = TArray<FText>(); // Add relevant keywords if necessary

                    return Metadata;
                };

            static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
            return Metadata;
        }

        // Allows MetaSound graph to interact with the node's inputs
        virtual FDataReferenceCollection GetInputs() const override
        {
            using namespace DustNodeNames;
            FDataReferenceCollection Inputs;
            Inputs.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputDensity), InputDensity);
            Inputs.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputDensityOffset), InputDensityOffset);
            Inputs.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputEnabled), InputEnabled);
            Inputs.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputBiPolar), InputBiPolar);
            return Inputs;
        }

        // Allows MetaSound graph to interact with the node's outputs
        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace DustNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputImpulse), OutputImpulse);

            return OutputDataReferences;
        }

        // Used to instantiate a new runtime instance of the node
        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace DustNodeNames;

            const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<FAudioBuffer> InputDensity = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputDensity), InParams.OperatorSettings);
            TDataReadReference<float> InputDensityOffset = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InputDensityOffset), InParams.OperatorSettings);
            TDataReadReference<bool> InputEnabled = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<bool>(InputInterface, METASOUND_GET_PARAM_NAME(InputEnabled), InParams.OperatorSettings);
            TDataReadReference<bool> InputBiPolar = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<bool>(InputInterface, METASOUND_GET_PARAM_NAME(InputBiPolar), InParams.OperatorSettings);

            return MakeUnique<FDustOperator>(InParams.OperatorSettings, InputDensity, InputDensityOffset, InputEnabled, InputBiPolar);
        }

        // Primary node functionality
        void Execute()
        {
        const float* DensityData = InputDensity->GetData();
        float* OutputDataPtr = OutputImpulse->GetData();
        int32 NumFrames = InputDensity->Num();
        float InputDensityOffsetValue = *InputDensityOffset;
        bool bEnabled = *InputEnabled;
        bool bBiPolar = *InputBiPolar;

        for (int32 i = 0; i < NumFrames; ++i)
        {
            if (bEnabled)
            {
                float Density = DensityData[i];
                float AbsDensity = FMath::Abs(Density) + InputDensityOffsetValue;
                float Threshold = 1.0f - AbsDensity * 0.0009f;

                float RandomValue = RNGStream.GetFraction();

                if (RandomValue > Threshold)
                {
                    if (bBiPolar)
                    {
                        OutputDataPtr[i] = SignalIsPositive ? 1.0f : -1.0f;
                        SignalIsPositive = !SignalIsPositive; 
                    }
                    else
                    {
                        OutputDataPtr[i] = 1.0f;
                    }
                }
                else
                {
                    OutputDataPtr[i] = 0.0f;
                }
            }
            else
            {
                OutputDataPtr[i] = 0.0f; // Output zero when disabled
            }
        }
    }

    private:

        // Inputs
        FAudioBufferReadRef InputDensity;
		FFloatReadRef InputDensityOffset;
        FBoolReadRef InputEnabled;
        FBoolReadRef InputBiPolar;

        // Outputs
        FAudioBufferWriteRef OutputImpulse;

        // Random number generator
        FRandomStream RNGStream;
        
        // Toggle flag for polarity
        bool SignalIsPositive;

        // Generate an initial seed for FRandomStream
        static int32 InitialSeed()
        {
            return FDateTime::UtcNow().GetTicks();
        }
    };

    // Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
    class FDustNode : public FNodeFacade
    {
    public:
        FDustNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FDustOperator>())
        {
        }
    };

    // Register node
    METASOUND_REGISTER_NODE(FDustNode);
}

#undef LOCTEXT_NAMESPACE