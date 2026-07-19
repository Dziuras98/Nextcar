#include "../include/simulator.h"

#include <algorithm>
#include <cassert>
#include <cmath>

Simulator::Simulator()
    : m_system(nullptr), m_engine(nullptr), m_currentIteration(0),
      m_physicsProcessingTime(0.0), m_simulationFrequency(10000),
      m_simulationSpeed(1.0), m_filteredEngineSpeed(0.0), m_steps(0) {}

Simulator::~Simulator() { assert(m_system == nullptr); }

void Simulator::initialize(const Parameters &params) {
    (void)params;
    assert(m_system == nullptr);
    auto *system = new atg_scs::OptimizedNsvRigidBodySystem;
    system->initialize(new atg_scs::GaussSeidelSleSolver);
    m_system = system;
}

void Simulator::loadSimulation(Engine *engine) {
    assert(m_system != nullptr);
    assert(engine != nullptr);
    m_engine = engine;
    setSimulationFrequency(static_cast<int>(engine->getSimulationFrequency()));
}

void Simulator::releaseSimulation() { destroy(); }

void Simulator::startFrame(double dt) {
    if (m_engine == nullptr || dt <= 0.0) {
        m_steps = 0;
        m_currentIteration = 0;
        return;
    }
    m_simulationStart = std::chrono::steady_clock::now();
    m_currentIteration = 0;
    m_synthesizer.setInputSampleRate(m_simulationFrequency * m_simulationSpeed);
    m_steps = std::max(0, static_cast<int>(
        std::round(dt * m_simulationSpeed * m_simulationFrequency)));
    for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
        m_engine->getIntake(i)->m_flowRate = 0.0;
    }
}

bool Simulator::simulateStep() {
    if (m_engine == nullptr || m_currentIteration >= m_steps) {
        if (m_currentIteration >= m_steps && m_steps > 0) {
            const auto now = std::chrono::steady_clock::now();
            const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                now - m_simulationStart).count();
            m_physicsProcessingTime = m_physicsProcessingTime * 0.98 + 0.02 * us;
        }
        return false;
    }

    const double dt = getTimestep();
    m_system->process(dt, 1);
    m_engine->update(dt);
    updateFilteredEngineSpeed(dt);
    Crankshaft *output = m_engine->getOutputCrankshaft();
    if (output != nullptr) {
        output->resetAngle();
        for (int i = 0; i < m_engine->getCrankshaftCount(); ++i) {
            m_engine->getCrankshaft(i)->m_body.theta = output->m_body.theta;
        }
    }
    simulateStep_();
    writeToSynthesizer();
    ++m_currentIteration;
    return true;
}

double Simulator::getTotalExhaustFlow() const { return 0.0; }

int Simulator::readAudioOutput(int samples, int16_t *target) {
    return m_synthesizer.readAudioOutput(samples, target);
}

int Simulator::endFrame(int maximumSynchronousFrames) {
    m_synthesizer.endInputBlock();
    const int capped = std::clamp(
        maximumSynchronousFrames,
        0,
        MaximumSynchronousFrames);
    return m_synthesizer.processSynchronous(capped);
}

void Simulator::destroy() {
    m_synthesizer.destroy();
    if (m_system != nullptr) {
        m_system->reset();
        delete m_system;
        m_system = nullptr;
    }
    m_engine = nullptr;
    m_currentIteration = 0;
    m_steps = 0;
}

double Simulator::getAverageOutputSignal() const { return 0.0; }

void Simulator::initializeSynthesizer() {
    assert(m_engine != nullptr);
    Synthesizer::Parameters parameters;
    parameters.audioBufferSize = 44100;
    parameters.audioSampleRate = 44100;
    parameters.inputBufferSize = 44100;
    parameters.inputChannelCount = m_engine->getExhaustSystemCount();
    parameters.inputSampleRate = static_cast<float>(getSimulationFrequency());
    parameters.initialAudioParameters.dF_F_mix =
        static_cast<float>(m_engine->getInitialHighFrequencyGain());
    parameters.initialAudioParameters.inputSampleNoise =
        static_cast<float>(m_engine->getInitialJitter());
    parameters.initialAudioParameters.airNoise =
        static_cast<float>(m_engine->getInitialNoise());
    m_synthesizer.initialize(parameters);

    for (int i = 0; i < m_engine->getExhaustSystemCount(); ++i) {
        ExhaustSystem *exhaust = m_engine->getExhaustSystem(i);
        ImpulseResponse *response = exhaust->getImpulseResponse();
        if (response == nullptr) {
            static constexpr int16_t IdentityImpulse = INT16_MAX;
            m_synthesizer.initializeImpulseResponse(&IdentityImpulse, 1, 1.0f, i);
            continue;
        }
        m_synthesizer.initializeImpulseResponse(
            response->getSamples(),
            static_cast<unsigned int>(response->getSampleCount()),
            static_cast<float>(response->getVolume()),
            i);
    }
}

void Simulator::simulateStep_() {}

void Simulator::updateFilteredEngineSpeed(double dt) {
    const double alpha = dt / (100.0 + dt);
    m_filteredEngineSpeed = alpha * m_filteredEngineSpeed
        + (1.0 - alpha) * m_engine->getRpm();
}
