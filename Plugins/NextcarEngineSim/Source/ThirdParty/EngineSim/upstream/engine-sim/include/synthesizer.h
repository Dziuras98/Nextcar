#ifndef ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H
#define ATG_ENGINE_SIM_ENGINE_SYNTHESIZER_H

#include "butterworth_low_pass_filter.h"
#include "convolution_filter.h"
#include "derivative_filter.h"
#include "jitter_filter.h"
#include "leveling_filter.h"
#include "low_pass_filter.h"
#include "ring_buffer.h"

#include <cinttypes>
#include <cstdint>

class Synthesizer {
public:
    struct AudioParameters {
        float volume = 1.0f;
        float convolution = 1.0f;
        float dF_F_mix = 0.01f;
        float inputSampleNoise = 0.5f;
        float inputSampleNoiseFrequencyCutoff = 10000.0f;
        float airNoise = 1.0f;
        float airNoiseFrequencyCutoff = 2000.0f;
        float levelerTarget = 30000.0f;
        float levelerMaxGain = 1.9f;
        float levelerMinGain = 0.00001f;
    };

    struct Parameters {
        int inputChannelCount = 1;
        int inputBufferSize = 1024;
        int audioBufferSize = 44100;
        float inputSampleRate = 10000;
        float audioSampleRate = 44100;
        AudioParameters initialAudioParameters;
    };

    struct InputChannel {
        RingBuffer<float> data;
        float *transferBuffer = nullptr;
        double lastInputSample = 0.0;
    };

    struct ProcessingFilters {
        ConvolutionFilter convolution;
        DerivativeFilter derivative;
        JitterFilter jitterFilter;
        ButterworthLowPassFilter<float> airNoiseLowPass;
        LowPassFilter inputDcFilter;
        ButterworthLowPassFilter<double> antialiasing;
    };

    Synthesizer();
    ~Synthesizer();

    void initialize(const Parameters &p);
    void initializeImpulseResponse(const int16_t *impulseResponse, unsigned int samples, float volume, int index);
    void destroy();

    int readAudioOutput(int samples, int16_t *buffer);
    void writeInput(const double *data);
    void endInputBlock() {}

    int processSynchronous(int maximumFrames);
    int getInputBufferedFrames() const;
    int getAudioBufferedFrames() const { return static_cast<int>(m_audioBuffer.size()); }
    int getAudioFreeFrames() const { return static_cast<int>(m_audioBuffer.freeSpace()); }
    int getImpulseResponseSampleCount(int index) const {
        return index >= 0 && index < m_inputChannelCount
            ? m_filters[index].convolution.getSampleCount()
            : 0;
    }

    void setInputSampleRate(double sampleRate) { m_inputSampleRate = static_cast<float>(sampleRate); }
    double getInputSampleRate() const { return m_inputSampleRate; }
    double getLatency() const;

    int16_t renderAudio(int inputOffset);

    double getLevelerGain() { return m_levelingFilter.getAttenuation(); }
    AudioParameters getAudioParameters() const { return m_audioParameters; }
    void setAudioParameters(const AudioParameters &params) { m_audioParameters = params; }
    void setDeterministicSeed(std::uint32_t seed) { m_rngState = seed == 0 ? 1u : seed; }

private:
    float nextSignedUnit();

    ButterworthLowPassFilter<float> m_antialiasing;
    LevelingFilter m_levelingFilter;
    InputChannel *m_inputChannels = nullptr;
    AudioParameters m_audioParameters;
    int m_inputChannelCount = 0;
    int m_inputBufferSize = 0;
    int m_latency = 0;
    double m_resampleAccumulator = 0.0;

    RingBuffer<int16_t> m_audioBuffer;
    int m_audioBufferSize = 0;
    float m_inputSampleRate = 0.0f;
    float m_audioSampleRate = 0.0f;
    ProcessingFilters *m_filters = nullptr;
    std::uint32_t m_rngState = 0x4E433033u;
};

#endif
