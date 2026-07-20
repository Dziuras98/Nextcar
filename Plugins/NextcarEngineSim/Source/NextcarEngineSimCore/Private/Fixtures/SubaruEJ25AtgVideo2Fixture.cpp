#include "SubaruEJ25AtgVideo2Fixture.h"
#include "MinimalMuffling02ImpulseResponse.generated.h"

#include "constants.h"
#include "direct_throttle_linkage.h"
#include "gas_system.h"
#include "units.h"

#include <cmath>
#include <cstdint>

namespace NextcarEngineSim::Phase0 {
SubaruEJ25AtgVideo2Fixture::SubaruEJ25AtgVideo2Fixture()
    : Snapshot(MakeSubaruEJ25AtgVideo2Snapshot()) {
    BuildFunctions();
    BuildEngine();
}
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
        if (i == 0) {
            target.addSample(0.0, lift);
            continue;
        }
        const double x = i * step;
        const double y = x >= extents ? 0.0 : lift * std::pow(0.5 + 0.5 * std::cos(k * x), gamma);
        target.addSample(x, y);
        target.addSample(-x, y);
    }
}

void SubaruEJ25AtgVideo2Fixture::BuildFunctions() {
    FuelTurbulence.initialize(
        static_cast<int>(Snapshot.FuelTurbulenceTable.size()),
        5.0);
    for (const auto &point : Snapshot.FuelTurbulenceTable) {
        FuelTurbulence.addSample(point[0], point[1]);
    }

    MeanPistonSpeedTurbulence.initialize(
        static_cast<int>(Snapshot.MeanPistonSpeedTurbulenceTable.size()),
        1.0);
    for (const auto &point : Snapshot.MeanPistonSpeedTurbulenceTable) {
        MeanPistonSpeedTurbulence.addSample(point[0], point[1]);
    }

    IntakeFlow.initialize(
        static_cast<int>(Snapshot.IntakeFlowCfm.size()),
        units::distance(50, units::thou));
    ExhaustFlow.initialize(
        static_cast<int>(Snapshot.ExhaustFlowCfm.size()),
        units::distance(50, units::thou));
    for (std::size_t i = 0; i < Snapshot.IntakeFlowCfm.size(); ++i) {
        const double lift = units::distance(static_cast<double>(i) * 50.0, units::thou);
        IntakeFlow.addSample(lift, GasSystem::k_28inH2O(Snapshot.IntakeFlowCfm[i]));
        ExhaustFlow.addSample(lift, GasSystem::k_28inH2O(Snapshot.ExhaustFlowCfm[i]));
    }

    AddHarmonicCamLobe(
        IntakeLobe,
        units::angle(Snapshot.IntakeLobe.DurationAt50ThouDeg, units::deg),
        Snapshot.IntakeLobe.Gamma,
        units::distance(Snapshot.IntakeLobe.LiftMm, units::mm),
        Snapshot.IntakeLobe.Steps);
    AddHarmonicCamLobe(
        ExhaustLobe,
        units::angle(Snapshot.ExhaustLobe.DurationAt50ThouDeg, units::deg),
        Snapshot.ExhaustLobe.Gamma,
        units::distance(Snapshot.ExhaustLobe.LiftMm, units::mm),
        Snapshot.ExhaustLobe.Steps);

    TimingCurve.initialize(
        static_cast<int>(Snapshot.TimingRpm.size()),
        units::rpm(1000));
    for (std::size_t i = 0; i < Snapshot.TimingRpm.size(); ++i) {
        TimingCurve.addSample(
            units::rpm(Snapshot.TimingRpm[i]),
            units::angle(Snapshot.TimingAdvanceDeg[i], units::deg));
    }
}

