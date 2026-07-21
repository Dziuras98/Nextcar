#include "../include/piston_engine_simulator.h"

#include "../include/constants.h"
#include "../include/units.h"

#include <algorithm>
#include <cassert>
#include <cmath>

PistonEngineSimulator::PistonEngineSimulator()
    : m_delayFilters(nullptr), m_crankConstraints(nullptr),
      m_crankshaftLinks(nullptr), m_crankshaftFrictionConstraints(nullptr),
      m_cylinderWallConstraints(nullptr), m_linkConstraints(nullptr),
      m_exhaustFlowStagingBuffer(nullptr), m_fluidSimulationSteps(8),
      m_ignitionEventsThisFrame(0) {
    m_derivativeFilter.m_dt = 1.0;
}

PistonEngineSimulator::~PistonEngineSimulator() {
    assert(m_crankConstraints == nullptr);
    assert(m_crankshaftLinks == nullptr);
    assert(m_crankshaftFrictionConstraints == nullptr);
    assert(m_cylinderWallConstraints == nullptr);
    assert(m_linkConstraints == nullptr);
    assert(m_exhaustFlowStagingBuffer == nullptr);
    assert(m_delayFilters == nullptr);
}

void PistonEngineSimulator::loadSimulation(Engine *engine) {
    Simulator::loadSimulation(engine);
    const int crankCount = engine->getCrankshaftCount();
    const int cylinderCount = engine->getCylinderCount();
    if (crankCount <= 0) return;

    m_crankConstraints = new atg_scs::FixedPositionConstraint[crankCount];
    m_cylinderWallConstraints = new atg_scs::LineConstraint[cylinderCount];
    m_linkConstraints = new atg_scs::LinkConstraint[cylinderCount * 2];
    m_crankshaftFrictionConstraints = new atg_scs::RotationFrictionConstraint[crankCount];
    if (crankCount > 1) m_crankshaftLinks = new atg_scs::ClutchConstraint[crankCount - 1];
    m_delayFilters = new DelayFilter[cylinderCount];

    constexpr double ks = 5000.0;
    constexpr double kd = 10.0;
    for (int i = 0; i < crankCount; ++i) {
        Crankshaft *output = engine->getCrankshaft(0);
        Crankshaft *crank = engine->getCrankshaft(i);
        m_crankConstraints[i].setBody(&crank->m_body);
        m_crankConstraints[i].setWorldPosition(crank->getPosX(), crank->getPosY());
        m_crankConstraints[i].setLocalPosition(0.0, 0.0);
        m_crankConstraints[i].m_kd = kd;
        m_crankConstraints[i].m_ks = ks;
        crank->m_body.p_x = crank->getPosX();
        crank->m_body.p_y = crank->getPosY();
        crank->m_body.theta = 0.0;
        crank->m_body.m = crank->getMass() + crank->getFlywheelMass();
        crank->m_body.I = crank->getMomentOfInertia();
        m_crankshaftFrictionConstraints[i].m_minTorque = -crank->getFrictionTorque();
        m_crankshaftFrictionConstraints[i].m_maxTorque = crank->getFrictionTorque();
        m_crankshaftFrictionConstraints[i].setBody(&crank->m_body);
        m_system->addRigidBody(&crank->m_body);
        m_system->addConstraint(&m_crankConstraints[i]);
        m_system->addConstraint(&m_crankshaftFrictionConstraints[i]);
        if (crank != output) {
            auto *link = &m_crankshaftLinks[i - 1];
            link->setBody1(&output->m_body);
            link->setBody2(&crank->m_body);
            m_system->addConstraint(link);
        }
    }

    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = engine->getPiston(i);
        ConnectingRod *rod = piston->getRod();
        CylinderBank *bank = piston->getCylinderBank();
        const double dx = std::cos(bank->getAngle() + constants::pi / 2);
        const double dy = std::sin(bank->getAngle() + constants::pi / 2);
        auto &wall = m_cylinderWallConstraints[i];
        wall.setBody(&piston->m_body);
        wall.m_dx = dx;
        wall.m_dy = dy;
        wall.m_local_x = 0.0;
        wall.m_local_y = piston->getWristPinLocation();
        wall.m_p0_x = bank->getX();
        wall.m_p0_y = bank->getY();
        wall.m_ks = ks;
        wall.m_kd = kd;
        piston->setCylinderConstraint(&wall);

        auto &wrist = m_linkConstraints[i * 2];
        wrist.setBody1(&rod->m_body);
        wrist.setBody2(&piston->m_body);
        wrist.setLocalPosition1(0.0, rod->getLittleEndLocal());
        wrist.setLocalPosition2(0.0, piston->getWristPinLocation());
        wrist.m_ks = ks;
        wrist.m_kd = kd;

        double journalX = 0.0, journalY = 0.0;
        auto &journal = m_linkConstraints[i * 2 + 1];
        if (rod->getMasterRod() == nullptr) {
            rod->getCrankshaft()->getRodJournalPositionLocal(
                rod->getJournal(), &journalX, &journalY);
            journal.setBody2(&rod->getCrankshaft()->m_body);
        } else {
            rod->getMasterRod()->getRodJournalPositionLocal(
                rod->getJournal(), &journalX, &journalY);
            journal.setBody2(&rod->getMasterRod()->m_body);
        }
        journal.setBody1(&rod->m_body);
        journal.setLocalPosition1(0.0, rod->getBigEndLocal());
        journal.setLocalPosition2(journalX, journalY);
        journal.m_ks = ks;
        journal.m_kd = kd;

        piston->m_body.m = piston->getMass();
        piston->m_body.I = 1.0;
        rod->m_body.m = rod->getMass();
        rod->m_body.I = rod->getMomentOfInertia();
        m_system->addRigidBody(&piston->m_body);
        m_system->addRigidBody(&rod->m_body);
        m_system->addConstraint(&wrist);
        m_system->addConstraint(&journal);
        m_system->addConstraint(&wall);
        m_system->addForceGenerator(engine->getChamber(i));
    }

    m_starterMotor.connectCrankshaft(engine->getOutputCrankshaft());
    m_starterMotor.m_maxTorque = engine->getStarterTorque();
    m_starterMotor.m_rotationSpeed = -engine->getStarterSpeed();
    m_system->addConstraint(&m_starterMotor);

    placeAndInitialize();
    initializeSynthesizer();
}

