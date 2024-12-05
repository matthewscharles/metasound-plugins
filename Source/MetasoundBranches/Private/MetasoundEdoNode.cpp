#include "MetasoundBranches/Public/MetasoundEdoNode.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundPrimitives.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundFacade.h"
#include "MetasoundParamHelper.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_EdoNode"

namespace Metasound
{
    namespace EdoNodeNames
    {
        METASOUND_PARAM(InputNoteNumber, "Note Number", "Input MIDI note number (integer).");
        METASOUND_PARAM(InputReferenceFrequency, "Reference Frequency", "Reference frequency (float).");
        METASOUND_PARAM(InputReferenceMIDINote, "Reference MIDI Note", "Reference MIDI note (integer).");
        METASOUND_PARAM(InputDivisions, "Divisions", "Number of divisions of the octave (integer).");
        METASOUND_PARAM(OutputFrequency, "Frequency", "Output frequency (float).");
    }

    class FEdoNodeOperator : public TExecutableOperator<FEdoNodeOperator>
    {
    public:
        FEdoNodeOperator(
            const FOperatorSettings& InSettings,
            const FInt32ReadRef& InNoteNumber,
            const FFloatReadRef& InReferenceFrequency,
            const FInt32ReadRef& InReferenceMIDINote,
            const FInt32ReadRef& InDivisions)
            : NoteNumber(InNoteNumber)
            , ReferenceFrequency(InReferenceFrequency)
            , ReferenceMIDINote(InReferenceMIDINote)
            , Divisions(InDivisions)
            , OutputFrequency(FFloatWriteRef::CreateNew(0.0f))
        {
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace EdoNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputNoteNumber)),
                    TInputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputReferenceFrequency), 440.0f),
                    TInputDataVertexModel<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputReferenceMIDINote), 69),
                    TInputDataVertexModel<int32>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputDivisions), 12)
                ),
                FOutputVertexInterface(
                    TOutputDataVertexModel<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutputFrequency))
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

                Metadata.ClassName = { StandardNodes::Namespace, TEXT("EDO"), TEXT("Float") };
                Metadata.MajorVersion = 1;
                Metadata.MinorVersion = 0;
                Metadata.DisplayName = METASOUND_LOCTEXT("EdoNodeDisplayName", "EDO");
                Metadata.Description = METASOUND_LOCTEXT("EdoNodeDesc", "Generates a frequency based on equal division of the octave.");
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
            using namespace EdoNodeNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputNoteNumber), NoteNumber);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputReferenceFrequency), ReferenceFrequency);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputReferenceMIDINote), ReferenceMIDINote);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputDivisions), Divisions);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace EdoNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputFrequency), OutputFrequency);

            return OutputDataReferences;
        }

        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace EdoNodeNames;

            const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<int32> NoteNumber = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, METASOUND_GET_PARAM_NAME(InputNoteNumber), InParams.OperatorSettings);
            TDataReadReference<float> ReferenceFrequency = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InputReferenceFrequency), InParams.OperatorSettings);
            TDataReadReference<int32> ReferenceMIDINote = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, METASOUND_GET_PARAM_NAME(InputReferenceMIDINote), InParams.OperatorSettings);
            TDataReadReference<int32> Divisions = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<int32>(InputInterface, METASOUND_GET_PARAM_NAME(InputDivisions), InParams.OperatorSettings);

            return MakeUnique<FEdoNodeOperator>(
                InParams.OperatorSettings,
                NoteNumber,
                ReferenceFrequency,
                ReferenceMIDINote,
                Divisions
            );
        }

        void Execute()
        {
            int32 noteNumber = *NoteNumber;
            float refFreq = *ReferenceFrequency;
            int32 refMIDINote = *ReferenceMIDINote;
            int32 divisions = *Divisions;

            if (divisions == 0)
            {
                divisions = 1;
            }

            float exponent = static_cast<float>(noteNumber - refMIDINote) / static_cast<float>(divisions);
            float frequency = refFreq * powf(2.0f, exponent);

            *OutputFrequency = frequency;
        }

    private:
        FInt32ReadRef NoteNumber;
        FFloatReadRef ReferenceFrequency;
        FInt32ReadRef ReferenceMIDINote;
        FInt32ReadRef Divisions;

        FFloatWriteRef OutputFrequency;
    };

    class FEdoNode : public FNodeFacade
    {
    public:
        FEdoNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FEdoNodeOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FEdoNode);
}

#undef LOCTEXT_NAMESPACE