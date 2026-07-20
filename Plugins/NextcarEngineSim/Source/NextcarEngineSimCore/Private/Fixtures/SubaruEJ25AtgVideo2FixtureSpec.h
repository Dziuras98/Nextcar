#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace NextcarEngineSim::Phase0 {

struct HarmonicLobeSnapshot {
    double DurationAt50ThouDeg;
    double Gamma;
    double LiftMm;
    int Steps;
};

struct ObjectCountSnapshot {
    int CylinderBanks;
    int Cylinders;
    int Crankshafts;
    int Intakes;
    int ExhaustSystems;
};

struct FixtureTranscriptionSnapshot {
    std::array<char, 13> EngineName{{'S','u','b','a','r','u',' ','E','J','2','5','\0','\0'}};
    double StarterTorqueLbFt = 70.0;
    double StarterSpeedRpm = 500.0;
    double RedlineRpm = 6500.0;
    double DynoMinSpeedRpm = 1000.0;
    double DynoMaxSpeedRpm = 6500.0;
    double DynoHoldStepRpm = 100.0;
    double ThrottleGamma = 2.0;
    int SimulationFrequencyHz = 20000;
    double HighFrequencyGain = 0.01;
    double Noise = 1.0;
    double Jitter = 0.5;

    std::array<char, 19> FuelName{{'G','a','s','o','l','i','n','e',' ','[','D','e','f','a','u','l','t',']','\0'}};
    double FuelMolecularMassG = 100.0;
    double FuelEnergyDensityKjPerG = 48.1;
    double FuelDensityKgPerL = 0.755;
    double FuelMolecularAfr = 12.5;
    double FuelBurningRandomness = 0.5;
    double FuelLowEfficiencyAttenuation = 0.6;
    double FuelMaxBurningEfficiency = 0.9;
    double FuelMaxTurbulenceEffect = 2.0;
    double FuelMaxDilutionEffect = 10.0;
    std::array<std::array<double, 2>, 10> FuelTurbulenceTable{{
        {{0.0, 3.0}}, {{5.0, 7.5}}, {{10.0, 15.0}}, {{15.0, 24.75}}, {{20.0, 37.5}},
        {{25.0, 46.875}}, {{30.0, 56.25}}, {{35.0, 65.625}}, {{40.0, 75.0}}, {{45.0, 84.375}}
    }};
    std::array<std::array<double, 2>, 30> MeanPistonSpeedTurbulenceTable{{
        {{0,0}},{{1,0.5}},{{2,1}},{{3,1.5}},{{4,2}},{{5,2.5}},{{6,3}},{{7,3.5}},{{8,4}},{{9,4.5}},
        {{10,5}},{{11,5.5}},{{12,6}},{{13,6.5}},{{14,7}},{{15,7.5}},{{16,8}},{{17,8.5}},{{18,9}},{{19,9.5}},
        {{20,10}},{{21,10.5}},{{22,11}},{{23,11.5}},{{24,12}},{{25,12.5}},{{26,13}},{{27,13.5}},{{28,14}},{{29,14.5}}
    }};

    double StrokeMm = 79.0;
    double BoreMm = 99.5;
    double RodLengthInch = 5.142;
    double RodMassG = 535.0;
    double CompressionHeightInch = 1.0;
    double CrankThrowMm = 39.5;
    double DeckHeightMm = 195.5068;
    double CrankDiskMomentKgM2 = 0.00732537375;
    double FlywheelMomentKgM2 = 0.157935168;
    double OtherMomentKgM2 = 0.018;
    double TotalCrankMomentKgM2 = 0.18326054175;
    double RodMomentKgM2 = 0.0007605085725282;
    ObjectCountSnapshot ObjectCounts{2, 4, 1, 1, 1};
    double CrankMassKg = 9.39;
    double FlywheelMassKg = 6.8;
    double FlywheelRadiusInch = 6.0;
    double CrankFrictionLbFt = 1.0;
    double CrankPositionX = 0.0;
    double CrankPositionY = 0.0;
    double CrankTdcDeg = 180.0;
    std::array<double, 4> JournalAnglesDeg{{0.0, 180.0, 0.0, 180.0}};
    double PistonMassG = 566.0;
    double PistonWristPinPosition = 0.0;
    double PistonDisplacement = 0.0;
    double RodCenterOfMass = 0.0;
    double RodSlaveThrow = 0.0;
    double BankDisplayDepth = 0.5;
    std::array<double, 2> BankAnglesDeg{{90.0, -90.0}};