void PistonEngineSimulator::startFrame(double dt) {
    m_ignitionEventsThisFrame = 0;
    Simulator::startFrame(dt);
}

double PistonEngineSimulator::getAverageOutputSignal() const {
    if (m_engine == nullptr || m_engine->getExhaustSystemCount() == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < m_engine->getExhaustSystemCount(); ++i) {
        sum += m_engine->getExhaustSystem(i)->getSystem()->pressure();
    }
    return sum / m_engine->getExhaustSystemCount();
}

void PistonEngineSimulator::placeAndInitialize() {
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        if (m_engine->getConnectingRod(i)->getRodJournalCount() != 0) placeCylinder(i);
    }
    for (int i = 0; i < cylinderCount; ++i) placeCylinder(i);
    for (int i = 0; i < cylinderCount; ++i) {
        auto *chamber = m_engine->getChamber(i);
        chamber->m_system.initialize(
            units::pressure(1.0, units::atm),
            chamber->getVolume(),
            units::celcius(25.0));
        Piston *piston = chamber->getPiston();
        CylinderHead *head = chamber->getCylinderHead();
        ExhaustSystem *exhaust = head->getExhaustSystem(piston->getCylinderIndex());
        const double length = head->getHeaderPrimaryLength(piston->getCylinderIndex())
            + exhaust->getLength();
        const double delay = length / (343.0 * units::m / units::sec);
        m_delayFilters[i].initialize(delay, getSimulationFrequency());
    }
    m_engine->getIgnitionModule()->reset();
    m_exhaustFlowStagingBuffer = new double[m_engine->getExhaustSystemCount()]{};
}

void PistonEngineSimulator::placeCylinder(int i) {
    ConnectingRod *rod = m_engine->getConnectingRod(i);
    Piston *piston = m_engine->getPiston(i);
    CylinderBank *bank = piston->getCylinderBank();
    double px, py;
    if (rod->getMasterRod() != nullptr) {
        rod->getMasterRod()->getRodJournalPositionGlobal(rod->getJournal(), &px, &py);
    } else {
        rod->getCrankshaft()->getRodJournalPositionGlobal(rod->getJournal(), &px, &py);
    }
    const double a = bank->getDx() * bank->getDx() + bank->getDy() * bank->getDy();
    const double b = -2 * bank->getDx() * (px - bank->getX())
        - 2 * bank->getDy() * (py - bank->getY());
    const double c = (px - bank->getX()) * (px - bank->getX())
        + (py - bank->getY()) * (py - bank->getY())
        - rod->getLength() * rod->getLength();
    const double determinant = b * b - 4 * a * c;
    if (determinant < 0) return;
    const double root = std::sqrt(determinant);
    const double s = std::max((-b + root) / (2 * a), (-b - root) / (2 * a));
    if (s < 0) return;
    const double ex = s * bank->getDx() + bank->getX();
    const double ey = s * bank->getDy() + bank->getY();
    const double cosine = std::clamp((ex - px) / rod->getLength(), -1.0, 1.0);
    const double theta = (ey - py) > 0 ? std::acos(cosine) : 2 * constants::pi - std::acos(cosine);
    rod->m_body.theta = theta - constants::pi / 2;
    double localX, localY;
    rod->m_body.localToWorld(0, rod->getBigEndLocal(), &localX, &localY);
    rod->m_body.p_x += px - localX;
    rod->m_body.p_y += py - localY;
    piston->m_body.p_x = ex;
    piston->m_body.p_y = ey;
    piston->m_body.theta = bank->getAngle() + constants::pi;
}

