#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace NextcarEngineSim::Phase0::Calibration {

enum class ColdStartResult {
    Success,
    Timeout,
    StallAfterDisengagement,
    NonFiniteRpm,
    NonFinitePcm,
    ZeroProducedPcm,
    ZeroPostStartRms,
    InternalError
};

struct ColdStartCandidate {
    std::string CandidateId;
    std::uint32_t Seed = 0x4E433033u;
    int TrialIndex = 0;
    double StartupThrottle = 0.0;
    double StarterDisengagementRpm = 0.0;
    double PostStarterMinimumRpm = 0.0;
    double StabilityWindowSeconds = 0.0;
    double MaximumStartupSimulationSeconds = 0.0;
    bool DisableIgnitionAfterDisengagement = false;
};

struct ColdStartCycleSample {
    int CycleIndex = 0;
    double SimulationTimeSeconds = 0.0;
    bool IgnitionEnabled = false;
    bool StarterEnabled = false;
    double CurrentRpm = 0.0;
    double StarterDisengagementRpmObserved = 0.0;
    bool HasStarterDisengagementRpmObserved = false;
    int ProducedNativeFrames = 0;
    int ProducedPcmFrames = 0;
    int PcmNonFiniteSamples = 0;
    double PcmPeak = 0.0;
    double PcmRms = 0.0;
};

struct ColdStartTrace {
    int SchemaVersion = 1;
    std::string FixtureId;
    std::string ValidatedParitySourceSha;
    std::string EngineSimCommit;
    std::string EngineSimTree;
    std::string SolverCommit;
    std::string FixtureSemanticContractSha256;
    std::string IrWavSha256;
    std::string GeneratedIrSha256;

    ColdStartCandidate Candidate;
    std::vector<ColdStartCycleSample> Cycles;

    ColdStartResult Result = ColdStartResult::InternalError;
    std::string FailureReason;

    double StarterDisengagementTimeSeconds = 0.0;
    double StarterDisengagementRpmObserved = 0.0;
    bool HasStarterDisengagement = false;
    double MinimumPostStarterRpm = 0.0;
    double MeanPostStarterRpm = 0.0;
    int ProducedNativeFrames = 0;
    int ProducedPcmFrames = 0;
    int PcmNonFiniteSamples = 0;
    double PcmPeak = 0.0;
    double PcmRms = 0.0;
    double PostStartPcmRms = 0.0;

    bool CleanupStarterDisabled = false;
    bool CleanupIgnitionDisabled = false;
    bool CleanupSimulatorDestroyed = false;
    bool CleanupFixtureDestroyed = false;
};

class ColdStartCalibrationHarness {
public:
    static constexpr double SimulationSliceSeconds = 1.0 / 120.0;

    ColdStartTrace Run(const ColdStartCandidate &candidate) const;

    static const char *ResultName(ColdStartResult result);
    static std::string SerializeTraceJson(const ColdStartTrace &trace);
    static void WriteTraceJson(const std::string &path, const ColdStartTrace &trace);
};

} // namespace NextcarEngineSim::Phase0::Calibration
