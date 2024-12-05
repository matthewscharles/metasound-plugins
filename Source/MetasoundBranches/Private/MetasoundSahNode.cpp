#include "MetasoundBranches/Public/MetasoundSahNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_SahNode"

namespace Metasound
{
    namespace SahNodeNames
    {
        METASOUND_PARAM(InputSignal, "Signal", "Input signal to sample.");
        METASOUND_PARAM(InputTrigger, "Trigger", "Trigger signal.");
        METASOUND_PARAM(InputThreshold, "Threshold", "Threshold for trigger.");

        METASOUND_PARAM(OutputSignal, "Output", "Sampled output signal.");
    }

    class FSahOperator : public TExecutableOperator<FSahOperator>
    {
    public:
        FSahOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InSignal,
            const FAudioBufferReadRef& InTrigger,
            const FFloatReadRef& InThreshold)
            : InputSignal(InSignal)
            , InputTrigger(InTrigger)
            , InputThreshold(InThreshold)
            , OutputSignal(FAudioBufferWriteRef::CreateNew(InSettings))
            , SampledValue(0.0f)
            , PreviousTriggerValue(0.0f)
        {
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace SahNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSignal)),
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTrigger)),
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputThreshold))
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal))
                )
            );

            return Interface;
        }

        static const FNodeClassMetadata& GetNodeInfo()
        {
            auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
                {
                    FVertexInterface NodeInterface = DeclareVertexInterface();

                    FNodeClassMetadata Metadata;

                    Metadata.ClassName = { StandardNodes::Namespace, TEXT("SaH"), StandardNodes::AudioVariant };
                    Metadata.MajorVersion = 1;
                    Metadata.MinorVersion = 0;
                    Metadata.DisplayName = METASOUND_LOCTEXT("SahNodeDisplayName", "Sample And Hold (audio trigger)");
                    Metadata.Description = METASOUND_LOCTEXT("SahNodeDesc", "Samples an input signal when a trigger crosses an audio threshold, and holds it until the next trigger.");
                    Metadata.Author = "Charles Matthews";
                    Metadata.PromptIfMissing = PluginNodeMissingPrompt;
                    Metadata.DefaultInterface = DeclareVertexInterface();
                    Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Custom", "Branches") };
                    Metadata.Keywords = TArray<FText>(); // Keywords for searching

                    return Metadata;
                };

            static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
            return Metadata;
        }

        virtual FDataReferenceCollection GetInputs() const override
        {
            using namespace SahNodeNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputSignal), InputSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputTrigger), InputTrigger);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputThreshold), InputThreshold);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace SahNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal), OutputSignal);

            return OutputDataReferences;
        }

        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace SahNodeNames;

            const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<FAudioBuffer> InputSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputSignal), InParams.OperatorSettings);
            TDataReadReference<FAudioBuffer> InputTrigger = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputTrigger), InParams.OperatorSettings);
            TDataReadReference<float> InputThreshold = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InputThreshold), InParams.OperatorSettings);

            return MakeUnique<FSahOperator>(InParams.OperatorSettings, InputSignal, InputTrigger, InputThreshold);
        }

        void Execute()
        {
            int32 NumFrames = InputSignal->Num();

            const float* SignalData = InputSignal->GetData();
            const float* TriggerData = InputTrigger->GetData();
            float* OutputData = OutputSignal->GetData();

            float Threshold = *InputThreshold;

            for (int32 i = 0; i < NumFrames; ++i)
            {
                float CurrentTriggerValue = TriggerData[i];

                // Detect rising edge
                if (PreviousTriggerValue < Threshold && CurrentTriggerValue >= Threshold)
                {
                    // Sample the input signal
                    SampledValue = SignalData[i];
                }

                // Output the sampled value
                OutputData[i] = SampledValue;

                // Update previous trigger value
                PreviousTriggerValue = CurrentTriggerValue;
            }
        }

    private:

        // Inputs
        FAudioBufferReadRef InputSignal;
        FAudioBufferReadRef InputTrigger;
        FFloatReadRef InputThreshold;

        // Outputs
        FAudioBufferWriteRef OutputSignal;

        // Internal variables
        float SampledValue;
        float PreviousTriggerValue;
    };

    class FSahNode : public FNodeFacade
    {
    public:
        FSahNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FSahOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FSahNode);
}

#undef LOCTEXT_NAMESPACE