void PistonEngineSimulator::simulateStep_() {
    const double timestep = getTimestep();
    IgnitionModule *ignition = m_engine->getIgnitionModule();
    ignition->update(timestep);
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        if (ignition->getIgnitionEvent(i)) {
            m_engine->getChamber(i)->ignite();
            ++m_ignitionEventsThisFrame;
        }
        m_engine->getChamber(i)->update(timestep);
        m_engine->getChamber(i)->resetLastTimestepExhaustFlow();
        m_engine->getChamber(i)->resetLastTimestepIntakeFlow();
    }
    const int steps = std::max(1, m_fluidSimulationSteps);
    const double fluidTimestep = timestep / steps;
    for (int step = 0; step < steps; ++step) {
        for (int i = 0; i < m_engine->getExhaustSystemCount(); ++i) {
            m_engine->getExhaustSystem(i)->process(fluidTimestep);
        }
        for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
            m_engine->getIntake(i)->process(fluidTimestep);
            m_engine->getIntake(i)->m_flowRate += m_engine->getIntake(i)->m_flow;
        }
        for (int i = 0; i < cylinderCount; ++i) m_engine->getChamber(i)->flow(fluidTimestep);
    }
    ignition->resetIgnitionEvents();
}

double PistonEngineSimulator::getTotalExhaustFlow() const {
    if (m_engine == nullptr) return 0.0;
    double total = 0.0;
    for (int i = 0; i < m_engine->getCylinderCount(); ++i) {
        total += m_engine->getChamber(i)->getLastTimestepExhaustFlow();
    }
    return total;
}

int PistonEngineSimulator::endFrame(int maximumSynchronousFrames) {
    const int produced = Simulator::endFrame(maximumSynchronousFrames);
    if (m_engine != nullptr) {
        const double frameTimestep = simulationSteps() * getTimestep();
        if (frameTimestep > 0.0) {
            for (int i = 0; i < m_engine->getIntakeCount(); ++i) {
                m_engine->getIntake(i)->m_flowRate /= frameTimestep;
            }
        }
    }
    return produced;
}

void PistonEngineSimulator::destroy() {
    if (m_system != nullptr) m_system->reset();
    delete[] m_crankConstraints;
    delete[] m_crankshaftLinks;
    delete[] m_crankshaftFrictionConstraints;
    delete[] m_cylinderWallConstraints;
    delete[] m_linkConstraints;
    delete[] m_exhaustFlowStagingBuffer;
    delete[] m_delayFilters;
    m_crankConstraints = nullptr;
    m_crankshaftLinks = nullptr;
    m_crankshaftFrictionConstraints = nullptr;
    m_cylinderWallConstraints = nullptr;
    m_linkConstraints = nullptr;
    m_exhaustFlowStagingBuffer = nullptr;
    m_delayFilters = nullptr;
    Simulator::destroy();
}

void PistonEngineSimulator::writeToSynthesizer() {
    const int exhaustCount = m_engine->getExhaustSystemCount();
    for (int i = 0; i < exhaustCount; ++i) m_exhaustFlowStagingBuffer[i] = 0.0;
    const double attenuation = std::min(std::abs(filteredEngineSpeed()), 40.0) / 40.0;
    const double attenuation3 = attenuation * attenuation * attenuation;
    const int cylinderCount = m_engine->getCylinderCount();
    for (int i = 0; i < cylinderCount; ++i) {
        Piston *piston = m_engine->getPiston(i);
        CylinderHead *head = m_engine->getHead(piston->getCylinderBank()->getIndex());
        ExhaustSystem *exhaust = head->getExhaustSystem(piston->getCylinderIndex());
        CombustionChamber *chamber = m_engine->getChamber(i);
        const double length = head->getHeaderPrimaryLength(piston->getCylinderIndex()) + exhaust->getLength();
        const double pulse = attenuation3 * 1600.0 * (
            chamber->m_exhaustRunnerAndPrimary.pressure() - units::pressure(1.0, units::atm)
            + 0.1 * chamber->m_exhaustRunnerAndPrimary.dynamicPressure(1.0, 0.0)
            + 0.1 * chamber->m_exhaustRunnerAndPrimary.dynamicPressure(-1.0, 0.0));
        const double delayed = m_delayFilters[i].fast_f(pulse);
        m_exhaustFlowStagingBuffer[exhaust->getIndex()] +=
            head->getSoundAttenuation(piston->getCylinderIndex())
            * (exhaust->getAudioVolume() * delayed / cylinderCount)
            / (length * length);
    }
    synthesizer().writeInput(m_exhaustFlowStagingBuffer);
}
