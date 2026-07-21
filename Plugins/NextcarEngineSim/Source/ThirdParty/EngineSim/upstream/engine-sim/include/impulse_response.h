#ifndef ATG_ENGINE_SIM_IMPULSE_RESPONSE_H
#define ATG_ENGINE_SIM_IMPULSE_RESPONSE_H

#include <cstddef>
#include <cstdint>

class ImpulseResponse {
public:
    ImpulseResponse() = default;
    virtual ~ImpulseResponse() = default;

    void initialize(const int16_t *samples, std::size_t sampleCount, unsigned int sampleRate, double volume) {
        m_samples = samples;
        m_sampleCount = sampleCount;
        m_sampleRate = sampleRate;
        m_volume = volume;
    }

    const int16_t *getSamples() const { return m_samples; }
    std::size_t getSampleCount() const { return m_sampleCount; }
    unsigned int getSampleRate() const { return m_sampleRate; }
    double getVolume() const { return m_volume; }

private:
    const int16_t *m_samples = nullptr;
    std::size_t m_sampleCount = 0;
    unsigned int m_sampleRate = 0;
    double m_volume = 1.0;
};

#endif
