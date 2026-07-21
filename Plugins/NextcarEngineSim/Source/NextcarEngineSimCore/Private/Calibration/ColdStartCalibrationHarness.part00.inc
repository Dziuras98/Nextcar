#include "ColdStartCalibrationHarness.h"

#include "SubaruEJ25AtgVideo2Fixture.h"

#include "combustion_chamber.h"
#include "engine.h"
#include "ignition_module.h"
#include "simulator.h"
#include "synthesizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace NextcarEngineSim::Phase0::Calibration {
namespace {

constexpr const char *ValidatedParitySourceSha =
    "542d3261efc3ef48c78f337d990793aea55dd7fb";
constexpr const char *EngineSimCommit =
    "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630";
constexpr const char *EngineSimTree =
    "0756dea0444ada160540685dd1d28fcd3ef4aac5";
constexpr const char *SolverCommit =
    "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3";
constexpr const char *FixtureSemanticContractSha256 =
    "3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00";
constexpr const char *IrWavSha256 =
    "df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48";
constexpr const char *GeneratedIrSha256 =
    "ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540";

bool IsFinite(double value) {
    return std::isfinite(value);
}

std::string EscapeJson(const std::string &value) {
    std::ostringstream output;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (ch < 0x20) {
                output << "\\u"
                       << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(ch)
                       << std::dec << std::setfill(' ');
            } else {
                output << static_cast<char>(ch);
            }
        }
    }
    return output.str();
}

void AppendDouble(std::ostringstream &output, double value) {
    if (!IsFinite(value)) {
        output << "null";
        return;
    }
    output << std::fixed << std::setprecision(9) << value;
}

void SetFailure(
    ColdStartTrace &trace,
    ColdStartResult result,
    const char *reason) {
    trace.Result = result;
    trace.FailureReason = reason;
}

} // namespace

const char *ColdStartCalibrationHarness::ResultName(ColdStartResult result) {
    switch (result) {
    case ColdStartResult::Success: return "Success";
    case ColdStartResult::Timeout: return "Timeout";
    case ColdStartResult::StallAfterDisengagement: return "StallAfterDisengagement";
    case ColdStartResult::NonFiniteRpm: return "NonFiniteRpm";
    case ColdStartResult::NonFinitePcm: return "NonFinitePcm";
    case ColdStartResult::ZeroProducedPcm: return "ZeroProducedPcm";
    case ColdStartResult::ZeroPostStartRms: return "ZeroPostStartRms";
    case ColdStartResult::InternalError: return "InternalError";
    }
    return "InternalError";
}

ColdStartTrace ColdStartCalibrationHarness::Run(
    const ColdStartCandidate &candidate) const {
    if (candidate.CandidateId.empty()) {
        throw std::invalid_argument("candidate ID must not be empty");
    }
    if (!IsFinite(candidate.StartupThrottle)
        || candidate.StartupThrottle < 0.0
        || candidate.StartupThrottle > 1.0
        || !IsFinite(candidate.StarterDisengagementRpm)
        || candidate.StarterDisengagementRpm < 0.0
        || !IsFinite(candidate.PostStarterMinimumRpm)
        || candidate.PostStarterMinimumRpm < 0.0
        || !IsFinite(candidate.StabilityWindowSeconds)
        || candidate.StabilityWindowSeconds < 0.0
        || !IsFinite(candidate.MaximumStartupSimulationSeconds)
        || candidate.MaximumStartupSimulationSeconds <= 0.0) {
        throw std::invalid_argument("candidate contains an invalid numeric value");
    }

    ColdStartTrace trace;
    trace.FixtureId = SubaruEJ25AtgVideo2Fixture::FixtureId;
    trace.ValidatedParitySourceSha = ValidatedParitySourceSha;
    trace.EngineSimCommit = EngineSimCommit;
    trace.EngineSimTree = EngineSimTree;
    trace.SolverCommit = SolverCommit;
    trace.FixtureSemanticContractSha256 = FixtureSemanticContractSha256;
    trace.IrWavSha256 = IrWavSha256;
    trace.GeneratedIrSha256 = GeneratedIrSha256;
    trace.Candidate = candidate;
    trace.Result = ColdStartResult::InternalError;
    trace.FailureReason = "InternalError";

    SubaruEJ25AtgVideo2Fixture fixture;
    Engine &engine = fixture.GetEngine();
    Simulator *simulator = nullptr;
    IgnitionModule *ignition = engine.getIgnitionModule();

    bool cleaned = false;
    auto cleanup = [&]() {
        if (cleaned) return;
        if (simulator != nullptr) {
            simulator->m_starterMotor.m_enabled = false;
            trace.CleanupStarterDisabled = !simulator->m_starterMotor.m_enabled;
        } else {
            trace.CleanupStarterDisabled = true;
        }

        if (ignition != nullptr) {
            ignition->m_enabled = false;
            trace.CleanupIgnitionDisabled = !ignition->m_enabled;
        } else {
            trace.CleanupIgnitionDisabled = true;
        }

        if (simulator != nullptr) {
            simulator->destroy();
            delete simulator;
            simulator = nullptr;
        }
        trace.CleanupSimulatorDestroyed = true;

        fixture.Destroy();
        trace.CleanupFixtureDestroyed = true;
        cleaned = true;
    };

    try {
        for (int cylinder = 0; cylinder < engine.getCylinderCount(); ++cylinder) {
            engine.getChamber(cylinder)->setDeterministicSeed(
                candidate.Seed + static_cast<std::uint32_t>(cylinder));
        }
