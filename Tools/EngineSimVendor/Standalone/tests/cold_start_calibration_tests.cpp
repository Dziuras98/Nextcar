#include "ColdStartCalibrationHarness.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

using NextcarEngineSim::Phase0::Calibration::ColdStartCalibrationHarness;
using NextcarEngineSim::Phase0::Calibration::ColdStartCandidate;
using NextcarEngineSim::Phase0::Calibration::ColdStartResult;

void Require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void TestDeterministicTimeoutTrace() {
    ColdStartCandidate candidate;
    candidate.CandidateId = "smoke-timeout";
    candidate.Seed = 0x4E433033u;
    candidate.TrialIndex = 0;
    candidate.StartupThrottle = 0.05;
    candidate.StarterDisengagementRpm = 100000.0;
    candidate.PostStarterMinimumRpm = 400.0;
    candidate.StabilityWindowSeconds = 0.5;
    candidate.MaximumStartupSimulationSeconds =
        2.0 * ColdStartCalibrationHarness::SimulationSliceSeconds;

    ColdStartCalibrationHarness harness;
    const auto first = harness.Run(candidate);
    const auto second = harness.Run(candidate);

    Require(first.Result == ColdStartResult::Timeout, "timeout result");
    Require(second.Result == ColdStartResult::Timeout, "second timeout result");
    Require(first.ProducedNativeFrames > 0, "native frames produced");
    Require(first.ProducedPcmFrames > 0, "PCM frames produced");
    Require(first.PcmNonFiniteSamples == 0, "finite PCM");
    Require(first.CleanupStarterDisabled, "starter cleanup");
    Require(first.CleanupIgnitionDisabled, "ignition cleanup");
    Require(first.CleanupSimulatorDestroyed, "simulator cleanup");
    Require(first.CleanupFixtureDestroyed, "fixture cleanup");

    const std::string firstJson =
        ColdStartCalibrationHarness::SerializeTraceJson(first);
    const std::string secondJson =
        ColdStartCalibrationHarness::SerializeTraceJson(second);
    Require(firstJson == secondJson, "trace determinism");
    Require(firstJson.find('\r') == std::string::npos, "LF trace");
    Require(
        firstJson.find("hostname") == std::string::npos,
        "no hostname");
    Require(
        firstJson.find("timestamp") == std::string::npos,
        "no timestamp");
}

} // namespace

int main() {
    try {
        TestDeterministicTimeoutTrace();
    } catch (const std::exception &) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
