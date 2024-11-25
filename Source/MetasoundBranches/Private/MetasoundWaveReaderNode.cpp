#include "MetasoundBranches/Public/MetasoundWaveReaderNode.h"
#include "MetasoundExecutableOperator.h"     // TExecutableOperator class
#include "MetasoundPrimitives.h"             // ReadRef and WriteRef descriptions for bool, int32, float, and string
#include "MetasoundNodeRegistrationMacro.h"  // METASOUND_LOCTEXT and METASOUND_REGISTER_NODE macros
#include "MetasoundStandardNodesNames.h"     // StandardNodes namespace
#include "MetasoundFacade.h"                 // FNodeFacade class, eliminates the need for a fair amount of boilerplate code
#include "MetasoundParamHelper.h"            // METASOUND_PARAM and METASOUND_GET_PARAM family of macros
#include "MetasoundWave.h"                   // For handling wave assets
#include "Sound/SoundWaveProxyReader.h"      // FSoundWaveProxyReader
#include "DSP/BufferVectorOperations.h"      // Audio utilities
#include "DSP/MultichannelLinearResampler.h" // Resampler
#include "MetasoundAudioBuffer.h"            // Audio buffer definitions

#define LOCTEXT_NAMESPACE "MetasoundStandardNodes_WaveReaderNode"

namespace Metasound
{
    namespace WaveReaderNodeNames
    {
        METASOUND_PARAM(InputPositionSignal, "Audio In", "Input audio signal used as position for reading the wave asset.");
        METASOUND_PARAM(InputWaveAsset, "Wave Asset", "The wave asset to read from.");
        METASOUND_PARAM(OutputSignal, "Audio Out", "Output audio signal.");
    }

    class FWaveReaderOperator : public TExecutableOperator<FWaveReaderOperator>
    {
    public:
        FWaveReaderOperator(
            const FOperatorSettings& InSettings,
            const FAudioBufferReadRef& InPositionSignal,
            const FWaveAssetReadRef& InWaveAsset)
            : InputPositionSignal(InPositionSignal)
            , InputWaveAsset(InWaveAsset)
            , OutputSignal(FAudioBufferWriteRef::CreateNew(InSettings))
            , OperatorSettings(InSettings)
        {
            Initialize();
        }

