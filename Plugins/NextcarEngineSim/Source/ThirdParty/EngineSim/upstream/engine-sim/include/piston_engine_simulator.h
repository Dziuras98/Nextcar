#ifndef ATG_ENGINE_SIM_PISTON_ENGINE_SIMULATOR_H
#define ATG_ENGINE_SIM_PISTON_ENGINE_SIMULATOR_H

#include "simulator.h"
#include "combustion_chamber.h"
#include "delay_filter.h"
#include "derivative_filter.h"
#include "clutch_constraint.h"
#include "fixed_position_constraint.h"
#include "line_constraint.h"
#include "link_constraint.h"
#include "rotation_friction_constraint.h"

class PistonEngineSimulator : public Simulator {
public:
    PistonEngineSimulator();
    ~PistonEngineSimulator() override;

    void loadSimulation(Engine *engine);
    void startFrame(double dt) override;
    double getTotalExhaustFlow() const override;
    int endFrame(int maximumSynchronousFrames) override;
    void destroy() override;

    void setFluidSimulationSteps(int steps) { m_fluidSimulationSteps = steps; }
    int getFluidSimulationSteps() const { return m_fluidSimulationSteps; }
    int getFluidSimulationFrequency() const { return m_fluidSimulationSteps * getSimulationFrequency(); }
    double getAverageOutputSignal() const override;
    int getIgnitionEventsThisFrame() const { return m_ignitionEventsThisFrame; }

    DerivativeFilter m_derivativeFilter;

protected:
    void simulateStep_() override;
    void writeToSynthesizer() override;

private:
    void placeAndInitialize();
    void placeCylinder(int index);

    DelayFilter *m_delayFilters;
    atg_scs::FixedPositionConstraint *m_crankConstraints;
    atg_scs::ClutchConstraint *m_crankshaftLinks;
    atg_scs::RotationFrictionConstraint *m_crankshaftFrictionConstraints;
    atg_scs::LineConstraint *m_cylinderWallConstraints;
    atg_scs::LinkConstraint *m_linkConstraints;
    double *m_exhaustFlowStagingBuffer;
    int m_fluidSimulationSteps;
    int m_ignitionEventsThisFrame;
};

#endif
