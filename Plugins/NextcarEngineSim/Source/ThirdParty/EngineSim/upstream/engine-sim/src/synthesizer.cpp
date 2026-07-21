#include "../include/synthesizer.h"

#include "../include/utilities.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

Synthesizer::Synthesizer() = default;

Synthesizer::~Synthesizer() {
    assert(m_inputChannels == nullptr);
    assert(m_filters == nullptr);
}

void Synthesizer::initialize(const Parameters &p) {
    destroy();
    m_inputChannelCount = std::max(0, p.inputChannelCount);
    m_inputBufferSize = std::max(0, p.inputBufferSize);
    m_audioBufferSize = std::max(0, p.audioBufferSize);
    m_inputSampleRate = p.inputSampleRate;
    m_audioSampleRate = p.audioSampleRate;
    m_audioParameters = p.initialAudioParameters;
    m_latency = 0;
    m_resampleAccumulator = 0.0;

    m_audioBuffer.initialize(static_cast<size_t>(m_audioBufferSize));
    if (m_inputChannelCount == 0) return;

    m_inputChannels = new InputChannel[m_inputChannelCount];
    m_filters = new ProcessingFilters[m_inputChannelCount];
    for (int i = 0; i < m_inputChannelCount; ++i) {
        m_inputChannels[i].transferBuffer = new float[static_cast<size_t>(m_inputBufferSize)];
        m_inputChannels[i].data.initialize(static_cast<size_t>(m_inputBufferSize));
        m_filters[i].airNoiseLowPass.setCutoffFrequency(
            m_audioParameters.airNoiseFrequencyCutoff, m_audioSampleRate);
        m_filters[i].derivative.m_dt = 1.0f / m_audioSampleRate;
        m_filters[i].inputDcFilter.setCutoffFrequency(10.0f);
        m_filters[i].inputDcFilter.m_dt = 1.0f / m_audioSampleRate;
        m_filters[i].jitterFilter.initialize(
            10,
            m_audioParameters.inputSampleNoiseFrequencyCutoff,
            m_audioSampleRate);
        m_filters[i].antialiasing.setCutoffFrequency(1900.0, m_audioSampleRate);
    }

    m_levelingFilter.p_target = m_audioParameters.levelerTarget;
    m_levelingFilter.p_maxLevel = m_audioParameters.levelerMaxGain;
    m_levelingFilter.p_minLevel = m_audioParameters.levelerMinGain;
    m_antialiasing.setCutoffFrequency(m_audioSampleRate * 0.45f, m_audioSampleRate);
}

void Synthesizer::initializeImpulseResponse(
    const int16_t *impulseResponse,
    unsigned int samples,
    float volume,
    int index)
{
    assert(index >= 0 && index < m_inputChannelCount);
    assert(impulseResponse != nullptr || samples == 0);

    unsigned int clippedLength = 0;
    for (unsigned int i = 0; i < samples; ++i) {
        if (std::abs(static_cast<int>(impulseResponse[i])) > 100) clippedLength = i + 1;
    }
    const unsigned int sampleCount = std::max(1u, std::min(10000u, clippedLength));
    m_filters[index].convolution.destroy();
    m_filters[index].convolution.initialize(static_cast<int>(sampleCount));
    float *response = m_filters[index].convolution.getImpulseResponse();
    if (clippedLength == 0) {
        response[0] = 1.0f;
        return;
    }
    for (unsigned int i = 0; i < sampleCount; ++i) {
        response[i] = volume * static_cast<float>(impulseResponse[i])
            / static_cast<float>(std::numeric_limits<int16_t>::max());
    }
}

void Synthesizer::destroy() {
    if (m_inputChannels != nullptr) {
        for (int i = 0; i < m_inputChannelCount; ++i) {
            delete[] m_inputChannels[i].transferBuffer;
            m_inputChannels[i].transferBuffer = nullptr;
            m_inputChannels[i].data.destroy();
        }
    }
    if (m_filters != nullptr) {
        for (int i = 0; i < m_inputChannelCount; ++i) {
            m_filters[i].convolution.destroy();
            m_filters[i].jitterFilter.destroy();
        }
    }
    delete[] m_inputChannels;
    delete[] m_filters;
    m_inputChannels = nullptr;
    m_filters = nullptr;
    m_inputChannelCount = 0;
    m_inputBufferSize = 0;
    m_audioBufferSize = 0;
    m_latency = 0;
    m_resampleAccumulator = 0.0;
    m_audioBuffer.destroy();
}

int Synthesizer::readAudioOutput(int samples, int16_t *buffer) {
    if (samples <= 0) return 0;
    assert(buffer != nullptr);
    const int available = getAudioBufferedFrames();
    const int consumed = std::min(samples, available);
    if (consumed > 0) m_audioBuffer.readAndRemove(static_cast<size_t>(consumed), buffer);
    if (consumed < samples) {
        std::memset(buffer + consumed, 0, sizeof(int16_t) * static_cast<size_t>(samples - consumed));
    }
    return consumed;
}