void SubaruEJ25AtgVideo2Fixture::BuildEngine() {
    auto *throttle = new DirectThrottleLinkage;
    DirectThrottleLinkage::Parameters throttleParams{};
    throttleParams.gamma = Snapshot.ThrottleGamma;
    throttle->initialize(throttleParams);

    Engine::Parameters ep{};
    ep.cylinderBanks = Snapshot.ObjectCounts.CylinderBanks;
    ep.cylinderCount = Snapshot.ObjectCounts.Cylinders;
    ep.crankshaftCount = Snapshot.ObjectCounts.Crankshafts;
    ep.exhaustSystemCount = Snapshot.ObjectCounts.ExhaustSystems;
    ep.intakeCount = Snapshot.ObjectCounts.Intakes;
    ep.name = Snapshot.EngineName.data();
    ep.starterTorque = units::torque(Snapshot.StarterTorqueLbFt, units::ft_lb);
    ep.starterSpeed = units::rpm(Snapshot.StarterSpeedRpm);
    ep.redline = units::rpm(Snapshot.RedlineRpm);
    ep.dynoMinSpeed = units::rpm(Snapshot.DynoMinSpeedRpm);
    ep.dynoMaxSpeed = units::rpm(Snapshot.DynoMaxSpeedRpm);
    ep.dynoHoldStep = units::rpm(Snapshot.DynoHoldStepRpm);
    ep.throttle = throttle;
    ep.initialSimulationFrequency = Snapshot.SimulationFrequencyHz;
    ep.initialHighFrequencyGain = Snapshot.HighFrequencyGain;
    ep.initialNoise = Snapshot.Noise;
    ep.initialJitter = Snapshot.Jitter;
    EngineInstance.initialize(ep);

    Fuel::Parameters fuel{};
    fuel.name = Snapshot.FuelName.data();
    fuel.molecularMass = units::mass(Snapshot.FuelMolecularMassG, units::g);
    fuel.energyDensity =
        units::energy(Snapshot.FuelEnergyDensityKjPerG, units::kJ)
        / units::mass(1.0, units::g);
    fuel.density =
        units::mass(Snapshot.FuelDensityKgPerL, units::kg)
        / units::volume(1.0, units::L);
    fuel.molecularAfr = Snapshot.FuelMolecularAfr;
    fuel.burningEfficiencyRandomness = Snapshot.FuelBurningRandomness;
    fuel.lowEfficiencyAttenuation = Snapshot.FuelLowEfficiencyAttenuation;
    fuel.maxBurningEfficiency = Snapshot.FuelMaxBurningEfficiency;
    fuel.maxTurbulenceEffect = Snapshot.FuelMaxTurbulenceEffect;
    fuel.maxDilutionEffect = Snapshot.FuelMaxDilutionEffect;
    fuel.turbulenceToFlameSpeedRatio = &FuelTurbulence;
    EngineInstance.getFuel()->initialize(fuel);

    const double bore = units::distance(Snapshot.BoreMm, units::mm);
    const double rodLength = units::distance(Snapshot.RodLengthInch, units::inch);
    const double rodMass = units::mass(Snapshot.RodMassG, units::g);
    const double compressionHeight = units::distance(Snapshot.CompressionHeightInch, units::inch);
    const double crankMass = units::mass(Snapshot.CrankMassKg, units::kg);
    const double flywheelMass = units::mass(Snapshot.FlywheelMassKg, units::kg);

    Crankshaft::Parameters crank{};
    crank.mass = crankMass;
    crank.flywheelMass = flywheelMass;
    crank.crankThrow = units::distance(Snapshot.CrankThrowMm, units::mm);
    crank.rodJournals = static_cast<int>(Snapshot.JournalAnglesDeg.size());
    crank.momentOfInertia = Snapshot.TotalCrankMomentKgM2;
    crank.frictionTorque = units::torque(Snapshot.CrankFrictionLbFt, units::ft_lb);
    crank.pos_x = Snapshot.CrankPositionX;
    crank.pos_y = Snapshot.CrankPositionY;
    crank.tdc = units::angle(Snapshot.CrankTdcDeg, units::deg);
    EngineInstance.getCrankshaft(0)->initialize(crank);
    for (std::size_t i = 0; i < Snapshot.JournalAnglesDeg.size(); ++i) {
        EngineInstance.getCrankshaft(0)->setRodJournalAngle(
            static_cast<int>(i),
            units::angle(Snapshot.JournalAnglesDeg[i], units::deg));
    }

    for (int i = 0; i < 2; ++i) {
        CylinderBank::Parameters bank{};
        bank.crankshaft = EngineInstance.getCrankshaft(0);
        bank.positionX = 0;
        bank.positionY = 0;
        bank.angle = units::angle(Snapshot.BankAnglesDeg[static_cast<std::size_t>(i)], units::deg);
        bank.bore = bore;
        bank.deckHeight = units::distance(Snapshot.DeckHeightMm, units::mm);
        bank.displayDepth = Snapshot.BankDisplayDepth;
        bank.cylinderCount = 2;
        bank.index = i;
        EngineInstance.getCylinderBank(i)->initialize(bank);
    }

    Intake::Parameters intake{};
    intake.volume = units::volume(Snapshot.IntakePlenumVolumeL, units::L);
    intake.CrossSectionArea = units::area(Snapshot.IntakePlenumAreaCm2, units::cm2);
    intake.InputFlowK = GasSystem::k_carb(Snapshot.IntakeFlowCarb);
    intake.RunnerFlowRate = GasSystem::k_carb(Snapshot.IntakeRunnerFlowCarb);
    intake.RunnerLength = units::distance(Snapshot.IntakeRunnerLengthInch, units::inch);
    intake.IdleFlowK = GasSystem::k_carb(Snapshot.IntakeIdleFlowCarb);
    intake.IdleThrottlePlatePosition = Snapshot.IntakeIdlePlate;
    intake.VelocityDecay = Snapshot.IntakeVelocityDecay;
    intake.MolecularAfr = Snapshot.IntakeMolecularAfr;
    EngineInstance.getIntake(0)->initialize(intake);

    Impulse.initialize(
        NextcarEngineSim::Generated::MinimalMuffling02Pcm,
        static_cast<int>(NextcarEngineSim::Generated::MinimalMuffling02SampleCount),
        static_cast<int>(NextcarEngineSim::Generated::MinimalMuffling02SampleRate),
        Snapshot.ImpulseGain);
    ExhaustSystem::Parameters exhaust{};
    exhaust.length = units::distance(Snapshot.ExhaustLengthMm, units::mm);
    // Script library default: circle_area(2.0 * units.inch).
    exhaust.collectorCrossSectionArea = units::area(
        Snapshot.ExhaustCollectorAreaSquareInch,
        units::inch * units::inch);
    exhaust.outletFlowRate = GasSystem::k_carb(Snapshot.ExhaustOutletFlowCarb);
    exhaust.primaryTubeLength =
        units::distance(Snapshot.ExhaustPrimaryTubeLengthInch, units::inch);
    exhaust.primaryFlowRate = GasSystem::k_carb(Snapshot.ExhaustPrimaryFlowCarb);
    exhaust.velocityDecay = Snapshot.ExhaustVelocityDecay;
    exhaust.audioVolume = Snapshot.ExhaustAudioVolume;
    exhaust.impulseResponse = &Impulse;
    EngineInstance.getExhaustSystem(0)->initialize(exhaust);

    for (int i = 0; i < 4; ++i) {
        ConnectingRod::Parameters rod{};
        rod.mass = rodMass;
        rod.momentOfInertia = Snapshot.RodMomentKgM2;
        rod.centerOfMass = Snapshot.RodCenterOfMass;
        rod.length = rodLength;
        rod.slaveThrow = Snapshot.RodSlaveThrow;
        rod.crankshaft = EngineInstance.getCrankshaft(0);
        rod.piston = EngineInstance.getPiston(i);
        rod.journal = Snapshot.JournalMapping[static_cast<std::size_t>(i)];
        EngineInstance.getConnectingRod(i)->initialize(rod);

        Piston::Parameters piston{};
        piston.rod = EngineInstance.getConnectingRod(i);
        piston.bank = EngineInstance.getCylinderBank(i / 2);
        piston.cylinderIndex = i % 2;
        piston.blowbyFlowCoefficient = GasSystem::k_28inH2O(
            Snapshot.Blowby28InH2O[static_cast<std::size_t>(i)]);
        piston.compressionHeight = compressionHeight;
        piston.wristPinPosition = Snapshot.PistonWristPinPosition;
        piston.displacement = Snapshot.PistonDisplacement;
        piston.mass = units::mass(Snapshot.PistonMassG, units::g);
        EngineInstance.getPiston(i)->initialize(piston);
    }

    for (int bankIndex = 0; bankIndex < 2; ++bankIndex) {
        Camshaft::Parameters cam{};
        cam.lobes = 2;
        cam.advance = units::angle(Snapshot.CamAdvanceDeg, units::deg);
        cam.crankshaft = EngineInstance.getCrankshaft(0);
        cam.lobeProfile = &IntakeLobe;
        cam.baseRadius = units::distance(Snapshot.CamBaseRadiusMm, units::mm);
        Camshafts[static_cast<std::size_t>(bankIndex * 2)].initialize(cam);
        cam.lobeProfile = &ExhaustLobe;
        Camshafts[static_cast<std::size_t>(bankIndex * 2 + 1)].initialize(cam);
        for (int local = 0; local < 2; ++local) {
            const int global = bankIndex * 2 + local;
            Camshafts[static_cast<std::size_t>(bankIndex * 2)].setLobeCenterline(
                local,
                units::angle(Snapshot.IntakeCenterlinesDeg[static_cast<std::size_t>(global)], units::deg));
            Camshafts[static_cast<std::size_t>(bankIndex * 2 + 1)].setLobeCenterline(
                local,
                units::angle(Snapshot.ExhaustCenterlinesDeg[static_cast<std::size_t>(global)], units::deg));
        }

        StandardValvetrain::Parameters vt{};
        vt.intakeCamshaft = &Camshafts[static_cast<std::size_t>(bankIndex * 2)];
        vt.exhaustCamshaft = &Camshafts[static_cast<std::size_t>(bankIndex * 2 + 1)];
        Valvetrains[static_cast<std::size_t>(bankIndex)].initialize(vt);

        CylinderHead::Parameters head{};
        head.bank = EngineInstance.getCylinderBank(bankIndex);
        head.exhaustPortFlow = &ExhaustFlow;
        head.intakePortFlow = &IntakeFlow;
        head.valvetrain = &Valvetrains[static_cast<std::size_t>(bankIndex)];
        head.combustionChamberVolume = units::volume(Snapshot.HeadChamberVolumeCc, units::cc);
        head.intakeRunnerVolume = units::volume(Snapshot.HeadIntakeRunnerVolumeCc, units::cc);
        head.intakeRunnerCrossSectionArea = units::area(
            Snapshot.HeadIntakeRunnerAreaSquareInch,
            units::inch * units::inch);
        head.exhaustRunnerVolume = units::volume(Snapshot.HeadExhaustRunnerVolumeCc, units::cc);
        head.exhaustRunnerCrossSectionArea = units::area(
            Snapshot.HeadExhaustRunnerAreaSquareInch,
            units::inch * units::inch);
        head.flipDisplay = Snapshot.HeadFlipDisplay[static_cast<std::size_t>(bankIndex)];
        EngineInstance.getHead(bankIndex)->initialize(head);
        EngineInstance.getHead(bankIndex)->setAllIntakes(EngineInstance.getIntake(0));
        EngineInstance.getHead(bankIndex)->setAllExhaustSystems(EngineInstance.getExhaustSystem(0));
    }

    for (int i = 0; i < 4; ++i) {
        EngineInstance.getHead(i / 2)->setHeaderPrimaryLength(
            i % 2,
            units::distance(Snapshot.PrimaryLengthsInch[static_cast<std::size_t>(i)], units::inch));
        EngineInstance.getHead(i / 2)->setSoundAttenuation(
            i % 2,
            Snapshot.SoundAttenuations[static_cast<std::size_t>(i)]);

        CombustionChamber::Parameters chamber{};
        chamber.piston = EngineInstance.getPiston(i);
        chamber.head = EngineInstance.getHead(i / 2);
        chamber.fuel = EngineInstance.getFuel();
        chamber.meanPistonSpeedToTurbulence = &MeanPistonSpeedTurbulence;
        chamber.startingPressure =
            units::pressure(Snapshot.ChamberStartingPressureAtm, units::atm);
        chamber.startingTemperature =
            units::celcius(Snapshot.ChamberStartingTemperatureC);
        chamber.crankcasePressure =
            units::pressure(Snapshot.ChamberCrankcasePressureAtm, units::atm);
        chamber.randomSeed = Snapshot.DeterministicSeeds[static_cast<std::size_t>(i)];
        EngineInstance.getChamber(i)->initialize(chamber);
    }

    IgnitionModule::Parameters ignition{};
    ignition.cylinderCount = 4;
    ignition.crankshaft = EngineInstance.getCrankshaft(0);
    ignition.timingCurve = &TimingCurve;
    ignition.revLimit = units::rpm(Snapshot.IgnitionRevLimitRpm);
    ignition.limiterDuration = Snapshot.IgnitionLimiterDurationSeconds;
    EngineInstance.getIgnitionModule()->initialize(ignition);
    for (std::size_t i = 0; i < Snapshot.FiringOrderDeg.size(); ++i) {
        EngineInstance.getIgnitionModule()->setFiringOrder(
            Snapshot.IgnitionCylinderMapping[i],
            units::angle(Snapshot.FiringOrderDeg[i], units::deg));
    }
    EngineInstance.calculateDisplacement();
}

void SubaruEJ25AtgVideo2Fixture::Destroy() {
    if (IsDestroyed) return;
    EngineInstance.destroy();
    for (auto &camshaft : Camshafts) camshaft.destroy();
    TimingCurve.destroy();
    ExhaustLobe.destroy();
    IntakeLobe.destroy();
    ExhaustFlow.destroy();
    IntakeFlow.destroy();
    MeanPistonSpeedTurbulence.destroy();
    FuelTurbulence.destroy();
    IsDestroyed = true;
}

} // namespace NextcarEngineSim::Phase0
