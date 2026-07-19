#include "../include/simulator.h"

#include <cassert>
#include <cmath>

Simulator::Simulator()
    : m_system(nullptr), m_currentIteration(0), m_engine(nullptr),
      m_physicsProcessingTime(0), m_simulationFrequency(10000),
      m_simulationSpeed(1), m_filteredEngineSpeed(0), m_steps(0),
      m_lastSynchronousProducedFrames(0), m_ownerThread() {}

Simulator::~Simulator() { assert(m_system == nullptr); }

void Simulator::initialize(const Parameters &params) {
    (void)params;
    m_ownerThread = std::this_thread::get_id();
    auto *system = new atg_scs::OptimizedNsvRigidBodySystem;
    system->initialize(new atg_scs::GaussSeidelSleSolver);
    m_system = system;
}

void Simulator::loadSimulation(Engine *engine) {
    assertOwnerThread();
    m_engine = engine;
}

void Simulator::releaseSimulation() {
    assertOwnerThread();
    destroy();
}

void Simulator::startFrame(double dt) {
    assertOwnerThread();
    if (!m_engine) { m_steps = 0; return; }
    m_simulationStart = std::chrono::steady_clock::now();
    m_currentIteration = 0;
    m_synthesizer.setInputSampleRate(m_simulationFrequency * m_simulationSpeed);
    m_steps = static_cast<int>(std::round(dt * m_simulationSpeed * m_simulationFrequency));
    for (int i = 0; i < m_engine->getIntakeCount(); ++i) m_engine->getIntake(i)->m_flowRate = 0;
}

bool Simulator::simulateStep() {
    assertOwnerThread();
    if (m_currentIteration >= m_steps) {
        const auto now = std::chrono::steady_clock::now();
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - m_simulationStart).count();
        m_physicsProcessingTime = m_physicsProcessingTime * 0.98 + 0.02 * us;
        return false;
    }
    const double dt = getTimestep();
    m_system->process(dt, 1);
    m_engine->update(dt);
    updateFilteredEngineSpeed(dt);
    Crankshaft *output = m_engine->getOutputCrankshaft();
    output->resetAngle();
    for (int i = 0; i < m_engine->getCrankshaftCount(); ++i) m_engine->getCrankshaft(i)->m_body.theta = output->m_body.theta;
    simulateStep_();
    writeToSynthesizer();
    ++m_currentIteration;
    return true;
}

double Simulator::getTotalExhaustFlow() const { return 0; }
int Simulator::readAudioOutput(int samples, int16_t *target) { assertOwnerThread(); return m_synthesizer.readAudioOutput(samples, target); }

void Simulator::endFrame() {
    assertOwnerThread();
    m_synthesizer.endInputBlock();
    m_lastSynchronousProducedFrames = m_synthesizer.processSynchronous(static_cast<int>(m_synthesizer.getInputBufferedFrames()));
}

void Simulator::destroy() {
    assertOwnerThread();
    m_synthesizer.destroy();
    if (m_system) {
        m_system->reset();
        delete m_system;
        m_system = nullptr;
    }
    m_engine = nullptr;
}

double Simulator::getAverageOutputSignal() const { return 0; }

void Simulator::initializeSynthesizer() {
    Synthesizer::Parameters parameters;
    parameters.audioBufferSize = 44100;
    parameters.audioSampleRate = 44100;
    parameters.inputBufferSize = 44100;
    parameters.inputChannelCount = m_engine->getExhaustSystemCount();
    parameters.inputSampleRate = static_cast<float>(getSimulationFrequency());
    parameters.initialAudioParameters.dF_F_mix = static_cast<float>(m_engine->getInitialHighFrequencyGain());
    parameters.initialAudioParameters.inputSampleNoise = static_cast<float>(m_engine->getInitialJitter());
    parameters.initialAudioParameters.airNoise = static_cast<float>(m_engine->getInitialNoise());
    m_synthesizer.initialize(parameters);
}

void Simulator::simulateStep_() {}
void Simulator::updateFilteredEngineSpeed(double dt) {
    const double alpha = dt / (100 + dt);
    m_filteredEngineSpeed = alpha * m_filteredEngineSpeed + (1 - alpha) * m_engine->getRpm();
}
void Simulator::assertOwnerThread() const { assert(m_ownerThread == std::this_thread::get_id()); }
