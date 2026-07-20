#include "SubaruEJ25AtgVideo2Fixture.h"
#include "MinimalMuffling02ImpulseResponse.generated.h"

#include "combustion_chamber.h"
#include "constants.h"
#include "force_generator.h"
#include "gauss_seidel_sle_solver.h"
#include "optimized_nsv_rigid_body_system.h"
#include "piston_engine_simulator.h"
#include "ring_buffer.h"
#include "synthesizer.h"
#include "units.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireNear(
    double actual,
    double expected,
    double absoluteTolerance,
    double relativeTolerance,
    const std::string &message) {
    const double allowed =
        absoluteTolerance + relativeTolerance * std::max(std::abs(actual), std::abs(expected));
    Require(std::abs(actual - expected) <= allowed, message);
}

template <typename T, std::size_t N>
void RequireExact(
    const std::array<T, N> &actual,
    const std::array<T, N> &expected,
    const std::string &message) {
    Require(actual == expected, message);
}

template <std::size_t N>
void RequireNearArray(
    const std::array<double, N> &actual,
    const std::array<double, N> &expected,
    double absoluteTolerance,
    double relativeTolerance,
    const std::string &message) {
    for (std::size_t i = 0; i < N; ++i) {
        RequireNear(actual[i], expected[i], absoluteTolerance, relativeTolerance, message);
    }
}

void TestRingBuffer() {
    RingBuffer<int> buffer;
    buffer.initialize(4);
    Require(buffer.empty(), "new ring buffer must be empty");
    Require(buffer.size() == 0, "new ring buffer size");
    Require(buffer.freeSpace() == 4, "new ring buffer free space");

    Require(buffer.write(10), "partial write 10");
    Require(buffer.write(20), "partial write 20");
    Require(buffer.size() == 2 && !buffer.empty() && !buffer.full(), "partial occupancy");
    Require(buffer.read(0) == 10 && buffer.read(1) == 20, "partial read order");

    Require(buffer.write(30), "write 30");
    Require(buffer.write(40), "write 40");
    Require(buffer.full(), "exactly full must report full");
    Require(!buffer.empty(), "exactly full must not report empty");
    Require(buffer.size() == 4 && buffer.freeSpace() == 0, "full occupancy");
    Require(!buffer.write(50), "write after full must be rejected");

    std::array<int, 2> first{};
    buffer.readAndRemove(first.size(), first.data());
    Require(first == std::array<int, 2>{{10, 20}}, "read and remove order");
    Require(buffer.size() == 2, "occupancy after remove");

    Require(buffer.write(50), "write after freeing full buffer");
    Require(buffer.write(60), "wraparound write");
    std::array<int, 4> wrapped{};
    buffer.read(wrapped.size(), wrapped.data());
    Require(wrapped == std::array<int, 4>{{30, 40, 50, 60}}, "wraparound order");
    Require(buffer.removeBeginning(3), "remove beginning succeeds");
    Require(buffer.size() == 1 && buffer.read(0) == 60, "remove beginning occupancy");
    buffer.destroy();
}

void InitializeSynthesizer(
    Synthesizer &synthesizer,
    int inputCapacity = 4096,
    int outputCapacity = 4096) {
    Synthesizer::Parameters parameters;
    parameters.inputChannelCount = 1;
    parameters.inputBufferSize = inputCapacity;
    parameters.audioBufferSize = outputCapacity;
    parameters.inputSampleRate = 44100.0f;
    parameters.audioSampleRate = 44100.0f;
    parameters.initialAudioParameters.airNoise = 0.0f;
    parameters.initialAudioParameters.inputSampleNoise = 0.0f;
    parameters.initialAudioParameters.convolution = 1.0f;
    parameters.initialAudioParameters.dF_F_mix = 0.0f;
    synthesizer.initialize(parameters);
    constexpr std::int16_t identity[] = {32767};
    synthesizer.initializeImpulseResponse(identity, 1, 1.0f, 0);
    synthesizer.setDeterministicSeed(0x12345678u);
}