void Synthesizer::writeInput(const double *data) {
    if (m_inputChannelCount == 0 || m_inputSampleRate <= 0 || m_audioSampleRate <= 0) return;
    assert(data != nullptr);

    m_resampleAccumulator += static_cast<double>(m_audioSampleRate) / m_inputSampleRate;
    const int outputFrames = static_cast<int>(std::floor(m_resampleAccumulator));
    m_resampleAccumulator -= outputFrames;
    if (outputFrames <= 0) {
        for (int channel = 0; channel < m_inputChannelCount; ++channel) {
            m_inputChannels[channel].lastInputSample = data[channel];
        }
        return;
    }

    for (int frame = 0; frame < outputFrames; ++frame) {
        const double t = static_cast<double>(frame + 1) / outputFrames;
        bool hasSpace = true;
        for (int channel = 0; channel < m_inputChannelCount; ++channel) {
            if (m_inputChannels[channel].data.full()) {
                hasSpace = false;
                break;
            }
        }
        if (!hasSpace) break;
        for (int channel = 0; channel < m_inputChannelCount; ++channel) {
            const double previous = m_inputChannels[channel].lastInputSample;
            const double interpolated = previous + (data[channel] - previous) * t;
            const float filtered = static_cast<float>(
                m_filters[channel].antialiasing.fast_f(interpolated));
            if (!m_inputChannels[channel].data.write(filtered)) return;
        }
    }

    for (int channel = 0; channel < m_inputChannelCount; ++channel) {
        m_inputChannels[channel].lastInputSample = data[channel];
    }
    m_latency = getInputBufferedFrames();
}

int Synthesizer::getInputBufferedFrames() const {
    if (m_inputChannelCount == 0 || m_inputChannels == nullptr) return 0;
    size_t available = m_inputChannels[0].data.size();
    for (int i = 1; i < m_inputChannelCount; ++i) {
        available = std::min(available, m_inputChannels[i].data.size());
    }
    return static_cast<int>(available);
}

int Synthesizer::processSynchronous(int maximumFrames) {
    if (maximumFrames <= 0 || m_inputChannelCount == 0) return 0;
    const int frames = std::min({
        maximumFrames,
        getInputBufferedFrames(),
        getAudioFreeFrames()
    });
    if (frames <= 0) return 0;

    for (int channel = 0; channel < m_inputChannelCount; ++channel) {
        m_inputChannels[channel].data.read(
            static_cast<size_t>(frames),
            m_inputChannels[channel].transferBuffer);
        m_filters[channel].airNoiseLowPass.setCutoffFrequency(
            m_audioParameters.airNoiseFrequencyCutoff,
            m_audioSampleRate);
        m_filters[channel].jitterFilter.setJitterScale(
            m_audioParameters.inputSampleNoise);
    }

    int produced = 0;
    for (int frame = 0; frame < frames; ++frame) {
        if (!m_audioBuffer.write(renderAudio(frame))) break;
        ++produced;
    }
    for (int channel = 0; channel < m_inputChannelCount; ++channel) {
        m_inputChannels[channel].data.removeBeginning(static_cast<size_t>(produced));
    }
    m_latency = getInputBufferedFrames();
    return produced;
}

double Synthesizer::getLatency() const {
    return m_audioSampleRate > 0
        ? static_cast<double>(m_latency) / m_audioSampleRate
        : 0.0;
}

float Synthesizer::nextSignedUnit() {
    std::uint32_t x = m_rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_rngState = x == 0 ? 1u : x;
    const float unit = static_cast<float>(m_rngState) /
        static_cast<float>(std::numeric_limits<std::uint32_t>::max());
    return 2.0f * unit - 1.0f;
}

int16_t Synthesizer::renderAudio(int inputOffset) {
    const float airNoise = m_audioParameters.airNoise;
    const float derivativeMix = m_audioParameters.dF_F_mix;
    const float convolutionAmount = m_audioParameters.convolution;

    float signal = 0.0f;
    for (int channel = 0; channel < m_inputChannelCount; ++channel) {
        const float jittered = m_filters[channel].jitterFilter.fast_f(
            m_inputChannels[channel].transferBuffer[inputOffset]);
        const float dc = m_filters[channel].inputDcFilter.fast_f(jittered);
        const float centered = jittered - dc;
        const float derivative = m_filters[channel].derivative.f(jittered);
        const float noise = m_filters[channel].airNoiseLowPass.fast_f(nextSignedUnit());
        const float mixedNoise = airNoise * noise + (1.0f - airNoise);
        float input = derivative * derivativeMix
            + centered * mixedNoise * (1.0f - derivativeMix);
        if (std::fpclassify(input) == FP_SUBNORMAL) input = 0.0f;
        signal += convolutionAmount * m_filters[channel].convolution.f(input)
            + (1.0f - convolutionAmount) * input;
    }

    signal = m_antialiasing.fast_f(signal);
    m_levelingFilter.p_target = m_audioParameters.levelerTarget;
    const float leveled = m_levelingFilter.f(signal) * m_audioParameters.volume;
    const long rounded = std::lround(leveled);
    return static_cast<int16_t>(std::clamp<long>(
        rounded,
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int16_t>::max()));
}
