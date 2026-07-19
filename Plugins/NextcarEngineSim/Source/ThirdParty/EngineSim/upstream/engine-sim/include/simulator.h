#ifndef ATG_ENGINE_SIM_SIMULATOR_H
#define ATG_ENGINE_SIM_SIMULATOR_H

#include "engine.h"
#include "synthesizer.h"
#include "starter_motor.h"
#include "gauss_seidel_sle_solver.h"
#include "optimized_nsv_rigid_body_system.h"
#include "rigid_body_system.h"

#include <chrono>
#include <cstdint>

class Simulator {
public:
    struct Parameters {};
    static constexpr int MaximumSynchronousFrames = 2000;

    Simulator();
    virtual ~Simulator();

    virtual void initialize(const Parameters &params = Parameters{});
    void loadSimulation(Engine *engine);
    void releaseSimulation();

    virtual void startFrame(double dt);
    bool simulateStep();
    virtual double getTotalExhaustFlow() const;
    int readAudioOutput(int samples, int16_t *target);
    virtual int endFrame(int maximumSynchronousFrames);
    virtual void destroy();

    int getFrameIterationCount() const { return m_steps; }
    int getCurrentIteration() const { return m_currentIteration; }
    int simulationSteps() const { return m_steps; }

    Synthesizer &synthesizer() { return m_synthesizer; }
    const Synthesizer &synthesizer() const { return m_synthesizer; }

    Engine *getEngine() const { return m_engine; }
    atg_scs::RigidBodySystem *getSystem() { return m_system; }

    void setSimulationFrequency(int frequency) { m_simulationFrequency = frequency; }
    int getSimulationFrequency() const { return m_simulationFrequency; }
    double getTimestep() const { return 1.0 / m_simulationFrequency; }

    void setSimulationSpeed(double speed) { m_simulationSpeed = speed; }
    double getSimulationSpeed() const { return m_simulationSpeed; }
    double getAverageProcessingTime() const { return m_physicsProcessingTime; }
    double filteredEngineSpeed() const { return m_filteredEngineSpeed; }

    virtual double getAverageOutputSignal() const;

    StarterMotor m_starterMotor;

protected:
    void initializeSynthesizer();
    virtual void simulateStep_();
    virtual void writeToSynthesizer() = 0;
    void updateFilteredEngineSpeed(double dt);

    atg_scs::RigidBodySystem *m_system;
    Engine *m_engine;

private:
    Synthesizer m_synthesizer;
    std::chrono::steady_clock::time_point m_simulationStart;
    int m_currentIteration;
    double m_physicsProcessingTime;
    int m_simulationFrequency;
    double m_simulationSpeed;
    double m_filteredEngineSpeed;
    int m_steps;
};

#endif