void WriteSynthFrames(Synthesizer &synthesizer, int count, double value = 1000.0) {
    const double input[] = {value};
    for (int i = 0; i < count; ++i) synthesizer.writeInput(input);
}

void TestSynchronousSynthesizer() {
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer);
        Require(synthesizer.processSynchronous(2000) == 0, "zero pending frames");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer);
        WriteSynthFrames(synthesizer, 37);
        Require(synthesizer.processSynchronous(2000) == 37, "partial production count");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer, 2500, 2500);
        WriteSynthFrames(synthesizer, 2000);
        Require(synthesizer.processSynchronous(2000) == 2000, "exact cap production");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer, 4096, 4096);
        WriteSynthFrames(synthesizer, 2501);
        Require(synthesizer.processSynchronous(2000) == 2000, "over cap production");
        Require(synthesizer.getInputBufferedFrames() == 501, "over cap pending remainder");
        synthesizer.destroy();
    }
}

class ChamberRngProbe : public CombustionChamber {
public:
    double Next() { return nextUnitRandom(); }
};

void TestDeterministicRng() {
    ChamberRngProbe first;
    ChamberRngProbe second;
    ChamberRngProbe different;
    first.setDeterministicSeed(0x10203040u);
    second.setDeterministicSeed(0x10203040u);
    different.setDeterministicSeed(0x10203041u);
    std::array<double, 16> sequence{};
    for (double &value : sequence) value = first.Next();
    for (double value : sequence) Require(value == second.Next(), "same seed sequence mismatch");
    bool differs = false;
    for (double value : sequence) differs = differs || value != different.Next();
    Require(differs, "different seeds must differ");
}

class ConstantForce final : public atg_scs::ForceGenerator {
public:
    void apply(atg_scs::SystemState *system) override {
        system->applyForce(0.0, 0.0, 10.0, 0.0, 0);
    }
};

void TestSolverRuntime() {
    atg_scs::OptimizedNsvRigidBodySystem system;
    system.initialize(new atg_scs::GaussSeidelSleSolver);
    atg_scs::RigidBody body;
    body.reset();
    body.m = 2.0;
    body.I = 1.0;
    ConstantForce force;
    system.addRigidBody(&body);
    system.addForceGenerator(&force);
    system.process(0.1, 1);
    Require(std::isfinite(body.p_x) && std::isfinite(body.v_x), "solver finite state");
    Require(body.v_x > 0.0 && body.p_x > 0.0, "solver force integration");
    system.reset();
}

void TestGeneratedImpulseResponse() {
    using namespace NextcarEngineSim::Generated;
    Require(MinimalMuffling02SampleCount == 17555, "generated impulse response sample count");
    Require(MinimalMuffling02SampleRate == 44100, "generated impulse response sample rate");
    Require(MinimalMuffling02Channels == 1, "generated impulse response channel count");
    Require(MinimalMuffling02BitsPerSample == 16, "generated impulse response bit depth");
    Require(
        !(MinimalMuffling02SampleCount == 1 && MinimalMuffling02Pcm[0] == 32767),
        "fixture impulse response must not be identity PCM");
}

