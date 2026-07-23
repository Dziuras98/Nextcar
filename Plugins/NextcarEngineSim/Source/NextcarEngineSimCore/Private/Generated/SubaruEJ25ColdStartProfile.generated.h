// Generated deterministically from the measured Subaru EJ25 cold-start profile.
// Do not edit.
#pragma once

#include <cstdint>

namespace NextcarEngineSim::Generated {

inline constexpr char SubaruEJ25ColdStartProfileId[] =
    "SubaruEJ25AtgVideo2ColdStartV1";
inline constexpr char SubaruEJ25ColdStartProfileFixtureId[] =
    "SubaruEJ25AtgVideo2";
inline constexpr char SubaruEJ25ColdStartProfileValidatedParitySourceSha[] =
    "542d3261efc3ef48c78f337d990793aea55dd7fb";
inline constexpr char SubaruEJ25ColdStartProfileEngineSimCommit[] =
    "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630";
inline constexpr char SubaruEJ25ColdStartProfileEngineSimTree[] =
    "0756dea0444ada160540685dd1d28fcd3ef4aac5";
inline constexpr char SubaruEJ25ColdStartProfileSolverCommit[] =
    "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3";
inline constexpr char SubaruEJ25ColdStartProfileFixtureSemanticContractSha256[] =
    "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00";
inline constexpr char SubaruEJ25ColdStartProfileIrWavSha256[] =
    "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48";
inline constexpr char SubaruEJ25ColdStartProfileGeneratedIrSha256[] =
    "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540";
inline constexpr char SubaruEJ25ColdStartProfilePayloadSha256[] =
    "093ef4e39526563ebcc211e127a7556dda0a154e1f4b0e75c806edf360389f1d";
inline constexpr char SubaruEJ25ColdStartProfileSelectedCandidateId[] =
    "thr-0p050-dis-0800-min-0600-win-2p000-max-8p000";

inline constexpr std::uint32_t SubaruEJ25ColdStartProfileSeed =
    1313026099u;
inline constexpr int SubaruEJ25ColdStartProfileSimulationFrequencyHz =
    20000;
inline constexpr int SubaruEJ25ColdStartProfileSampleRateHz =
    44100;
inline constexpr double SubaruEJ25ColdStartProfileStartupThrottle =
    0.050000000000000003;
inline constexpr double SubaruEJ25ColdStartProfileStarterDisengagementRpm =
    800;
inline constexpr double SubaruEJ25ColdStartProfilePostStarterMinimumRpm =
    600;
inline constexpr double SubaruEJ25ColdStartProfileStabilityWindowSeconds =
    2;
inline constexpr double SubaruEJ25ColdStartProfileMaximumStartupSimulationSeconds =
    8;
inline constexpr int SubaruEJ25ColdStartProfileTrialCount =
    10;
inline constexpr int SubaruEJ25ColdStartProfileSuccessCount =
    10;

static_assert(SubaruEJ25ColdStartProfileTrialCount >= 10);
static_assert(
    SubaruEJ25ColdStartProfileSuccessCount
    == SubaruEJ25ColdStartProfileTrialCount);

} // namespace NextcarEngineSim::Generated
