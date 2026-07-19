#include "SubaruEJ25AtgVideo2Fixture.h"

#include "constants.h"
#include "direct_throttle_linkage.h"
#include "gas_system.h"
#include "units.h"

#include <cmath>
#include <cstdint>

namespace NextcarEngineSim::Phase0 {
namespace {
constexpr double DiskMoment(double mass, double radius) { return 0.5 * mass * radius * radius; }
constexpr double RodMoment(double mass, double length) { return mass * length * length / 12.0; }
// Compile-smoke placeholder only; the exact pinned IR PCM is generated and verified later.
constexpr std::int16_t Phase0PlaceholderImpulse[] = {32767};
constexpr double CircleArea(double diameter) {
    const double radius = diameter / 2.0;
    return constants::pi * radius * radius;
}
} // namespace

SubaruEJ25AtgVideo2Fixture::SubaruEJ25AtgVideo2Fixture() { BuildFunctions(); BuildEngine(); }
SubaruEJ25AtgVideo2Fixture::~SubaruEJ25AtgVideo2Fixture() { Destroy(); }

void SubaruEJ25AtgVideo2Fixture::AddHarmonicCamLobe(
    Function &target, double durationAt50Thou, double gamma, double lift, int steps) {
    const double angle = durationAt50Thou / 4.0;
    const double s = std::pow(2.0 * units::distance(50.0, units::thou) / lift, 1.0 / gamma) - 1.0;
    const double k = std::acos(s) / angle;
    const double extents = constants::pi / k;
    const double step = extents / (steps - 5.0);
    target.initialize(2 * steps - 1, step);
    for (int i = 0; i < steps; ++i) {
        if (i == 0) { target.addSample(0.0, lift); continue; }
        const double x = i * step;
        const double y = x >= extents ? 0.0 : lift * std::pow(0.5 + 0.5 * std::cos(k * x), gamma);
        target.addSample(x, y);
        target.addSample(-x, y);
    }
}

void SubaruEJ25AtgVideo2Fixture::BuildFunctions() {
    Turbulence.initialize(10, 5.0);
    const double turbulence[][2] = {
        {0, 3}, {5, 7.5}, {10, 15}, {15, 24.75}, {20, 37.5},
        {25, 46.875}, {30, 56.25}, {35, 65.625}, {40, 75}, {45, 84.375}};
    for (const auto &point : turbulence) Turbulence.addSample(point[0], point[1]);

    IntakeFlow.initialize(10, units::distance(50, units::thou));
    ExhaustFlow.initialize(10, units::distance(50, units::thou));
    const double intakeCfm[] = {0, 58, 103, 156, 214, 249, 268, 280, 280, 281};
    const double exhaustCfm[] = {0, 37, 72, 113, 160, 196, 222, 235, 245, 246};
    for (int i = 0; i < 10; ++i) {
        const double lift = units::distance(i * 50.0, units::thou);
        IntakeFlow.addSample(lift, GasSystem::k_28inH2O(intakeCfm[i]));
        ExhaustFlow.addSample(lift, GasSystem::k_28inH2O(exhaustCfm[i]));
    }
    AddHarmonicCamLobe(IntakeLobe, units::angle(232, units::deg), 2.0, units::distance(9.78, units::mm), 100);
    AddHarmonicCamLobe(ExhaustLobe, units::angle(236, units::deg), 2.0, units::distance(9.60, units::mm), 100);

    TimingCurve.initialize(5, units::rpm(1000));
    const double rpm[] = {0, 1000, 2000, 3000, 4000};
    const double advance[] = {25, 25, 30, 40, 40};
    for (int i = 0; i < 5; ++i) TimingCurve.addSample(units::rpm(rpm[i]), units::angle(advance[i], units::deg));
}

void SubaruEJ25AtgVideo2Fixture::BuildEngine() {
    auto *throttle = new DirectThrottleLinkage;
    DirectThrottleLinkage::Parameters throttleParams{};
    throttleParams.gamma = 2.0;
    throttle->initialize(throttleParams);

    Engine::Parameters ep{};
    ep.cylinderBanks = 2; ep.cylinderCount = 4; ep.crankshaftCount = 1;
    ep.exhaustSystemCount = 1; ep.intakeCount = 1; ep.name = "Subaru EJ25";
    ep.starterTorque = units::torque(70, units::ft_lb); ep.starterSpeed = units::rpm(500);
    ep.redline = units::rpm(6500); ep.throttle = throttle;
    ep.initialSimulationFrequency = SimulationFrequencyHz;
    ep.initialHighFrequencyGain = 0.01; ep.initialNoise = 1.0; ep.initialJitter = 0.5;
    EngineInstance.initialize(ep);

    Fuel::Parameters fuel{};
    fuel.maxBurningEfficiency = 0.9;
    fuel.turbulenceToFlameSpeedRatio = &Turbulence;
    EngineInstance.getFuel()->initialize(fuel);

    constexpr double stroke = units::distance(79, units::mm);
    constexpr double bore = units::distance(99.5, units::mm);
    constexpr double rodLength = units::distance(5.142, units::inch);
    constexpr double rodMass = units::mass(535, units::g);
    constexpr double compressionHeight = units::distance(1, units::inch);
    constexpr double crankMass = units::mass(9.39, units::kg);
    constexpr double flywheelMass = units::mass(6.8, units::kg);

    Crankshaft::Parameters crank{};
    crank.mass = crankMass; crank.flywheelMass = flywheelMass; crank.crankThrow = stroke / 2;
    crank.rodJournals = 4;
    crank.momentOfInertia = DiskMoment(crankMass, stroke / 2)
        + 2 * DiskMoment(flywheelMass, units::distance(6, units::inch))
        + DiskMoment(units::mass(10, units::kg), units::distance(6, units::cm));
    crank.frictionTorque = units::torque(1, units::ft_lb);
    crank.tdc = units::angle(180, units::deg);
    EngineInstance.getCrankshaft(0)->initialize(crank);
    const double journalAngles[] = {0, 180, 0, 180};
    for (int i = 0; i < 4; ++i)
        EngineInstance.getCrankshaft(0)->setRodJournalAngle(i, units::angle(journalAngles[i], units::deg));

    for (int i = 0; i < 2; ++i) {
        CylinderBank::Parameters bank{};
        bank.crankshaft = EngineInstance.getCrankshaft(0);
        bank.positionX = 0; bank.positionY = 0; bank.angle = units::angle(i == 0 ? 90 : -90, units::deg);
        bank.bore = bore; bank.deckHeight = stroke / 2 + rodLength + compressionHeight;
        bank.displayDepth = 0.4; bank.cylinderCount = 2; bank.index = i;
        EngineInstance.getCylinderBank(i)->initialize(bank);
    }

    Intake::Parameters intake{};
    intake.volume = units::volume(1.325, units::L); intake.CrossSectionArea = units::area(20, units::cm2);
    intake.InputFlowK = GasSystem::k_carb(400); intake.RunnerFlowRate = GasSystem::k_carb(100);
    intake.RunnerLength = units::distance(12, units::inch); intake.IdleFlowK = GasSystem::k_carb(0);
    intake.IdleThrottlePlatePosition = 0.9978; intake.VelocityDecay = 1;
    EngineInstance.getIntake(0)->initialize(intake);

    Impulse.initialize(Phase0PlaceholderImpulse, 1, 44100, 0.01);
    ExhaustSystem::Parameters exhaust{};
    exhaust.length = units::distance(500, units::mm);
    exhaust.collectorCrossSectionArea = CircleArea(units::distance(2.0, units::inch));
    exhaust.outletFlowRate = GasSystem::k_carb(1000); exhaust.primaryTubeLength = units::distance(40, units::inch);
    exhaust.primaryFlowRate = GasSystem::k_carb(400); exhaust.velocityDecay = 1;
    exhaust.audioVolume = 0.5 * 0.02; exhaust.impulseResponse = &Impulse;
    EngineInstance.getExhaustSystem(0)->initialize(exhaust);

    const int journalMap[] = {0, 3, 1, 2};
    const double blowby[] = {0.001, 0.002, 0.001, 0.002};
    for (int i = 0; i < 4; ++i) {
        ConnectingRod::Parameters rod{};
        rod.mass = rodMass; rod.momentOfInertia = RodMoment(rodMass, rodLength);
        rod.centerOfMass = 0; rod.length = rodLength; rod.crankshaft = EngineInstance.getCrankshaft(0);
        rod.piston = EngineInstance.getPiston(i); rod.journal = journalMap[i];
        EngineInstance.getConnectingRod(i)->initialize(rod);

        Piston::Parameters piston{};
        piston.rod = EngineInstance.getConnectingRod(i); piston.bank = EngineInstance.getCylinderBank(i / 2);
        piston.cylinderIndex = i % 2; piston.blowbyFlowCoefficient = GasSystem::k_28inH2O(blowby[i]);
        piston.compressionHeight = compressionHeight; piston.wristPinPosition = 0;
        piston.displacement = 0; piston.mass = units::mass(566, units::g);
        EngineInstance.getPiston(i)->initialize(piston);
    }

    const double intakeCenters[] = {477, 657, 837, 1017};
    const double exhaustCenters[] = {248, 428, 608, 788};
    for (int bankIndex = 0; bankIndex < 2; ++bankIndex) {
        Camshaft::Parameters cam{};
        cam.lobes = 2; cam.advance = 0; cam.crankshaft = EngineInstance.getCrankshaft(0);
        cam.lobeProfile = &IntakeLobe; cam.baseRadius = units::distance(17, units::mm);
        Camshafts[bankIndex * 2].initialize(cam);
        cam.lobeProfile = &ExhaustLobe;
        Camshafts[bankIndex * 2 + 1].initialize(cam);
        for (int local = 0; local < 2; ++local) {
            const int global = bankIndex * 2 + local;
            Camshafts[bankIndex * 2].setLobeCenterline(local, units::angle(intakeCenters[global], units::deg));
            Camshafts[bankIndex * 2 + 1].setLobeCenterline(local, units::angle(exhaustCenters[global], units::deg));
        }
        StandardValvetrain::Parameters vt{};
        vt.intakeCamshaft = &Camshafts[bankIndex * 2];
        vt.exhaustCamshaft = &Camshafts[bankIndex * 2 + 1];
        Valvetrains[bankIndex].initialize(vt);

        CylinderHead::Parameters head{};
        head.bank = EngineInstance.getCylinderBank(bankIndex); head.exhaustPortFlow = &ExhaustFlow;
        head.intakePortFlow = &IntakeFlow; head.valvetrain = &Valvetrains[bankIndex];
        head.combustionChamberVolume = units::volume(67, units::cc);
        head.intakeRunnerVolume = units::volume(149.6, units::cc);
        head.intakeRunnerCrossSectionArea = units::area(1.75 * 1.75, units::inch * units::inch);
        head.exhaustRunnerVolume = units::volume(50, units::cc);
        head.exhaustRunnerCrossSectionArea = units::area(1.25 * 1.25, units::inch * units::inch);
        head.flipDisplay = bankIndex == 0;
        EngineInstance.getHead(bankIndex)->initialize(head);
        EngineInstance.getHead(bankIndex)->setAllIntakes(EngineInstance.getIntake(0));
        EngineInstance.getHead(bankIndex)->setAllExhaustSystems(EngineInstance.getExhaustSystem(0));
    }

    const double primaryLengths[] = {2, 3, 3, 5};
    const double attenuations[] = {0.9, 1.0, 1.1, 0.9};
    for (int i = 0; i < 4; ++i) {
        EngineInstance.getHead(i / 2)->setHeaderPrimaryLength(i % 2, units::distance(primaryLengths[i], units::inch));
        EngineInstance.getHead(i / 2)->setSoundAttenuation(i % 2, attenuations[i]);
        CombustionChamber::Parameters chamber{};
        chamber.piston = EngineInstance.getPiston(i); chamber.head = EngineInstance.getHead(i / 2);
        chamber.fuel = EngineInstance.getFuel(); chamber.meanPistonSpeedToTurbulence = &Turbulence;
        chamber.startingPressure = units::pressure(1, units::atm);
        chamber.startingTemperature = units::celcius(25); chamber.crankcasePressure = units::pressure(1, units::atm);
        chamber.randomSeed = DeterministicSeed + static_cast<std::uint32_t>(i);
        EngineInstance.getChamber(i)->initialize(chamber);
    }

    IgnitionModule::Parameters ignition{};
    ignition.cylinderCount = 4; ignition.crankshaft = EngineInstance.getCrankshaft(0);
    ignition.timingCurve = &TimingCurve; ignition.revLimit = units::rpm(6800); ignition.limiterDuration = 0.16;
    EngineInstance.getIgnitionModule()->initialize(ignition);
    const double fireAngles[] = {0, 180, 360, 540};
    for (int i = 0; i < 4; ++i)
        EngineInstance.getIgnitionModule()->setFiringOrder(i, units::angle(fireAngles[i], units::deg));
    EngineInstance.calculateDisplacement();
}

void SubaruEJ25AtgVideo2Fixture::Destroy() {
    if (IsDestroyed) return;
    EngineInstance.destroy();
    for (auto &camshaft : Camshafts) camshaft.destroy();
    TimingCurve.destroy(); ExhaustLobe.destroy(); IntakeLobe.destroy();
    ExhaustFlow.destroy(); IntakeFlow.destroy(); Turbulence.destroy();
    IsDestroyed = true;
}

} // namespace NextcarEngineSim::Phase0
