#include "SubaruEJ25AtgVideo2Fixture.h"

#include "constants.h"
#include "direct_throttle_linkage.h"
#include "gas_system.h"
#include "units.h"

#include <cmath>

namespace NextcarEngineSim::Phase0 {
namespace {
constexpr double DiskMoment(double Mass, double Radius) { return 0.5 * Mass * Radius * Radius; }
constexpr double RodMoment(double Mass, double Length) { return Mass * Length * Length / 12.0; }
}

SubaruEJ25AtgVideo2Fixture::SubaruEJ25AtgVideo2Fixture() { BuildFunctions(); BuildEngine(); }
SubaruEJ25AtgVideo2Fixture::~SubaruEJ25AtgVideo2Fixture() { Destroy(); }

void SubaruEJ25AtgVideo2Fixture::AddHarmonicCamLobe(Function &Target, double DurationAt50Thou, double Gamma, double Lift, int Steps) {
    const double Angle = DurationAt50Thou / 4.0;
    const double S = std::pow(2.0 * units::distance(50.0, units::thou) / Lift, 1.0 / Gamma) - 1.0;
    const double K = std::acos(S) / Angle;
    const double Extents = constants::pi / K;
    const double Step = Extents / (Steps - 5.0);
    Target.initialize(2 * Steps - 1, Step);
    for (int Index = 0; Index < Steps; ++Index) {
        if (Index == 0) {
            Target.addSample(0.0, Lift);
        } else {
            const double X = Index * Step;
            const double SampleLift = (X >= Extents)
                ? 0.0
                : Lift * std::pow(0.5 + 0.5 * std::cos(K * X), Gamma);
            Target.addSample(X, SampleLift);
            Target.addSample(-X, SampleLift);
        }
    }
}

void SubaruEJ25AtgVideo2Fixture::BuildFunctions() {
    Turbulence.initialize(10, 5.0);
    const double TurbulencePoints[][2] = {{0,3},{5,7.5},{10,15},{15,24.75},{20,37.5},{25,46.875},{30,56.25},{35,65.625},{40,75},{45,84.375}};
    for (const auto &Point : TurbulencePoints) Turbulence.addSample(Point[0], Point[1]);

    IntakeFlow.initialize(10, units::distance(50, units::thou));
    ExhaustFlow.initialize(10, units::distance(50, units::thou));
    const double IntakeCfm[] = {0,58,103,156,214,249,268,280,280,281};
    const double ExhaustCfm[] = {0,37,72,113,160,196,222,235,245,246};
    for (int Index = 0; Index < 10; ++Index) {
        const double ValveLift = units::distance(Index * 50.0, units::thou);
        IntakeFlow.addSample(ValveLift, GasSystem::k_28inH2O(IntakeCfm[Index]));
        ExhaustFlow.addSample(ValveLift, GasSystem::k_28inH2O(ExhaustCfm[Index]));
    }

    AddHarmonicCamLobe(IntakeLobe, units::angle(232, units::deg), 2.0, units::distance(9.78, units::mm), 100);
    AddHarmonicCamLobe(ExhaustLobe, units::angle(236, units::deg), 2.0, units::distance(9.60, units::mm), 100);

    TimingCurve.initialize(5, units::rpm(1000));
    TimingCurve.addSample(units::rpm(0), units::angle(25, units::deg));
    TimingCurve.addSample(units::rpm(1000), units::angle(25, units::deg));
    TimingCurve.addSample(units::rpm(2000), units::angle(30, units::deg));
    TimingCurve.addSample(units::rpm(3000), units::angle(40, units::deg));
    TimingCurve.addSample(units::rpm(4000), units::angle(40, units::deg));
}

void SubaruEJ25AtgVideo2Fixture::BuildEngine() {
    auto *Throttle = new DirectThrottleLinkage;
    DirectThrottleLinkage::Parameters ThrottleParams{};
    ThrottleParams.gamma = 2.0;
    Throttle->initialize(ThrottleParams);

    Engine::Parameters EngineParams{};
    EngineParams.cylinderBanks = 2;
    EngineParams.cylinderCount = 4;
    EngineParams.crankshaftCount = 1;
    EngineParams.exhaustSystemCount = 1;
    EngineParams.intakeCount = 1;
    EngineParams.name = "Subaru EJ25";
    EngineParams.starterTorque = units::torque(70, units::ft_lb);
    EngineParams.starterSpeed = units::rpm(500);
    EngineParams.redline = units::rpm(6500);
    EngineParams.throttle = Throttle;
    EngineParams.initialSimulationFrequency = SimulationFrequencyHz;
    EngineParams.initialHighFrequencyGain = 0.01;
    EngineParams.initialNoise = 1.0;
    EngineParams.initialJitter = 0.5;
    EngineInstance.initialize(EngineParams);

    Fuel::Parameters FuelParams{};
    FuelParams.maxBurningEfficiency = 0.9;
    FuelParams.turbulenceToFlameSpeedRatio = &Turbulence;
    EngineInstance.getFuel()->initialize(FuelParams);

    constexpr double Stroke = units::distance(79, units::mm);
    constexpr double Bore = units::distance(99.5, units::mm);
    constexpr double RodLength = units::distance(5.142, units::inch);
    constexpr double RodMass = units::mass(535, units::g);
    constexpr double CompressionHeight = units::distance(1, units::inch);
    constexpr double CrankMass = units::mass(9.39, units::kg);
    constexpr double FlywheelMass = units::mass(6.8, units::kg);

    Crankshaft::Parameters CrankParams{};
    CrankParams.mass = CrankMass;
    CrankParams.flywheelMass = FlywheelMass;
    CrankParams.crankThrow = Stroke / 2;
    CrankParams.rodJournals = 4;
    CrankParams.momentOfInertia = DiskMoment(CrankMass, Stroke / 2)
        + 2 * DiskMoment(FlywheelMass, units::distance(6, units::inch))
        + DiskMoment(units::mass(10, units::kg), units::distance(6, units::cm));
    CrankParams.frictionTorque = units::torque(1, units::ft_lb);
    CrankParams.tdc = units::angle(180, units::deg);
    EngineInstance.getCrankshaft(0)->initialize(CrankParams);
    const double JournalAngles[] = {0,180,0,180};
    for (int Index = 0; Index < 4; ++Index) {
        EngineInstance.getCrankshaft(0)->setRodJournalAngle(Index, units::angle(JournalAngles[Index], units::deg));
    }

    for (int BankIndex = 0; BankIndex < 2; ++BankIndex) {
        CylinderBank::Parameters BankParams{};
        BankParams.crankshaft = EngineInstance.getCrankshaft(0);
        BankParams.positionX = 0;
        BankParams.positionY = 0;
        BankParams.angle = units::angle(BankIndex == 0 ? 90 : -90, units::deg);
        BankParams.bore = Bore;
        BankParams.deckHeight = Stroke / 2 + RodLength + CompressionHeight;
        BankParams.displayDepth = 0.4;
        BankParams.cylinderCount = 2;
        BankParams.index = BankIndex;
        EngineInstance.getCylinderBank(BankIndex)->initialize(BankParams);
    }

    Intake::Parameters IntakeParams{};
    IntakeParams.volume = units::volume(1.325, units::L);
    IntakeParams.CrossSectionArea = units::area(20, units::cm2);
    IntakeParams.InputFlowK = GasSystem::k_carb(400);
    IntakeParams.RunnerFlowRate = GasSystem::k_carb(100);
    IntakeParams.RunnerLength = units::distance(12, units::inch);
    IntakeParams.IdleFlowK = GasSystem::k_carb(0);
    IntakeParams.IdleThrottlePlatePosition = 0.9978;
    IntakeParams.VelocityDecay = 1;
    EngineInstance.getIntake(0)->initialize(IntakeParams);

    Impulse.initialize("es/sound-library/new/minimal_muffling_02.wav", 0.01);
    ExhaustSystem::Parameters ExhaustParams{};
    ExhaustParams.length = units::distance(500, units::mm);
    ExhaustParams.collectorCrossSectionArea = units::area(1.25 * 1.25, units::inch * units::inch);
    ExhaustParams.outletFlowRate = GasSystem::k_carb(1000);
    ExhaustParams.primaryTubeLength = units::distance(40, units::inch);
    ExhaustParams.primaryFlowRate = GasSystem::k_carb(400);
    ExhaustParams.velocityDecay = 1;
    ExhaustParams.audioVolume = 0.5 * 0.02;
    ExhaustParams.impulseResponse = &Impulse;
    EngineInstance.getExhaustSystem(0)->initialize(ExhaustParams);

    const int JournalMap[] = {0,3,1,2};
    const double BlowbyCfm[] = {0.001,0.002,0.001,0.002};
    for (int Index = 0; Index < 4; ++Index) {
        const int Bank = Index / 2;
        const int Local = Index % 2;
        ConnectingRod::Parameters RodParams{};
        RodParams.mass = RodMass;
        RodParams.momentOfInertia = RodMoment(RodMass, RodLength);
        RodParams.centerOfMass = 0;
        RodParams.length = RodLength;
        RodParams.crankshaft = EngineInstance.getCrankshaft(0);
        RodParams.piston = EngineInstance.getPiston(Index);
        RodParams.journal = JournalMap[Index];
        EngineInstance.getConnectingRod(Index)->initialize(RodParams);

        Piston::Parameters PistonParams{};
        PistonParams.Rod = EngineInstance.getConnectingRod(Index);
        PistonParams.Bank = EngineInstance.getCylinderBank(Bank);
        PistonParams.CylinderIndex = Local;
        PistonParams.BlowbyFlowCoefficient = GasSystem::k_28inH2O(BlowbyCfm[Index]);
        PistonParams.CompressionHeight = CompressionHeight;
        PistonParams.WristPinPosition = 0;
        PistonParams.Displacement = 0;
        PistonParams.mass = units::mass(414 + 152, units::g);
        EngineInstance.getPiston(Index)->initialize(PistonParams);
    }

    const double IntakeCenters[] = {360+117, 360+117+180, 360+117+360, 360+117+540};
    const double ExhaustCenters[] = {360-112, 360-112+180, 360-112+360, 360-112+540};
    for (int Bank = 0; Bank < 2; ++Bank) {
        Camshaft::Parameters IntakeCamParams{};
        IntakeCamParams.lobes = 2;
        IntakeCamParams.advance = 0;
        IntakeCamParams.crankshaft = EngineInstance.getCrankshaft(0);
        IntakeCamParams.lobeProfile = &IntakeLobe;
        IntakeCamParams.baseRadius = units::distance(17, units::mm);
        Camshafts[Bank * 2].initialize(IntakeCamParams);
        Camshaft::Parameters ExhaustCamParams = IntakeCamParams;
        ExhaustCamParams.lobeProfile = &ExhaustLobe;
        Camshafts[Bank * 2 + 1].initialize(ExhaustCamParams);

        for (int Local = 0; Local < 2; ++Local) {
            const int Global = Bank * 2 + Local;
            Camshafts[Bank * 2].setLobeCenterline(Local, units::angle(IntakeCenters[Global], units::deg));
            Camshafts[Bank * 2 + 1].setLobeCenterline(Local, units::angle(ExhaustCenters[Global], units::deg));
        }

        StandardValvetrain::Parameters ValvetrainParams{};
        ValvetrainParams.intakeCamshaft = &Camshafts[Bank * 2];
        ValvetrainParams.exhaustCamshaft = &Camshafts[Bank * 2 + 1];
        Valvetrains[Bank].initialize(ValvetrainParams);

        CylinderHead::Parameters HeadParams{};
        HeadParams.Bank = EngineInstance.getCylinderBank(Bank);
        HeadParams.ExhaustPortFlow = &ExhaustFlow;
        HeadParams.IntakePortFlow = &IntakeFlow;
        HeadParams.valvetrain = &Valvetrains[Bank];
        HeadParams.CombustionChamberVolume = units::volume(67, units::cc);
        HeadParams.IntakeRunnerVolume = units::volume(149.6, units::cc);
        HeadParams.IntakeRunnerCrossSectionArea = units::area(1.75 * 1.75, units::inch * units::inch);
        HeadParams.ExhaustRunnerVolume = units::volume(50, units::cc);
        HeadParams.ExhaustRunnerCrossSectionArea = units::area(1.25 * 1.25, units::inch * units::inch);
        HeadParams.FlipDisplay = Bank == 0;
        EngineInstance.getHead(Bank)->initialize(HeadParams);
        EngineInstance.getHead(Bank)->setAllIntakes(EngineInstance.getIntake(0));
        EngineInstance.getHead(Bank)->setAllExhaustSystems(EngineInstance.getExhaustSystem(0));
    }

    EngineInstance.getHead(0)->setHeaderPrimaryLength(0, units::distance(2, units::inch));
    EngineInstance.getHead(0)->setHeaderPrimaryLength(1, units::distance(3, units::inch));
    EngineInstance.getHead(1)->setHeaderPrimaryLength(0, units::distance(3, units::inch));
    EngineInstance.getHead(1)->setHeaderPrimaryLength(1, units::distance(5, units::inch));
    EngineInstance.getHead(0)->setSoundAttenuation(0, 0.9);
    EngineInstance.getHead(0)->setSoundAttenuation(1, 1.0);
    EngineInstance.getHead(1)->setSoundAttenuation(0, 1.1);
    EngineInstance.getHead(1)->setSoundAttenuation(1, 0.9);

    for (int Index = 0; Index < 4; ++Index) {
        CombustionChamber::Parameters ChamberParams{};
        ChamberParams.piston = EngineInstance.getPiston(Index);
        ChamberParams.head = EngineInstance.getHead(Index / 2);
        ChamberParams.fuel = EngineInstance.getFuel();
        ChamberParams.MeanPistonSpeedToTurbulence = &Turbulence;
        ChamberParams.StartingPressure = units::pressure(1, units::atm);
        ChamberParams.StartingTemperature = units::celcius(25);
        ChamberParams.CrankcasePressure = units::pressure(1, units::atm);
        ChamberParams.RandomSeed = DeterministicSeed + Index;
        EngineInstance.getChamber(Index)->initialize(ChamberParams);
    }

    IgnitionModule::Parameters IgnitionParams{};
    IgnitionParams.cylinderCount = 4;
    IgnitionParams.crankshaft = EngineInstance.getCrankshaft(0);
    IgnitionParams.timingCurve = &TimingCurve;
    IgnitionParams.revLimit = units::rpm(6800);
    IgnitionParams.limiterDuration = 0.16;
    EngineInstance.getIgnitionModule()->initialize(IgnitionParams);
    const double FireAngles[] = {0,180,360,540};
    for (int Index = 0; Index < 4; ++Index) {
        EngineInstance.getIgnitionModule()->setFiringOrder(Index, units::angle(FireAngles[Index], units::deg));
    }
    EngineInstance.calculateDisplacement();
}

void SubaruEJ25AtgVideo2Fixture::Destroy() {
    if (IsDestroyed) return;
    EngineInstance.destroy();
    for (auto &CamshaftInstance : Camshafts) CamshaftInstance.destroy();
    TimingCurve.destroy();
    ExhaustLobe.destroy();
    IntakeLobe.destroy();
    ExhaustFlow.destroy();
    IntakeFlow.destroy();
    Turbulence.destroy();
    IsDestroyed = true;
}

}
