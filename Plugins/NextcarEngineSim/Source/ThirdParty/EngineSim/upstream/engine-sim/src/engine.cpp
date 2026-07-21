#include "../include/engine.h"

#include "../include/constants.h"
#include "../include/units.h"
#include "../include/fuel.h"
#include "../include/piston_engine_simulator.h"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>

Engine::Engine()
    : m_crankshafts(nullptr), m_crankshaftCount(0), m_cylinderBanks(nullptr),
      m_heads(nullptr), m_cylinderBankCount(0), m_pistons(nullptr),
      m_connectingRods(nullptr), m_combustionChambers(nullptr),
      m_cylinderCount(0), m_starterTorque(0), m_starterSpeed(0), m_redline(0),
      m_dynoMinSpeed(0), m_dynoMaxSpeed(0), m_dynoHoldStep(0),
      m_initialSimulationFrequency(10000.0), m_initialHighFrequencyGain(0.01),
      m_initialNoise(1.0), m_initialJitter(0.5), m_exhaustSystems(nullptr),
      m_exhaustSystemCount(0), m_intakes(nullptr), m_intakeCount(0),
      m_throttle(nullptr), m_throttleValue(0.0), m_displacement(0.0) {}

Engine::~Engine() {
    assert(m_crankshafts == nullptr);
    assert(m_cylinderBanks == nullptr);
    assert(m_pistons == nullptr);
    assert(m_connectingRods == nullptr);
    assert(m_heads == nullptr);
    assert(m_exhaustSystems == nullptr);
    assert(m_intakes == nullptr);
    assert(m_combustionChambers == nullptr);
}

void Engine::initialize(const Parameters &params) {
    m_crankshaftCount = params.crankshaftCount;
    m_cylinderCount = params.cylinderCount;
    m_cylinderBankCount = params.cylinderBanks;
    m_exhaustSystemCount = params.exhaustSystemCount;
    m_intakeCount = params.intakeCount;
    m_starterTorque = params.starterTorque;
    m_starterSpeed = params.starterSpeed;
    m_dynoMinSpeed = params.dynoMinSpeed;
    m_dynoMaxSpeed = params.dynoMaxSpeed;
    m_dynoHoldStep = params.dynoHoldStep;
    m_redline = params.redline;
    m_name = params.name;
    m_throttle = params.throttle;
    m_initialHighFrequencyGain = params.initialHighFrequencyGain;
    m_initialSimulationFrequency = params.initialSimulationFrequency;
    m_initialJitter = params.initialJitter;
    m_initialNoise = params.initialNoise;

    m_crankshafts = new Crankshaft[m_crankshaftCount];
    m_cylinderBanks = new CylinderBank[m_cylinderBankCount];
    m_heads = new CylinderHead[m_cylinderBankCount];
    m_pistons = new Piston[m_cylinderCount];
    m_connectingRods = new ConnectingRod[m_cylinderCount];
    m_exhaustSystems = new ExhaustSystem[m_exhaustSystemCount];
    m_intakes = new Intake[m_intakeCount];
    m_combustionChambers = new CombustionChamber[m_cylinderCount];
    for (int i = 0; i < m_exhaustSystemCount; ++i) m_exhaustSystems[i].m_index = i;
    for (int i = 0; i < m_cylinderCount; ++i) m_combustionChambers[i].setEngine(this);
}

void Engine::destroy() {
    if (m_crankshafts) for (int i = 0; i < m_crankshaftCount; ++i) m_crankshafts[i].destroy();
    if (m_pistons && m_connectingRods && m_combustionChambers) {
        for (int i = 0; i < m_cylinderCount; ++i) {
            m_pistons[i].destroy();
            m_connectingRods[i].destroy();
            m_combustionChambers[i].destroy();
        }
    }
    if (m_heads) for (int i = 0; i < m_cylinderBankCount; ++i) m_heads[i].destroy();
    if (m_exhaustSystems) for (int i = 0; i < m_exhaustSystemCount; ++i) m_exhaustSystems[i].destroy();
    if (m_intakes) for (int i = 0; i < m_intakeCount; ++i) m_intakes[i].destroy();
    m_ignitionModule.destroy();
    delete m_throttle;
    delete[] m_crankshafts;
    delete[] m_cylinderBanks;
    delete[] m_heads;
    delete[] m_pistons;
    delete[] m_connectingRods;
    delete[] m_exhaustSystems;
    delete[] m_intakes;
    delete[] m_combustionChambers;
    m_crankshafts = nullptr;
    m_cylinderBanks = nullptr;
    m_heads = nullptr;
    m_pistons = nullptr;
    m_connectingRods = nullptr;
    m_exhaustSystems = nullptr;
    m_intakes = nullptr;
    m_combustionChambers = nullptr;
    m_throttle = nullptr;
}