    double IntakePlenumVolumeL = 1.325;
    double IntakePlenumAreaCm2 = 20.0;
    double IntakeFlowCarb = 400.0;
    double IntakeRunnerFlowCarb = 100.0;
    double IntakeRunnerLengthInch = 12.0;
    double IntakeIdleFlowCarb = 0.0;
    double IntakeIdlePlate = 0.9978;
    double IntakeVelocityDecay = 1.0;
    double IntakeMolecularAfr = 12.5;

    double ExhaustLengthMm = 500.0;
    double ExhaustCollectorDiameterInch = 2.0;
    double ExhaustCollectorAreaSquareInch = 3.14159265358979323846;
    double ExhaustOutletFlowCarb = 1000.0;
    double ExhaustPrimaryTubeLengthInch = 40.0;
    double ExhaustPrimaryFlowCarb = 400.0;
    double ExhaustVelocityDecay = 1.0;
    double ExhaustAudioVolume = 0.01;
    double ImpulseGain = 0.01;

    std::array<int, 4> JournalMapping{{0, 3, 1, 2}};
    std::array<double, 4> Blowby28InH2O{{0.001, 0.002, 0.001, 0.002}};
    std::array<double, 4> PrimaryLengthsInch{{2.0, 3.0, 3.0, 5.0}};
    std::array<double, 4> SoundAttenuations{{0.9, 1.0, 1.1, 0.9}};

    double HeadChamberVolumeCc = 67.0;
    double HeadIntakeRunnerVolumeCc = 149.6;
    double HeadIntakeRunnerAreaSquareInch = 3.0625;
    double HeadExhaustRunnerVolumeCc = 50.0;
    double HeadExhaustRunnerAreaSquareInch = 1.5625;
    std::array<bool, 2> HeadFlipDisplay{{true, false}};
    std::array<double, 10> IntakeFlowCfm{{0, 58, 103, 156, 214, 249, 268, 280, 280, 281}};
    std::array<double, 10> ExhaustFlowCfm{{0, 37, 72, 113, 160, 196, 222, 235, 245, 246}};

    HarmonicLobeSnapshot IntakeLobe{232.0, 2.0, 9.78, 100};
    HarmonicLobeSnapshot ExhaustLobe{236.0, 2.0, 9.60, 100};
    double CamBaseRadiusMm = 17.0;
    double CamAdvanceDeg = 0.0;
    std::array<double, 4> IntakeCenterlinesDeg{{477.0, 657.0, 837.0, 1017.0}};
    std::array<double, 4> ExhaustCenterlinesDeg{{248.0, 428.0, 608.0, 788.0}};

    std::array<double, 5> TimingRpm{{0, 1000, 2000, 3000, 4000}};
    std::array<double, 5> TimingAdvanceDeg{{25, 25, 30, 40, 40}};
    double IgnitionRevLimitRpm = 6800.0;
    double IgnitionLimiterDurationSeconds = 0.16;
    std::array<double, 4> FiringOrderDeg{{0.0, 180.0, 360.0, 540.0}};
    std::array<int, 4> IgnitionCylinderMapping{{0, 1, 2, 3}};

    double ChamberStartingPressureAtm = 1.0;
    double ChamberStartingTemperatureC = 25.0;
    double ChamberCrankcasePressureAtm = 1.0;
    int ValvetrainCount = 2;
    int CamshaftCount = 4;
    std::array<std::uint32_t, 4> DeterministicSeeds{{
        0x4E433033u, 0x4E433034u, 0x4E433035u, 0x4E433036u
    }};

    std::size_t IrSampleCount = 17555;
    std::uint32_t IrSampleRate = 44100;
    std::uint16_t IrChannels = 1;
    std::uint16_t IrBitsPerSample = 16;
};

inline FixtureTranscriptionSnapshot MakeSubaruEJ25AtgVideo2Snapshot() {
    return FixtureTranscriptionSnapshot{};
}

} // namespace NextcarEngineSim::Phase0