        static const FVertexInterface& DeclareVertexInterface()
        {
            using namespace WaveReaderNodeNames;

            static const FVertexInterface Interface(
                FInputVertexInterface(
                    TInputDataVertexModel<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputPositionSignal)),
                    TInputDataVertexModel<FWaveAsset>(METASOUND_GET_PARAM_NAME_AND_METADATA(InputWaveAsset))
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

                    Metadata.ClassName = { StandardNodes::Namespace, TEXT("WaveReader"), StandardNodes::AudioVariant };
                    Metadata.MajorVersion = 1;
                    Metadata.MinorVersion = 0;
                    Metadata.DisplayName = METASOUND_LOCTEXT("WaveReaderNodeDisplayName", "Wave Reader");
                    Metadata.Description = METASOUND_LOCTEXT("WaveReaderNodeDesc", "Fetches samples from a wave asset at positions specified by an input audio signal.");
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

        virtual FDataReferenceCollection GetInputs() const override
        {
            using namespace WaveReaderNodeNames;

            FDataReferenceCollection InputDataReferences;

            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputPositionSignal), InputPositionSignal);
            InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InputWaveAsset), InputWaveAsset);

            return InputDataReferences;
        }

        virtual FDataReferenceCollection GetOutputs() const override
        {
            using namespace WaveReaderNodeNames;

            FDataReferenceCollection OutputDataReferences;

            OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutputSignal), OutputSignal);

            return OutputDataReferences;
        }

        static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
        {
            using namespace WaveReaderNodeNames;

            const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
            const FInputVertexInterface& InputInterface = DeclareVertexInterface().GetInputInterface();

            TDataReadReference<FAudioBuffer> InputPositionSignal = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<FAudioBuffer>(InputInterface, METASOUND_GET_PARAM_NAME(InputPositionSignal), InParams.OperatorSettings);
            TDataReadReference<FWaveAsset> InputWaveAsset = InputCollection.GetDataReadReferenceOrConstruct<FWaveAsset>(METASOUND_GET_PARAM_NAME(InputWaveAsset));

            return MakeUnique<FWaveReaderOperator>(InParams.OperatorSettings, InputPositionSignal, InputWaveAsset);
        }

        void Execute()
        {
            int32 NumFrames = OperatorSettings.GetNumFramesPerBlock();
            const float* PositionData = InputPositionSignal->GetData();
            float* OutputData = OutputSignal->GetData();

            if (TotalNumFrames == 0)
            {
                // Output silence if the wave data is invalid
                FMemory::Memzero(OutputData, sizeof(float) * NumFrames);
                return;
            }

            for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
            {
                // Now, interpret the input position as sample indices directly
                float PositionInFrames = PositionData[FrameIndex];

                // Read sample from wave data
                float SampleValue = GetSampleAtPosition(PositionInFrames);

                OutputData[FrameIndex] = SampleValue;
            }
        }

    private:
        void Initialize()
        {
            // Load the wave asset's audio data
            FSoundWaveProxyPtr SoundWaveProxy = InputWaveAsset->GetSoundWaveProxy();
            if (SoundWaveProxy.IsValid())
            {
                // Create a wave proxy reader
                FSoundWaveProxyReader::FSettings ReaderSettings;
                ReaderSettings.StartTimeInSeconds = 0.0f;
                ReaderSettings.bIsLooping = false;
                ReaderSettings.LoopStartTimeInSeconds = 0.0f;
                ReaderSettings.LoopDurationInSeconds = -1.0f;
                ReaderSettings.MaxDecodeSizeInFrames = 1024;

                WaveProxyReader = FSoundWaveProxyReader::Create(SoundWaveProxy.ToSharedRef(), ReaderSettings);

                if (!WaveProxyReader.IsValid())
                {
                    NumChannels = 0;
                    TotalNumFrames = 0;
                    return;
                }

                // Read wave into memory
                InterleavedWaveData.Reset();
                Audio::FAlignedFloatBuffer TempBuffer;
                int32 NumFramesRead = 0;

                do
                {
                    NumFramesRead = WaveProxyReader->PopAudio(TempBuffer);
                    if (NumFramesRead > 0)
                    {
                        InterleavedWaveData.Append(TempBuffer);
                    }

                    if (WaveProxyReader->HasFailed())
                    {
                        NumChannels = 0;
                        TotalNumFrames = 0;
                        return;
                    }
                }
                while (NumFramesRead > 0);

                NumChannels = WaveProxyReader->GetNumChannels();
                WaveSampleRate = WaveProxyReader->GetSampleRate();

                // Deinterleave the wave data
                DeinterleaveWaveData();
            }
            else
            {
                // Wave data is invalid
                NumChannels = 0;
                TotalNumFrames = 0;
            }
        }

        void DeinterleaveWaveData()
        {
            int32 NumSamples = InterleavedWaveData.Num();
            if (NumChannels == 0)
            {
                return;
            }

            int32 NumFrames = NumSamples / NumChannels;
            TotalNumFrames = NumFrames;

            DeinterleavedWaveData.SetNum(NumChannels);
            for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
            {
                DeinterleavedWaveData[ChannelIndex].SetNum(NumFrames);
            }

            for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
            {
                for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
                {
                    DeinterleavedWaveData[ChannelIndex][FrameIndex] = InterleavedWaveData[FrameIndex * NumChannels + ChannelIndex];
                }
            }
        }

        float GetSampleAtPosition(float PositionInFrames)
        {
            // Handle out-of-bounds positions
            if (PositionInFrames < 0.0f || PositionInFrames >= (float)TotalNumFrames)
            {
                return 0.0f;
            }

            // Test with first channel only, for now....
            const TArray<float>& ChannelData = DeinterleavedWaveData[0];

            // Linear interpolation (could be optional)
            int32 Index1 = FMath::Clamp(FMath::FloorToInt(PositionInFrames), 0, TotalNumFrames - 1);
            int32 Index2 = FMath::Min(Index1 + 1, TotalNumFrames - 1);
            float Fraction = PositionInFrames - static_cast<float>(Index1);

            return FMath::Lerp(ChannelData[Index1], ChannelData[Index2], Fraction);
        }

        // Inputs
        FAudioBufferReadRef InputPositionSignal;
        FWaveAssetReadRef InputWaveAsset;

        // Outputs
        FAudioBufferWriteRef OutputSignal;

        // Internal
        FOperatorSettings OperatorSettings;
        Audio::FAlignedFloatBuffer InterleavedWaveData;
        TArray<TArray<float>> DeinterleavedWaveData;
        int32 NumChannels = 0;
        int32 WaveSampleRate = 0;
        int32 TotalNumFrames = 0;

        TUniquePtr<FSoundWaveProxyReader> WaveProxyReader;
    };

    class FWaveReaderNode : public FNodeFacade
    {
    public:
        FWaveReaderNode(const FNodeInitData& InitData)
            : FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FWaveReaderOperator>())
        {
        }
    };

    METASOUND_REGISTER_NODE(FWaveReaderNode);
}

#undef LOCTEXT_NAMESPACE