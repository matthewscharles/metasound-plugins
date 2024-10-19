#include "MetasoundBranches/Public/MetasoundSahBankNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "Modules/ModuleManager.h"           // ModuleManager (is this still necessary?)

// Required for ensuring the node is supported by all languages in engine. Must be unique per MetaSound.
#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_SahBankNode"

namespace Metasound
{
    // Vertex Names - define the node's inputs and outputs here
    namespace SahBankNodeNames
    {
        METASOUND_PARAM(InputSignal1, "Signal 1", "Input signal to sample 1.");
        METASOUND_PARAM(InputTrigger1, "Trigger 1", "Trigger signal 1.");
        METASOUND_PARAM(InputSignal2, "Signal 2", "Input signal to sample 2.");
        METASOUND_PARAM(InputTrigger2, "Trigger 2", "Trigger signal 2.");
        METASOUND_PARAM(InputSignal3, "Signal 3", "Input signal to sample 3.");
        METASOUND_PARAM(InputTrigger3, "Trigger 3", "Trigger signal 3.");
        METASOUND_PARAM(InputSignal4, "Signal 4", "Input signal to sample 4.");
        METASOUND_PARAM(InputTrigger4, "Trigger 4", "Trigger signal 4.");
        METASOUND_PARAM(InputThreshold, "Threshold", "Threshold for triggers.");

        METASOUND_PARAM(OutputSignal1, "Output 1", "Sampled output signal.");
        METASOUND_PARAM(OutputSignal2, "Output 2", "Sampled output signal.");
        METASOUND_PARAM(OutputSignal3, "Output 3", "Sampled output signal.");
        METASOUND_PARAM(OutputSignal4, "Output 4", "Sampled output signal.");
    }

    // Operator Class - defines the way the node is described, created and executed
    class FSahBankOperator : public TExecutableOperator<FSahBankOperator>
    {
    public:
        // Constructor
        FSahBankOperator(
            const TArray<FAudioBufferReadRef>& InSignals,
            const TArray<FAudioBufferReadRef>& InTriggers,
            const FFloatReadRef& InThreshold)
            : InputSignals(InSignals)
            , InputTriggers(InTriggers)
            , InputThreshold(InThreshold)
        {
            int32 NumChannels = InputSignals.Num();
            SampledValues.SetNumZeroed(NumChannels);
            PreviousTriggerValues.SetNumZeroed(NumChannels);
            OutputSignals.SetNum(NumChannels);

            for (int32 i = 0; i < NumChannels; ++i)
            {
                OutputSignals[i] = FAudioBufferWriteRef::CreateNew(InputSignals[i]->Num());
            }
        }

        // Helper function for constructing vertex interface
        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace SaHBankNodeNames;

            const int32 NumChannels = 4;
            FInputVertexInterface InputInterface;
            FOutputVertexInterface OutputInterface;

            for (int32 i = 0; i < NumChannels; ++i)
            {
                InputInterface.Add(TInputDataVertexModel<FAudioBuffer>(
                    *FString::Printf(TEXT("Signal %d"), i + 1),
                    *FString::Printf(TEXT("Input signal to sample %d."), i + 1)));

                InputInterface.Add(TInputDataVertexModel<FAudioBuffer>(
                    *FString::Printf(TEXT("Trigger %d"), i + 1),
                    *FString::Printf(TEXT("Trigger signal %d."), i + 1)));

                OutputInterface.Add(TOutputDataVertexModel<FAudioBuffer>(
                    *FString::Printf(TEXT("Output %d"), i + 1),
                    *FString::Printf(TEXT("Sampled output signal %d."), i + 1)));
            }

