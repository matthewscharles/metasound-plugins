// MetasoundEdgeFloatNode.cpp

#include "MetasoundBranches/Public/MetasoundEdgeFloatNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "MetasoundTrigger.h"                // For FTriggerWriteRef and FTrigger

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_EdgeFloat"

namespace Metasound
{
    namespace EdgeFloatNames
    {
        METASOUND_PARAM(InputSignal, "In", "Float value to monitor for edge detection.");
        METASOUND_PARAM(InputDebounce, "Debounce", "Debounce time in seconds to prevent rapid triggering.");

        METASOUND_PARAM(OutputTriggerRise, "Rise", "Trigger on rise.");
        METASOUND_PARAM(OutputTriggerFall, "Fall", "Trigger on fall.");
    }

    class FEdgeFloatOperator : public TExecutableOperator<FEdgeFloatOperator>
    {
    public:
        // Constructor
        FEdgeFloatOperator(
            const FFloatReadRef& InSignal,
            const FFloatReadRef& InDebounce,
            const FOperatorSettings& InSettings)
            : InputSignal(InSignal)
            , InputDebounce(InDebounce)
            , OutputTriggerRise(FTriggerWriteRef::CreateNew(InSettings))
            , OutputTriggerFall(FTriggerWriteRef::CreateNew(InSettings))
            , PreviousSignalValue(*InSignal)
            , DebounceSamples(0)
            , DebounceCounter(0)
            , SampleRate(InSettings.GetSampleRate())
        {
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace EdgeFloatNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputSignal)),
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDebounce), 0.0f)
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerRise)),
                    TOutputDataVertexModel<FTrigger>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputTriggerFall))
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

                Metadata.ClassName = { StandardNodes::Namespace, TEXT("Edge (float)"), TEXT("Float") };
                Metadata.MajorVersion = 1;
                Metadata.MinorVersion = 0;
                Metadata.DisplayName = METASOUND_LOCTEXT("EdgeFloatNodeDisplayName", "Edge (float)");
                Metadata.Description = METASOUND_LOCTEXT("EdgeFloatNodeDesc", "Detects upward and downward changes in an input float signal, with optional debounce.");
                Metadata.Author = "Charles Matthews";
                Metadata.PromptIfMissing = PluginNodeMissingPrompt;
                Metadata.DefaultInterface = NodeInterface;
                Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Custom", "Branches") };
                Metadata.Keywords = TArray<FText>(); // Add relevant keywords if necessary

                return Metadata;
            };

            static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
            return Metadata;
        }

        virtual FDataReferenceCollection GetInputs() const override
        {
            using namespace EdgeFloatNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputSignal), InputSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputDebounce), InputDebounce);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace EdgeFloatNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputTriggerRise), OutputTriggerRise);
            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputTriggerFall), OutputTriggerFall);

            return OutputDataReferences;
        }

        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace EdgeFloatNames;

            const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<float> InputSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(
                InputInterface, METASOUND_GET_PARAM_NAME(InputSignal), InParams.OperatorSettings);

            TDataReadReference<float> InputDebounce = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(
                InputInterface, METASOUND_GET_PARAM_NAME(InputDebounce), InParams.OperatorSettings);

            return MakeUnique<FEdgeFloatOperator>(InputSignal, InputDebounce, InParams.OperatorSettings);
        }
        
        virtual void Reset(const IOperator::FResetParams& InParams)
        {
            // Reset triggers
            OutputTriggerRise->Reset();
            OutputTriggerFall->Reset();

            // Initialize previous signal value
            PreviousSignalValue = *InputSignal;

            // Reset debounce counter
            DebounceCounter = 0;
        }

        void Execute()
        {
            OutputTriggerRise->AdvanceBlock();
            OutputTriggerFall->AdvanceBlock();

            // Recalculate debounce samples if debounce time or sample rate has changed
            if (LastDebounceTime != *InputDebounce || LastSampleRate != SampleRate)
            {
                DebounceSamples = FMath::RoundToInt(FMath::Clamp(*InputDebounce, 0.001f, 5.0f) * SampleRate);
                LastDebounceTime = *InputDebounce;
                LastSampleRate = SampleRate;
            }

            // Decrement debounce counter
            if (DebounceCounter > 0)
            {
                DebounceCounter--;
            }

            float CurrentSignal = *InputSignal;

            // Detect rising edge
            if (CurrentSignal > PreviousSignalValue && !PreviousIsRising && DebounceCounter <= 0)
            {
                // Rising edge detected
                OutputTriggerRise->TriggerFrame(0);
                DebounceCounter = DebounceSamples;
                PreviousIsRising = true;
            }
            // Detect falling edge
            else if (CurrentSignal < PreviousSignalValue && PreviousIsRising && DebounceCounter <= 0)
            {
                // Falling edge detected
                OutputTriggerFall->TriggerFrame(0);
                DebounceCounter = DebounceSamples;
                PreviousIsRising = false;
            }

            // Update previous signal value
            PreviousSignalValue = CurrentSignal;
        }

    private:
        // Inputs
        FFloatReadRef InputSignal;
        FFloatReadRef InputDebounce;

        // Outputs
        FTriggerWriteRef OutputTriggerRise;
        FTriggerWriteRef OutputTriggerFall;

        // Internal variables
        float PreviousSignalValue;
        bool PreviousIsRising = false;
        int32 DebounceSamples = 0;
        int32 DebounceCounter = 0;
        float SampleRate;
        
        // Variables to track changes in debounce time and sample rate
        float LastDebounceTime = -1.0f;
        float LastSampleRate = -1.0f;
    };

    class FEdgeFloatNode : public FNodeFacade
    {
    public:
        FEdgeFloatNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FEdgeFloatOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FEdgeFloatNode);
}

#undef LOCTEXT_NAMESPACE