Crankshaft *Engine::getOutputCrankshaft() const { return m_crankshaftCount ? &m_crankshafts[0] : nullptr; }
void Engine::setSpeedControl(double s) { if (m_throttle) m_throttle->setSpeedControl(s); }
double Engine::getSpeedControl() { return m_throttle ? m_throttle->getSpeedControl() : 0.0; }
void Engine::setThrottle(double throttle) {
    for (int i = 0; i < m_intakeCount; ++i) m_intakes[i].m_throttle = throttle;
    m_throttleValue = throttle;
}
double Engine::getThrottle() const { return m_throttleValue; }
double Engine::getThrottlePlateAngle() const {
    return m_intakeCount ? (1 - m_intakes[0].getThrottlePlatePosition()) * constants::pi / 2 : 0.0;
}

namespace {
bool PlaceRod(const ConnectingRod &rod, const CylinderBank &bank, double crankshaftAngle,
              double *p_x, double *p_y, double *theta, double *s) {
    double p_x_0, p_y_0, l_x, l_y, theta_0;
    if (rod.getMasterRod() != nullptr) {
        double parentS;
        if (!PlaceRod(*rod.getMasterRod(), *rod.getMasterRod()->getPiston()->getCylinderBank(),
                      crankshaftAngle, &p_x_0, &p_y_0, &theta_0, &parentS)) return false;
        rod.getMasterRod()->getRodJournalPositionLocal(rod.getPiston()->getCylinderIndex(), &l_x, &l_y);
    } else {
        theta_0 = crankshaftAngle;
        p_x_0 = rod.getCrankshaft()->getPosX();
        p_y_0 = rod.getCrankshaft()->getPosY();
        rod.getCrankshaft()->getRodJournalPositionLocal(rod.getPiston()->getCylinderIndex(), &l_x, &l_y);
    }
    const double dx = std::cos(theta_0), dy = std::sin(theta_0);
    *p_x = p_x_0 + (dx * l_x - dy * l_y);
    *p_y = p_y_0 + (dy * l_x + dx * l_y);
    const double a = bank.getDx() * bank.getDx() + bank.getDy() * bank.getDy();
    const double b = -2 * bank.getDx() * ((*p_x) - bank.getX()) - 2 * bank.getDy() * ((*p_y) - bank.getY());
    const double c = ((*p_x) - bank.getX()) * ((*p_x) - bank.getX())
        + ((*p_y) - bank.getY()) * ((*p_y) - bank.getY()) - rod.getLength() * rod.getLength();
    const double det = b * b - 4 * a * c;
    if (det < 0) return false;
    const double root = std::sqrt(det);
    *s = std::max((-b + root) / (2 * a), (-b - root) / (2 * a));
    if (*s < 0) return false;
    const double tx = bank.getX() + bank.getDx() * (*s) - *p_x;
    const double ty = bank.getY() + bank.getDy() * (*s) - *p_y;
    *theta = ty > 0 ? std::acos(tx) : -std::acos(tx);
    return true;
}
}