            InputInterface.Add(TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputThreshold)));

            static const FVertexInterface Interface(InputInterface, OutputInterface);
            return Interface;
        }

        // Retrieves necessary metadata about the node
        static const FNodeClassMetadata& GetNodeInfo()
        {
            auto CreateNodeClassMetadata = []() -> FNodeClassMetadata
                {
                    FVertexInterface NodeInterface = DeclareVertexInterface();

                    FNodeClassMetadata Metadata;

                    Metadata.ClassName = { StandardNodes::Namespace, TEXT("SaH Bank"), StandardNodes::AudioVariant };
                    Metadata.MajorVersion = 1;
                    Metadata.MinorVersion = 0;
                    Metadata.DisplayName = METASOUND_LOCTEXT("SahBankNodeDisplayName", "SaH Bank");
                    Metadata.Description = METASOUND_LOCTEXT("SahBankNodeDesc", "Bank of 4 sample and hold modules.");
                    Metadata.Author = PluginAuthor;
                    Metadata.PromptIfMissing = PluginNodeMissingPrompt;
                    Metadata.DefaultInterface = DeclareVertexInterface();
                    Metadata.CategoryHierarchy = { METASOUND_LOCTEXT("Custom", "Branches") };
                    Metadata.Keywords = TArray<FText>(); // Keywords for searching

                    return Metadata;
                };

            static const FNodeClassMetadata Metadata = CreateNodeClassMetadata();
            return Metadata;
        }

        // Allows MetaSound graph to interact with the node's inputs
        virtual FDataReferenceCollection GetInputs() const override
        {
            FDataReferenceCollection InputDataReferences;

            for (int32 i = 0; i < InputSignals.Num(); ++i)
            {
                FName SignalParamName = *FString::Printf(TEXT("Signal %d"), i + 1);
                FName TriggerParamName = *FString::Printf(TEXT("Trigger %d"), i + 1);

                InputDataReferences.AddDataReadReference(SignalParamName, InputSignals[i]);
                InputDataReferences.AddDataReadReference(TriggerParamName, InputTriggers[i]);
            }

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputThreshold), InputThreshold);

            return InputDataReferences;
        }

        // Allows MetaSound graph to interact with the node's outputs
        virtual FDataReferenceCollection GetOutputs() const override
        {
            FDataReferenceCollection OutputDataReferences;

            for (int32 i = 0; i < OutputSignals.Num(); ++i)
            {
                FName OutputParamName = *FString::Printf(TEXT("Output %d"), i + 1);
                OutputDataReferences.AddDataReadReference(OutputParamName, OutputSignals[i]);
            }

            return OutputDataReferences;
        }

        // Used to instantiate a new runtime instance of the node
        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace SaHBankNodeNames;

            const Metasound::FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const Metasound::FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TArray<FAudioBufferReadRef> InputSignals;
            TArray<FAudioBufferReadRef> InputTriggers;

            const int32 NumChannels = 4;

            for (int32 i = 0; i < NumChannels; ++i)
            {
                FName SignalParamName = *FString::Printf(TEXT("Signal %d"), i + 1);
                FName TriggerParamName = *FString::Printf(TEXT("Trigger %d"), i + 1);

                InputSignals.Add(InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(
                    InputInterface, SignalParamName, InParams.OperatorSettings));

                InputTriggers.Add(InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(
                    InputInterface, TriggerParamName, InParams.OperatorSettings));
            }

            TDataReadReference<float> InputThreshold = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(
                InputInterface, METASOUND_GET_PARAM_NAME(InputThreshold), InParams.OperatorSettings);

            return MakeUnique<FSahOperator>(InputSignals, InputTriggers, InputThreshold);
        }

        // Primary node functionality
        void Execute()
        {
            int32 NumFrames = InputSignals[0]->Num();
            int32 NumChannels = InputSignals.Num();
            float Threshold = *InputThreshold;

            for (int32 channel = 0; channel < NumChannels; ++channel)
            {
                const float* SignalData = InputSignals[channel]->GetData();
                const float* TriggerData = InputTriggers[channel]->GetData();
                float* OutputData = OutputSignals[channel]->GetData();

                for (int32 i = 0; i < NumFrames; ++i)
                {
                    float CurrentTriggerValue = TriggerData[i];

                    if (PreviousTriggerValues[channel] < Threshold && CurrentTriggerValue >= Threshold)
                    {
                        SampledValues[channel] = SignalData[i];
                    }

                    OutputData[i] = SampledValues[channel];
                    PreviousTriggerValues[channel] = CurrentTriggerValue;
                }
            }
        }

    private:

        // Inputs
        TArray<FAudioBufferReadRef> InputSignals;
        TArray<FAudioBufferReadRef> InputTriggers;

        // Outputs
        TArray<FAudioBufferWriteRef> OutputSignals;

        // Internal variables
        TArray<float> SampledValues;
        TArray<float> PreviousTriggerValues;
    
    };

    // Node Class - Inheriting from FNodeFacade is recommended for nodes that have a static FVertexInterface
    class FSahBankNode : public FNodeFacade
    {
    public:
        FSahBankNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FSahBankOperator>())
        {
        }
    };

    // Register node
    METASOUND_REGISTER_NODE(FSahBankNode);
}

#undef LOCTEXT_NAMESPACE