void TestFixtureTranscriptionParity() {
    using NextcarEngineSim::Phase0::SubaruEJ25AtgVideo2Fixture;
    SubaruEJ25AtgVideo2Fixture fixture;
    const auto &s = fixture.GetTranscriptionSnapshot();
    constexpr double ExactAbs = 1e-12;
    constexpr double ExactRel = 1e-12;
    constexpr double UnitAbs = 1e-12;
    constexpr double UnitRel = 1e-10;

    Require(std::string(s.EngineName.data()) == "Subaru EJ25", "engine name");
    RequireNear(s.StarterTorqueLbFt, 70.0, ExactAbs, ExactRel, "starter torque");
    RequireNear(s.StarterSpeedRpm, 500.0, ExactAbs, ExactRel, "starter speed");
    RequireNear(s.RedlineRpm, 6500.0, ExactAbs, ExactRel, "redline");
    RequireNear(s.DynoMinSpeedRpm, 1000.0, ExactAbs, ExactRel, "dyno min");
    RequireNear(s.DynoMaxSpeedRpm, 6500.0, ExactAbs, ExactRel, "dyno max");
    RequireNear(s.DynoHoldStepRpm, 100.0, ExactAbs, ExactRel, "dyno step");
    RequireNear(s.ThrottleGamma, 2.0, ExactAbs, ExactRel, "throttle gamma");
    Require(s.SimulationFrequencyHz == 20000, "simulation frequency");
    RequireNear(s.HighFrequencyGain, 0.01, ExactAbs, ExactRel, "hf gain");
    RequireNear(s.Noise, 1.0, ExactAbs, ExactRel, "noise");
    RequireNear(s.Jitter, 0.5, ExactAbs, ExactRel, "jitter");

    Require(std::string(s.FuelName.data()) == "Gasoline [Default]", "fuel name default");
    RequireNear(s.FuelMolecularMassG, 100.0, ExactAbs, ExactRel, "fuel molecular mass");
    RequireNear(s.FuelEnergyDensityKjPerG, 48.1, ExactAbs, ExactRel, "fuel energy density");
    RequireNear(s.FuelDensityKgPerL, 0.755, ExactAbs, ExactRel, "fuel density");
    RequireNear(s.FuelMolecularAfr, 12.5, ExactAbs, ExactRel, "fuel AFR");
    RequireNear(s.FuelBurningRandomness, 0.5, ExactAbs, ExactRel, "fuel randomness");
    RequireNear(s.FuelLowEfficiencyAttenuation, 0.6, ExactAbs, ExactRel, "low efficiency attenuation");
    RequireNear(s.FuelMaxBurningEfficiency, 0.9, ExactAbs, ExactRel, "max burn efficiency");
    RequireNear(s.FuelMaxTurbulenceEffect, 2.0, ExactAbs, ExactRel, "max turbulence");
    RequireNear(s.FuelMaxDilutionEffect, 10.0, ExactAbs, ExactRel, "max dilution");
    const std::array<std::array<double, 2>, 10> fuelTable{{
        {{0, 3}}, {{5, 7.5}}, {{10, 15}}, {{15, 24.75}}, {{20, 37.5}},
        {{25, 46.875}}, {{30, 56.25}}, {{35, 65.625}}, {{40, 75}}, {{45, 84.375}}
    }};
    for (std::size_t i = 0; i < fuelTable.size(); ++i) {
        RequireNear(s.FuelTurbulenceTable[i][0], fuelTable[i][0], ExactAbs, ExactRel, "fuel table x");
        RequireNear(s.FuelTurbulenceTable[i][1], fuelTable[i][1], ExactAbs, ExactRel, "fuel table y");
    }
    Require(s.MeanPistonSpeedTurbulenceTable.size() == 30, "mean-piston turbulence size");
    for (std::size_t i = 0; i < s.MeanPistonSpeedTurbulenceTable.size(); ++i) {
        RequireNear(s.MeanPistonSpeedTurbulenceTable[i][0], static_cast<double>(i), ExactAbs, ExactRel, "mean-piston x");
        RequireNear(s.MeanPistonSpeedTurbulenceTable[i][1], static_cast<double>(i) * 0.5, ExactAbs, ExactRel, "mean-piston y");
    }

    RequireNear(s.StrokeMm, 79.0, ExactAbs, ExactRel, "stroke");
    RequireNear(s.BoreMm, 99.5, ExactAbs, ExactRel, "bore");
    RequireNear(s.RodLengthInch, 5.142, ExactAbs, ExactRel, "rod length");
    RequireNear(s.RodMassG, 535.0, ExactAbs, ExactRel, "rod mass");
    RequireNear(s.CompressionHeightInch, 1.0, ExactAbs, ExactRel, "compression height");
    const double strokeM = units::distance(79.0, units::mm);
    const double rodLengthM = units::distance(5.142, units::inch);
    const double compressionHeightM = units::distance(1.0, units::inch);
    const double crankMassKg = units::mass(9.39, units::kg);
    const double flywheelMassKg = units::mass(6.8, units::kg);
    const double flywheelRadiusM = units::distance(6.0, units::inch);
    const double expectedCrankThrowM = strokeM / 2.0;
    const double expectedDeckHeightM = expectedCrankThrowM + rodLengthM + compressionHeightM;
    const double expectedCrankMoment = 0.5 * crankMassKg * expectedCrankThrowM * expectedCrankThrowM;
    const double expectedFlywheelMoment = 2.0 * 0.5 * flywheelMassKg * flywheelRadiusM * flywheelRadiusM;
    const double expectedOtherMoment = 0.5 * units::mass(10.0, units::kg)
        * std::pow(units::distance(6.0, units::cm), 2.0);
    const double expectedRodMoment = units::mass(535.0, units::g) * rodLengthM * rodLengthM / 12.0;
    RequireNear(units::distance(s.CrankThrowMm, units::mm), expectedCrankThrowM, UnitAbs, UnitRel, "crank throw formula");
    RequireNear(units::distance(s.DeckHeightMm, units::mm), expectedDeckHeightM, UnitAbs, UnitRel, "deck height formula");
    RequireNear(s.CrankDiskMomentKgM2, expectedCrankMoment, UnitAbs, UnitRel, "crank moment formula");
    RequireNear(s.FlywheelMomentKgM2, expectedFlywheelMoment, UnitAbs, UnitRel, "flywheel moment formula");
    RequireNear(s.OtherMomentKgM2, expectedOtherMoment, UnitAbs, UnitRel, "other moment formula");
    RequireNear(
        s.TotalCrankMomentKgM2,
        expectedCrankMoment + expectedFlywheelMoment + expectedOtherMoment,
        UnitAbs,
        UnitRel,
        "total crank moment formula");
    RequireNear(s.RodMomentKgM2, expectedRodMoment, UnitAbs, UnitRel, "rod moment formula");
    Require(s.ObjectCounts.CylinderBanks == 2, "object count banks");
    Require(s.ObjectCounts.Cylinders == 4, "object count cylinders");
    Require(s.ObjectCounts.Crankshafts == 1, "object count crankshafts");
    Require(s.ObjectCounts.Intakes == 1, "object count intakes");
    Require(s.ObjectCounts.ExhaustSystems == 1, "object count exhausts");

    RequireNear(s.CrankMassKg, 9.39, ExactAbs, ExactRel, "crank mass");
    RequireNear(s.FlywheelMassKg, 6.8, ExactAbs, ExactRel, "flywheel mass");
    RequireNear(s.FlywheelRadiusInch, 6.0, ExactAbs, ExactRel, "flywheel radius");
    RequireNear(s.CrankFrictionLbFt, 1.0, ExactAbs, ExactRel, "crank friction");
    RequireNear(s.CrankPositionX, 0.0, ExactAbs, ExactRel, "crank position x");
    RequireNear(s.CrankPositionY, 0.0, ExactAbs, ExactRel, "crank position y");
    RequireNear(s.CrankTdcDeg, 180.0, ExactAbs, ExactRel, "crank tdc");
    RequireNear(s.PistonMassG, 566.0, ExactAbs, ExactRel, "piston mass");
    RequireNear(s.PistonWristPinPosition, 0.0, ExactAbs, ExactRel, "wrist pin");
    RequireNear(s.PistonDisplacement, 0.0, ExactAbs, ExactRel, "piston displacement");
    RequireNear(s.RodCenterOfMass, 0.0, ExactAbs, ExactRel, "rod center of mass");
    RequireNear(s.RodSlaveThrow, 0.0, ExactAbs, ExactRel, "rod slave throw");
    RequireNear(s.BankDisplayDepth, 0.5, ExactAbs, ExactRel, "bank display default");

    const std::array<double, 4> journalAngles{{0, 180, 0, 180}};
    const std::array<double, 2> bankAngles{{90, -90}};
    const std::array<int, 4> journalMapping{{0, 3, 1, 2}};
    const std::array<double, 4> blowby{{0.001, 0.002, 0.001, 0.002}};
    const std::array<double, 4> primaryLengths{{2, 3, 3, 5}};
    const std::array<double, 4> attenuations{{0.9, 1.0, 1.1, 0.9}};
    RequireNearArray(s.JournalAnglesDeg, journalAngles, ExactAbs, ExactRel, "journal angles");
    RequireNearArray(s.BankAnglesDeg, bankAngles, ExactAbs, ExactRel, "bank angles");
    RequireExact(s.JournalMapping, journalMapping, "journal mapping");
    RequireNearArray(s.Blowby28InH2O, blowby, ExactAbs, ExactRel, "blowby");
    RequireNearArray(s.PrimaryLengthsInch, primaryLengths, ExactAbs, ExactRel, "primary lengths");
    RequireNearArray(s.SoundAttenuations, attenuations, ExactAbs, ExactRel, "attenuations");

    RequireNear(s.IntakePlenumVolumeL, 1.325, ExactAbs, ExactRel, "intake plenum volume");
    RequireNear(s.IntakePlenumAreaCm2, 20.0, ExactAbs, ExactRel, "intake plenum area");
    RequireNear(s.IntakeFlowCarb, 400.0, ExactAbs, ExactRel, "intake input flow");
    RequireNear(s.IntakeRunnerFlowCarb, 100.0, ExactAbs, ExactRel, "intake runner flow");
    RequireNear(s.IntakeRunnerLengthInch, 12.0, ExactAbs, ExactRel, "intake runner length");
    RequireNear(s.IntakeIdleFlowCarb, 0.0, ExactAbs, ExactRel, "intake idle flow");
    RequireNear(s.IntakeIdlePlate, 0.9978, ExactAbs, ExactRel, "intake idle plate");
    RequireNear(s.IntakeVelocityDecay, 1.0, ExactAbs, ExactRel, "intake velocity decay");
    RequireNear(s.IntakeMolecularAfr, 12.5, ExactAbs, ExactRel, "intake AFR default");

    RequireNear(s.ExhaustLengthMm, 500.0, ExactAbs, ExactRel, "exhaust length");
    RequireNear(s.ExhaustCollectorDiameterInch, 2.0, ExactAbs, ExactRel, "collector diameter");
    RequireNear(
        s.ExhaustCollectorAreaSquareInch,
        constants::pi * std::pow(s.ExhaustCollectorDiameterInch / 2.0, 2.0),
        ExactAbs,
        ExactRel,
        "collector area formula");
    RequireNear(s.ExhaustOutletFlowCarb, 1000.0, ExactAbs, ExactRel, "exhaust outlet flow");
    RequireNear(s.ExhaustPrimaryTubeLengthInch, 40.0, ExactAbs, ExactRel, "primary tube length");
    RequireNear(s.ExhaustPrimaryFlowCarb, 400.0, ExactAbs, ExactRel, "primary flow");
    RequireNear(s.ExhaustVelocityDecay, 1.0, ExactAbs, ExactRel, "exhaust velocity decay");
    RequireNear(s.ExhaustAudioVolume, 0.01, ExactAbs, ExactRel, "exhaust audio volume");
    RequireNear(s.ImpulseGain, 0.01, ExactAbs, ExactRel, "fixture IR gain");

    RequireNear(s.HeadChamberVolumeCc, 67.0, ExactAbs, ExactRel, "head chamber volume");
    RequireNear(s.HeadIntakeRunnerVolumeCc, 149.6, ExactAbs, ExactRel, "intake runner volume");
    RequireNear(s.HeadIntakeRunnerAreaSquareInch, 1.75 * 1.75, ExactAbs, ExactRel, "intake runner area");
    RequireNear(s.HeadExhaustRunnerVolumeCc, 50.0, ExactAbs, ExactRel, "exhaust runner volume");
    RequireNear(s.HeadExhaustRunnerAreaSquareInch, 1.25 * 1.25, ExactAbs, ExactRel, "exhaust runner area");
    RequireExact(s.HeadFlipDisplay, std::array<bool, 2>{{true, false}}, "head flip display");
    const std::array<double, 10> intakeFlow{{0, 58, 103, 156, 214, 249, 268, 280, 280, 281}};
    const std::array<double, 10> exhaustFlow{{0, 37, 72, 113, 160, 196, 222, 235, 245, 246}};
    RequireNearArray(s.IntakeFlowCfm, intakeFlow, ExactAbs, ExactRel, "intake flow");
    RequireNearArray(s.ExhaustFlowCfm, exhaustFlow, ExactAbs, ExactRel, "exhaust flow");

    RequireNear(s.IntakeLobe.DurationAt50ThouDeg, 232.0, ExactAbs, ExactRel, "intake lobe duration");
    RequireNear(s.IntakeLobe.Gamma, 2.0, ExactAbs, ExactRel, "intake lobe gamma");
    RequireNear(s.IntakeLobe.LiftMm, 9.78, ExactAbs, ExactRel, "intake lobe lift");
    Require(s.IntakeLobe.Steps == 100, "intake lobe steps");
    RequireNear(s.ExhaustLobe.DurationAt50ThouDeg, 236.0, ExactAbs, ExactRel, "exhaust lobe duration");
    RequireNear(s.ExhaustLobe.Gamma, 2.0, ExactAbs, ExactRel, "exhaust lobe gamma");
    RequireNear(s.ExhaustLobe.LiftMm, 9.60, ExactAbs, ExactRel, "exhaust lobe lift");
    Require(s.ExhaustLobe.Steps == 100, "exhaust lobe steps");
    RequireNear(s.CamBaseRadiusMm, 17.0, ExactAbs, ExactRel, "cam base radius");
    RequireNear(s.CamAdvanceDeg, 0.0, ExactAbs, ExactRel, "cam advance");
    const std::array<double, 4> intakeCenters{{477, 657, 837, 1017}};
    const std::array<double, 4> exhaustCenters{{248, 428, 608, 788}};
    RequireNearArray(s.IntakeCenterlinesDeg, intakeCenters, ExactAbs, ExactRel, "intake centers");
    RequireNearArray(s.ExhaustCenterlinesDeg, exhaustCenters, ExactAbs, ExactRel, "exhaust centers");

    const std::array<double, 5> timingRpm{{0, 1000, 2000, 3000, 4000}};
    const std::array<double, 5> timingAdvance{{25, 25, 30, 40, 40}};
    const std::array<double, 4> firingOrder{{0, 180, 360, 540}};
    const std::array<int, 4> ignitionMapping{{0, 1, 2, 3}};
    RequireNearArray(s.TimingRpm, timingRpm, ExactAbs, ExactRel, "timing RPM");
    RequireNearArray(s.TimingAdvanceDeg, timingAdvance, ExactAbs, ExactRel, "timing advance");
    RequireNear(s.IgnitionRevLimitRpm, 6800.0, ExactAbs, ExactRel, "ignition rev limit");
    RequireNear(s.IgnitionLimiterDurationSeconds, 0.16, ExactAbs, ExactRel, "limiter duration");
    RequireNearArray(s.FiringOrderDeg, firingOrder, ExactAbs, ExactRel, "firing order");
    RequireExact(s.IgnitionCylinderMapping, ignitionMapping, "ignition cylinder mapping");

    RequireNear(s.ChamberStartingPressureAtm, 1.0, ExactAbs, ExactRel, "starting pressure");
    RequireNear(s.ChamberStartingTemperatureC, 25.0, ExactAbs, ExactRel, "starting temperature");
    RequireNear(s.ChamberCrankcasePressureAtm, 1.0, ExactAbs, ExactRel, "crankcase pressure");
    Require(s.ValvetrainCount == 2, "valvetrain ownership count");
    Require(s.CamshaftCount == 4, "camshaft ownership count");
    const std::array<std::uint32_t, 4> seeds{{
        0x4E433033u, 0x4E433034u, 0x4E433035u, 0x4E433036u
    }};
    RequireExact(s.DeterministicSeeds, seeds, "deterministic seeds");
    for (std::size_t i = 1; i < seeds.size(); ++i) {
        Require(seeds[i] == seeds[0] + i, "seed increment rule");
        Require(seeds[i] != seeds[i - 1], "seeds must be distinct");
    }

    Require(s.IrSampleCount == 17555, "snapshot IR sample count");
    Require(s.IrSampleRate == 44100, "snapshot IR sample rate");
    Require(s.IrChannels == 1, "snapshot IR channels");
    Require(s.IrBitsPerSample == 16, "snapshot IR bits");

    Engine &engine = fixture.GetEngine();
    const double expectedCollectorArea = units::area(
        constants::pi * std::pow(2.0 / 2.0, 2.0),
        units::inch * units::inch);
    RequireNear(
        engine.getExhaustSystem(0)->getCollectorCrossSectionArea(),
        expectedCollectorArea,
        UnitAbs,
        UnitRel,
        "collector area circle semantics");
    RequireNear(engine.getCylinderBank(0)->getDisplayDepth(), 0.5, ExactAbs, ExactRel, "bank 0 display depth");
    RequireNear(engine.getCylinderBank(1)->getDisplayDepth(), 0.5, ExactAbs, ExactRel, "bank 1 display depth");
    RequireNear(engine.getFuel()->getMaxDilutionEffect(), 10.0, ExactAbs, ExactRel, "fuel default applied");
    Require(engine.getCylinderCount() == s.ObjectCounts.Cylinders, "cylinder count applied");
    Require(engine.getCylinderBankCount() == s.ObjectCounts.CylinderBanks, "bank count applied");
    Require(engine.getCrankshaftCount() == s.ObjectCounts.Crankshafts, "crank count applied");
    Require(engine.getIntakeCount() == s.ObjectCounts.Intakes, "intake count applied");
    Require(engine.getExhaustSystemCount() == s.ObjectCounts.ExhaustSystems, "exhaust count applied");
    fixture.Destroy();
}

