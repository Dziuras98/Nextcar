#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "synthesizer.h"
#include "starter_motor.h"
#include "scs.h"

#include <chrono>
#include <cstdint>
#include <thread>

class Simulator {
public:
    enum class SystemType { NsvOptimized };
    struct Parameters { SystemType systemType = SystemType::NsvOptimized; };

    Simulator();
    virtual ~Simulator();
    virtual void initialize(const Parameters &params);
    virtual void loadSimulation(Engine *engine);
    void releaseSimulation();
    virtual void startFrame(double dt);
    bool simulateStep();
    virtual double getTotalExhaustFlow() const;
    int readAudioOutput(int samples, int16_t *target);
    virtual void endFrame();
    virtual void destroy();

    int getFrameIterationCount() const { return m_steps; }
    Synthesizer &synthesizer() { return m_synthesizer; }
    Engine *getEngine() const { return m_engine; }
    atg_scs::RigidBodySystem *getSystem() { return m_system; }
    void setSimulationFrequency(int frequency) { m_simulationFrequency = frequency; }
    int getSimulationFrequency() const { return m_simulationFrequency; }
    double getTimestep() const { return 1.0 / m_simulationFrequency; }
    void setSimulationSpeed(double speed) { m_simulationSpeed = speed; }
    double getSimulationSpeed() const { return m_simulationSpeed; }
    int getCurrentIteration() const { return m_currentIteration; }
    double getAverageProcessingTime() const { return m_physicsProcessingTime; }
    int simulationSteps() const { return m_steps; }
    virtual double getAverageOutputSignal() const;
    double filteredEngineSpeed() const { return m_filteredEngineSpeed; }
    int getLastSynchronousProducedFrames() const { return m_lastSynchronousProducedFrames; }
    std::thread::id getOwnerThreadId() const { return m_ownerThread; }

    StarterMotor m_starterMotor;

protected:
    void initializeSynthesizer();
    virtual void simulateStep_();
    virtual void writeToSynthesizer() = 0;
    void assertOwnerThread() const;
    atg_scs::RigidBodySystem *m_system;

private:
    void updateFilteredEngineSpeed(double dt);
    Synthesizer m_synthesizer;
    std::chrono::steady_clock::time_point m_simulationStart;
    int m_currentIteration;
    Engine *m_engine;
    double m_physicsProcessingTime;
    int m_simulationFrequency;
    double m_simulationSpeed;
    double m_filteredEngineSpeed;
    int m_steps;
    int m_lastSynchronousProducedFrames;
    std::thread::id m_ownerThread;
};

#endif
