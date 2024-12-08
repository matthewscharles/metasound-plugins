#include "MetasoundBranches/Public/MetasoundStereoCrossRouterNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "Math/UnrealMathUtility.h"          // For FMath functions like Clamp, Sin, Cos

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_CrossRouterNode"

namespace Metasound
{
    namespace CrossRouterNodeNames
    {
        METASOUND_PARAM(InputLeftSignal, "In L", "Left channel of the stereo input signal.");
        METASOUND_PARAM(InputRightSignal, "In R", "Right channel of the stereo input signal.");
        METASOUND_PARAM(InputCrossfade, "Crossfade", "Crossfade control to route the input to Outputs 1 or 2 (0.0 to 1.0).");

        METASOUND_PARAM(OutputLeft1, "Out1 L", "Left channel of the first stereo output.");
        METASOUND_PARAM(OutputRight1, "Out1 R", "Right channel of the first stereo output.");
        METASOUND_PARAM(OutputLeft2, "Out2 L", "Left channel of the second stereo output.");
        METASOUND_PARAM(OutputRight2, "Out2 R", "Right channel of the second stereo output.");
    }

    class FCrossRouterOperator : public TExecutableOperator<FCrossRouterOperator>
    {
    public:
        FCrossRouterOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InLeftSignal,
            const FAudioBufferReadRef& InRightSignal,
            const FFloatReadRef& InCrossfade)
            : InputLeftSignal(InLeftSignal)
            , InputRightSignal(InRightSignal)
            , InputCrossfade(InCrossfade)
            , OutputLeft1(FAudioBufferWriteRef::CreateNew(InSettings))
            , OutputRight1(FAudioBufferWriteRef::CreateNew(InSettings))
            , OutputLeft2(FAudioBufferWriteRef::CreateNew(InSettings))
            , OutputRight2(FAudioBufferWriteRef::CreateNew(InSettings))
        {
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace CrossRouterNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputLeftSignal)),
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputRightSignal)),
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputCrossfade), 0.5f) // Default is centered crossfade
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputLeft1)),
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputRight1)),
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputLeft2)),
                    TOutputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputRight2))
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

                Metadata.ClassName = { StandardNodes::Namespace, TEXT("Stereo CrossRouter"), StandardNodes::AudioVariant };
                Metadata.MajorVersion = 1;
                Metadata.MinorVersion = 0;
                Metadata.DisplayName = METASOUND_LOCTEXT("CrossRouterNodeDisplayName", "Stereo CrossRouter");
                Metadata.Description = METASOUND_LOCTEXT("CrossRouterNodeDesc", "Routes a stereo input to two stereo outputs with equal-power crossfading.");
                Metadata.Author = "Charles Matthews";
                Metadata.PromptIfMissing = PluginNodeMissingPrompt;
                Metadata.DefaultInterface = NodeInterface;
                Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Custom", "Branches") };
                Metadata.Keywords = TArray<FText>();

                return Metadata;
            };

            static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
            return Metadata;
        }

        virtual FDataReferenceCollection GetInputs() const override
        {
            using namespace CrossRouterNodeNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputLeftSignal), InputLeftSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputRightSignal), InputRightSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputCrossfade), InputCrossfade);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace CrossRouterNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputLeft1), OutputLeft1);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputRight1), OutputRight1);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputLeft2), OutputLeft2);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputRight2), OutputRight2);

            return OutputDataReferences;
        }
        
        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace CrossRouterNodeNames;

            const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<FAudioBuffer> InputLeftSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputLeftSignal), InParams.OperatorSettings);
            TDataReadReference<FAudioBuffer> InputRightSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputRightSignal), InParams.OperatorSettings);
            TDataReadReference<float> InputCrossfade = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InputCrossfade), InParams.OperatorSettings);

            return MakeUnique<FCrossRouterOperator>(InParams.OperatorSettings, InputLeftSignal, InputRightSignal, InputCrossfade);
        }
        
        void Execute()
        {
            int32 NumFrames = InputLeftSignal->Num();

            const float* LeftData = InputLeftSignal->GetData();
            const float* RightData = InputRightSignal->GetData();
            
            float* OutLeft1 = OutputLeft1->GetData();
            float* OutRight1 = OutputRight1->GetData();
            float* OutLeft2 = OutputLeft2->GetData();
            float* OutRight2 = OutputRight2->GetData();

            float CrossfadeFactor = FMath::Clamp(*InputCrossfade, 0.0f, 1.0f);

            float Gain1 = FMath::Cos(CrossfadeFactor * HALF_PI);
            float Gain2 = FMath::Sin(CrossfadeFactor * HALF_PI);

            for (int32 i = 0; i < NumFrames; ++i)
            {
                OutLeft1[i] = LeftData[i] * Gain1;
                OutRight1[i] = RightData[i] * Gain1;
                OutLeft2[i] = LeftData[i] * Gain2;
                OutRight2[i] = RightData[i] * Gain2;
            }
        }

    private:

        // Inputs
        FAudioBufferReadRef InputLeftSignal;
        FAudioBufferReadRef InputRightSignal;
        FFloatReadRef InputCrossfade;

        // Outputs
        FAudioBufferWriteRef OutputLeft1;
        FAudioBufferWriteRef OutputRight1;
        FAudioBufferWriteRef OutputLeft2;
        FAudioBufferWriteRef OutputRight2;
    };

    class FCrossRouterNode : public FNodeFacade
    {
    public:
        FCrossRouterNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FCrossRouterOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FCrossRouterNode);
}

#undef LOCTEXT_NAMESPACE