void TestFixtureAndSimulator() {
    NextcarEngineSim::Phase0::SubaruEJ25AtgVideo2Fixture fixture;
    Engine &engine = fixture.GetEngine();
    Require(engine.getCylinderCount() == 4, "fixture cylinder count");
    Require(engine.getDisplacement() > 0.0 && std::isfinite(engine.getDisplacement()), "fixture displacement");

    Simulator *simulator = engine.createSimulator();
    Require(simulator != nullptr, "headless simulator creation without Vehicle or Transmission");
    Require(simulator->getEngine() == &engine, "headless simulator load");

    const double input[] = {1.0};
    for (int i = 0; i < 2501; ++i) simulator->synthesizer().writeInput(input);
    const int pendingBefore = simulator->synthesizer().getInputBufferedFrames();
    Require(pendingBefore > Simulator::MaximumSynchronousFrames, "Simulator cap precondition");
    const int produced = simulator->endFrame(5000);
    Require(produced == Simulator::MaximumSynchronousFrames, "Simulator::endFrame cap");
    Require(
        simulator->synthesizer().getInputBufferedFrames()
            == pendingBefore - Simulator::MaximumSynchronousFrames,
        "Simulator cap remainder");

    simulator->destroy();
    delete simulator;
    fixture.Destroy();
}

} // namespace

int main() {
    try {
        TestRingBuffer();
        std::cout << "PASS ring-buffer\n";
        TestSynchronousSynthesizer();
        std::cout << "PASS synchronous-synthesizer\n";
        TestDeterministicRng();
        std::cout << "PASS deterministic-rng\n";
        TestSolverRuntime();
        std::cout << "PASS solver-runtime\n";
        TestGeneratedImpulseResponse();
        std::cout << "PASS impulse-response-integrity\n";
        TestFixtureTranscriptionParity();
        std::cout << "PASS fixture-transcription-parity\n";
        TestFixtureAndSimulator();
        std::cout << "PASS fixture-simulator-smoke\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
