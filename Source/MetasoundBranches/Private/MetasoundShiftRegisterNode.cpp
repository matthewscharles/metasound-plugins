#include "MetasoundBranches/Public/MetasoundShiftRegisterNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_ShiftRegisterNode"

namespace Metasound
{
    namespace ShiftRegisterNodeNames
    {
        METASOUND_PARAM(InputSignal, "Signal", "Input float to the shift register.");
        METASOUND_PARAM(InputTrigger, "Trigger", "Trigger.");

        METASOUND_PARAM(OutputSignal1, "Stage 1", "Shifted output at stage 1.");
        METASOUND_PARAM(OutputSignal2, "Stage 2", "Shifted output at stage 2.");
        METASOUND_PARAM(OutputSignal3, "Stage 3", "Shifted output at stage 3.");
        METASOUND_PARAM(OutputSignal4, "Stage 4", "Shifted output at stage 4.");
        METASOUND_PARAM(OutputSignal5, "Stage 5", "Shifted output at stage 5.");
        METASOUND_PARAM(OutputSignal6, "Stage 6", "Shifted output at stage 6.");
        METASOUND_PARAM(OutputSignal7, "Stage 7", "Shifted output at stage 7.");
        METASOUND_PARAM(OutputSignal8, "Stage 8", "Shifted output at stage 8.");
    }

    class FShiftRegisterOperator : public TExecutableOperator<FShiftRegisterOperator>
    {
    public:
        FShiftRegisterOperator(
            const FOperatorSettings& InSettings,
            const FFloatReadRef& InInputSignal,
            const FTriggerReadRef& InInputTrigger)
            : InputSignal(InInputSignal)
            , InputTrigger(InInputTrigger)
            , OutputSignal1(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal2(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal3(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal4(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal5(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal6(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal7(FFloatWriteRef::CreateNew(0.0f))
            , OutputSignal8(FFloatWriteRef::CreateNew(0.0f))
            , ShiftedValue1(0.0f)
            , ShiftedValue2(0.0f)
            , ShiftedValue3(0.0f)
            , ShiftedValue4(0.0f)
            , ShiftedValue5(0.0f)
            , ShiftedValue6(0.0f)
            , ShiftedValue7(0.0f)
            , ShiftedValue8(0.0f)
        {
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace ShiftRegisterNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSignal)),
                    TInputDataVertexModel<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputTrigger))
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal1)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal2)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal3)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal4)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal5)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal6)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal7)),
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputSignal8))
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

                    Metadata.ClassName = { StandardNodes::Namespace, TEXT("Shift Register"), TEXT("Float") };
                    Metadata.MajorVersion = 1;
                    Metadata.MinorVersion = 0;
                    Metadata.DisplayName = METASOUND_LOCTEXT("ShiftRegisterNodeDisplayName", "Shift Register");
                    Metadata.Description = METASOUND_LOCTEXT("ShiftRegisterNodeDesc", "Shift register node with eight stages.");
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
            using namespace ShiftRegisterNodeNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputSignal), InputSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputTrigger), InputTrigger);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace ShiftRegisterNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal1), OutputSignal1);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal2), OutputSignal2);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal3), OutputSignal3);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal4), OutputSignal4);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal5), OutputSignal5);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal6), OutputSignal6);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal7), OutputSignal7);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal8), OutputSignal8);

            return OutputDataReferences;
        }

        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace ShiftRegisterNodeNames;

            const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<float> InputSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InputSignal), InParams.OperatorSettings);
            TDataReadReference<FTrigger> InputTrigger = InputCollection.GetDataReadReferenceOrConstruct<FTrigger>(METASOUND_GET_PARAM_NAME(InputTrigger), InParams.OperatorSettings);

            return MakeUnique<FShiftRegisterOperator>(
                InParams.OperatorSettings,
                InputSignal,
                InputTrigger
            );
        }

        void Execute()
        {
            InputTrigger->ExecuteBlock(
                // Pre-trigger lambda (called before any triggers in the block)
                [](int32 StartFrame, int32 EndFrame)
                {
                    // No action needed before triggers
                },

                // On-trigger lambda (called for each trigger event)
                [&](int32 StartFrame, int32 EndFrame)
                {
                    ShiftedValue8 = ShiftedValue7;
                    ShiftedValue7 = ShiftedValue6;
                    ShiftedValue6 = ShiftedValue5;
                    ShiftedValue5 = ShiftedValue4;
                    ShiftedValue4 = ShiftedValue3;
                    ShiftedValue3 = ShiftedValue2;
                    ShiftedValue2 = ShiftedValue1;
                    ShiftedValue1 = *InputSignal;
                }
            );

            // Update outputs
            *OutputSignal1 = ShiftedValue1;
            *OutputSignal2 = ShiftedValue2;
            *OutputSignal3 = ShiftedValue3;
            *OutputSignal4 = ShiftedValue4;
            *OutputSignal5 = ShiftedValue5;
            *OutputSignal6 = ShiftedValue6;
            *OutputSignal7 = ShiftedValue7;
            *OutputSignal8 = ShiftedValue8;
        }

    private:
        FFloatReadRef InputSignal;
        FTriggerReadRef InputTrigger;

        FFloatWriteRef OutputSignal1;
        FFloatWriteRef OutputSignal2;
        FFloatWriteRef OutputSignal3;
        FFloatWriteRef OutputSignal4;
        FFloatWriteRef OutputSignal5;
        FFloatWriteRef OutputSignal6;
        FFloatWriteRef OutputSignal7;
        FFloatWriteRef OutputSignal8;

        float ShiftedValue1;
        float ShiftedValue2;
        float ShiftedValue3;
        float ShiftedValue4;
        float ShiftedValue5;
        float ShiftedValue6;
        float ShiftedValue7;
        float ShiftedValue8;
    };

    class FShiftRegisterNode : public FNodeFacade
    {
    public:
        FShiftRegisterNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FShiftRegisterOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FShiftRegisterNode);
}

#undef LOCTEXT_NAMESPACE