void Engine::calculateDisplacement() {
    constexpr int Resolution = 1000;
    double *minimum = new double[m_cylinderCount];
    double *maximum = new double[m_cylinderCount];
    for (int i = 0; i < m_cylinderCount; ++i) { minimum[i] = DBL_MAX; maximum[i] = -DBL_MAX; }
    for (int j = 0; j < Resolution; ++j) {
        const double angle = 2 * (j / static_cast<double>(Resolution)) * constants::pi;
        for (int i = 0; i < m_cylinderCount; ++i) {
            const Piston &piston = m_pistons[i];
            const CylinderBank &bank = *piston.getCylinderBank();
            const ConnectingRod &rod = *piston.getRod();
            double x, y, theta, s;
            if (!PlaceRod(rod, bank, angle, &x, &y, &theta, &s)) continue;
            minimum[i] = std::min(minimum[i], s);
            maximum[i] = std::max(maximum[i], s);
        }
    }
    m_displacement = 0;
    for (int i = 0; i < m_cylinderCount; ++i) {
        if (minimum[i] < maximum[i]) {
            const double radius = m_pistons[i].getCylinderBank()->getBore() / 2.0;
            m_displacement += constants::pi * radius * radius * (maximum[i] - minimum[i]);
        }
    }
    delete[] minimum;
    delete[] maximum;
}

double Engine::getIntakeFlowRate() const {
    double total = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        total += m_intakes[i].m_flowRate;
    }
    return total;
}

void Engine::update(double dt) {
    if (m_throttle != nullptr) m_throttle->update(dt, this);
}

double Engine::getManifoldPressure() const {
    if (m_intakeCount == 0) return 0.0;
    double total = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        total += m_intakes[i].m_system.pressure();
    }
    return total / m_intakeCount;
}

double Engine::getIntakeAfr() const {
    double oxygen = 0.0;
    double fuel = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        oxygen += m_intakes[i].m_system.n_o2();
        fuel += m_intakes[i].m_system.n_fuel();
    }
    constexpr double OctaneMolecularMass = units::mass(114.23, units::g);
    constexpr double OxygenMolecularMass = units::mass(31.9988, units::g);
    return fuel == 0.0
        ? 0.0
        : (OxygenMolecularMass * oxygen / 0.21) / (fuel * OctaneMolecularMass);
}

double Engine::getExhaustO2() const {
    double inert = 0.0;
    double oxygen = 0.0;
    double fuel = 0.0;
    for (int i = 0; i < m_exhaustSystemCount; ++i) {
        inert += m_exhaustSystems[i].m_system.n_inert();
        oxygen += m_exhaustSystems[i].m_system.n_o2();
        fuel += m_exhaustSystems[i].m_system.n_fuel();
    }
    constexpr double OctaneMolecularMass = units::mass(114.23, units::g);
    constexpr double OxygenMolecularMass = units::mass(31.9988, units::g);
    constexpr double NitrogenMolecularMass = units::mass(28.014, units::g);
    return fuel == 0.0
        ? 0.0
        : (OxygenMolecularMass * oxygen)
            / (fuel * OctaneMolecularMass
                + inert * NitrogenMolecularMass
                + oxygen * OxygenMolecularMass);
}

void Engine::resetFuelConsumption() {
    for (int i = 0; i < m_intakeCount; ++i) {
        m_intakes[i].m_totalFuelInjected = 0.0;
    }
}

double Engine::getTotalFuelMassConsumed() const {
    double molecules = 0.0;
    for (int i = 0; i < m_intakeCount; ++i) {
        molecules += m_intakes[i].m_totalFuelInjected;
    }
    return molecules * m_fuel.getMolecularMass();
}

double Engine::getTotalVolumeFuelConsumed() const {
    return m_fuel.getDensity() != 0.0
        ? getTotalFuelMassConsumed() / m_fuel.getDensity()
        : 0.0;
}

int Engine::getMaxDepth() const {
    int result = 0;
    for (int i = 0; i < m_crankshaftCount; ++i) {
        result = std::max(result, m_crankshafts[i].getRodJournalCount());
    }
    return result;
}

Simulator *Engine::createSimulator() {
    auto *simulator = new PistonEngineSimulator;
    simulator->initialize();
    simulator->loadSimulation(this);
    simulator->setFluidSimulationSteps(8);
    return simulator;
}

double Engine::getRpm() const { return m_crankshaftCount ? std::abs(units::toRpm(getCrankshaft(0)->m_body.v_theta)) : 0; }
double Engine::getSpeed() const { return m_crankshaftCount ? std::abs(getCrankshaft(0)->m_body.v_theta) : 0; }
bool Engine::isSpinningCw() const { return getOutputCrankshaft() && getOutputCrankshaft()->m_body.v_theta <= 0; }
