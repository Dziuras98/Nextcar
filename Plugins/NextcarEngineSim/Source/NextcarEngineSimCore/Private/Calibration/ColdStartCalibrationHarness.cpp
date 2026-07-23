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

        engine.setThrottle(candidate.StartupThrottle);
        ignition->m_enabled = true;

        simulator = engine.createSimulator();
        if (simulator == nullptr) {
            throw std::runtime_error("engine.createSimulator returned null");
        }
        simulator->synthesizer().setDeterministicSeed(candidate.Seed);
        simulator->m_starterMotor.m_enabled = true;

        double simulationTime = 0.0;
        double postRpmSum = 0.0;
        int postRpmCount = 0;
        double postPcmSquareSum = 0.0;
        std::uint64_t postPcmSampleCount = 0;
        double allPcmSquareSum = 0.0;
        std::uint64_t allPcmSampleCount = 0;
        double minimumPostRpm = std::numeric_limits<double>::infinity();

        const int maximumCycles = static_cast<int>(
            std::ceil(
                candidate.MaximumStartupSimulationSeconds
                / SimulationSliceSeconds)) + 1;

        for (int cycleIndex = 0; cycleIndex < maximumCycles; ++cycleIndex) {
            const bool ignitionEnabledDuringSimulation = ignition->m_enabled;
            const bool starterEnabledDuringSimulation =
                simulator->m_starterMotor.m_enabled;

            simulator->startFrame(SimulationSliceSeconds);
            while (simulator->simulateStep()) {
                // Deterministic simulation work is completed by simulateStep().
            }

            const int nativeFrames = simulator->simulationSteps();
            const int synthesizedFrames =
                simulator->endFrame(Simulator::MaximumSynchronousFrames);

            std::vector<std::int16_t> pcm(
                static_cast<std::size_t>(std::max(0, synthesizedFrames)));
            int consumedFrames = 0;
            if (synthesizedFrames > 0) {
                consumedFrames = simulator->readAudioOutput(
                    synthesizedFrames,
                    pcm.data());
            }
            if (consumedFrames != synthesizedFrames) {
                throw std::runtime_error(
                    "synchronous PCM accounting mismatch");
            }

            simulationTime += SimulationSliceSeconds;
            const double rpm = engine.getRpm();

            ColdStartCycleSample sample;
            sample.CycleIndex = cycleIndex;
            sample.SimulationTimeSeconds = simulationTime;
            sample.CurrentRpm = rpm;
            sample.IgnitionEnabled = ignitionEnabledDuringSimulation;
            sample.StarterEnabled = starterEnabledDuringSimulation;
            sample.ProducedNativeFrames = nativeFrames;
            sample.ProducedPcmFrames = consumedFrames;

            double cycleSquareSum = 0.0;
            double cyclePeak = 0.0;
            int nonFiniteSamples = 0;
            for (int index = 0; index < consumedFrames; ++index) {
                const double normalized =
                    static_cast<double>(pcm[static_cast<std::size_t>(index)])
                    / 32768.0;
                if (!IsFinite(normalized)) {
                    ++nonFiniteSamples;
                    continue;
                }
                cyclePeak = std::max(cyclePeak, std::abs(normalized));
                cycleSquareSum += normalized * normalized;
            }
            sample.PcmNonFiniteSamples = nonFiniteSamples;
            sample.PcmPeak = cyclePeak;
            sample.PcmRms = consumedFrames > 0
                ? std::sqrt(cycleSquareSum / consumedFrames)
                : 0.0;

            trace.ProducedNativeFrames += nativeFrames;
            trace.ProducedPcmFrames += consumedFrames;
            trace.PcmNonFiniteSamples += nonFiniteSamples;
            trace.PcmPeak = std::max(trace.PcmPeak, cyclePeak);
            allPcmSquareSum += cycleSquareSum;
            allPcmSampleCount += static_cast<std::uint64_t>(
                std::max(0, consumedFrames));

            if (!IsFinite(rpm)) {
                trace.Cycles.push_back(sample);
                SetFailure(
                    trace,
                    ColdStartResult::NonFiniteRpm,
                    "NonFiniteRpm");
                break;
            }
            if (nonFiniteSamples != 0 || !IsFinite(sample.PcmRms)) {
                trace.Cycles.push_back(sample);
                SetFailure(
                    trace,
                    ColdStartResult::NonFinitePcm,
                    "NonFinitePcm");
                break;
            }

            if (!trace.HasStarterDisengagement
                && rpm >= candidate.StarterDisengagementRpm) {
                trace.HasStarterDisengagement = true;
                trace.StarterDisengagementTimeSeconds = simulationTime;
                trace.StarterDisengagementRpmObserved = rpm;
                simulator->m_starterMotor.m_enabled = false;
                if (candidate.DisableIgnitionAfterDisengagement) {
                    ignition->m_enabled = false;
                }
            }

            if (trace.HasStarterDisengagement) {
                sample.HasStarterDisengagementRpmObserved = true;
                sample.StarterDisengagementRpmObserved =
                    trace.StarterDisengagementRpmObserved;

                if (!starterEnabledDuringSimulation) {
                    minimumPostRpm = std::min(minimumPostRpm, rpm);
                    postRpmSum += rpm;
                    ++postRpmCount;
                    postPcmSquareSum += cycleSquareSum;
                    postPcmSampleCount += static_cast<std::uint64_t>(
                        std::max(0, consumedFrames));
                }
            }
            trace.Cycles.push_back(sample);

            if (trace.HasStarterDisengagement
                && !starterEnabledDuringSimulation) {
                if (rpm < candidate.PostStarterMinimumRpm) {
                    SetFailure(
                        trace,
                        ColdStartResult::StallAfterDisengagement,
                        "StallAfterDisengagement");
                    break;
                }

                const double stableFor =
                    simulationTime
                    - trace.StarterDisengagementTimeSeconds;
                if (stableFor + 1e-12
                    >= candidate.StabilityWindowSeconds) {
                    if (trace.ProducedPcmFrames == 0) {
                        SetFailure(
                            trace,
                            ColdStartResult::ZeroProducedPcm,
                            "ZeroProducedPcm");
                    } else if (postPcmSampleCount == 0
                        || postPcmSquareSum <= 0.0) {
                        SetFailure(
                            trace,
                            ColdStartResult::ZeroPostStartRms,
                            "ZeroPostStartRms");
                    } else {
                        trace.Result = ColdStartResult::Success;
                        trace.FailureReason.clear();
                    }
                    break;
                }
            }

            if (simulationTime + 1e-12
                >= candidate.MaximumStartupSimulationSeconds) {
                SetFailure(
                    trace,
                    ColdStartResult::Timeout,
                    "Timeout");
                break;
            }
        }

        trace.PcmRms = allPcmSampleCount > 0
            ? std::sqrt(allPcmSquareSum / allPcmSampleCount)
            : 0.0;
        trace.PostStartPcmRms = postPcmSampleCount > 0
            ? std::sqrt(postPcmSquareSum / postPcmSampleCount)
            : 0.0;
        if (postRpmCount > 0) {
            trace.MinimumPostStarterRpm = minimumPostRpm;
            trace.MeanPostStarterRpm = postRpmSum / postRpmCount;
        }

        cleanup();
    } catch (...) {
        cleanup();
        throw;
    }

    return trace;
}

std::string ColdStartCalibrationHarness::SerializeTraceJson(
    const ColdStartTrace &trace) {
    std::ostringstream output;
    output << "{\n";
    output << "  \"schema_version\": " << trace.SchemaVersion << ",\n";
    output << "  \"fixture_id\": \"" << EscapeJson(trace.FixtureId) << "\",\n";
    output << "  \"validated_parity_source_sha\": \""
           << EscapeJson(trace.ValidatedParitySourceSha) << "\",\n";
    output << "  \"engine_sim_commit\": \""
           << EscapeJson(trace.EngineSimCommit) << "\",\n";
    output << "  \"engine_sim_tree\": \""
           << EscapeJson(trace.EngineSimTree) << "\",\n";
    output << "  \"solver_commit\": \""
           << EscapeJson(trace.SolverCommit) << "\",\n";
    output << "  \"fixture_semantic_contract_sha256\": \""
           << EscapeJson(trace.FixtureSemanticContractSha256) << "\",\n";
    output << "  \"ir_wav_sha256\": \""
           << EscapeJson(trace.IrWavSha256) << "\",\n";
    output << "  \"generated_ir_sha256\": \""
           << EscapeJson(trace.GeneratedIrSha256) << "\",\n";
    output << "  \"seed\": " << trace.Candidate.Seed << ",\n";
    output << "  \"candidate_id\": \""
           << EscapeJson(trace.Candidate.CandidateId) << "\",\n";
    output << "  \"trial_index\": " << trace.Candidate.TrialIndex << ",\n";
    output << "  \"startup_throttle\": ";
    AppendDouble(output, trace.Candidate.StartupThrottle);
    output << ",\n  \"starter_disengagement_rpm\": ";
    AppendDouble(output, trace.Candidate.StarterDisengagementRpm);
    output << ",\n  \"post_starter_minimum_rpm\": ";
    AppendDouble(output, trace.Candidate.PostStarterMinimumRpm);
    output << ",\n  \"stability_window_seconds\": ";
    AppendDouble(output, trace.Candidate.StabilityWindowSeconds);
    output << ",\n  \"maximum_startup_simulation_seconds\": ";
    AppendDouble(
        output,
        trace.Candidate.MaximumStartupSimulationSeconds);
    output << ",\n  \"starter_disengagement_time_seconds\": ";
    if (trace.HasStarterDisengagement) {
        AppendDouble(output, trace.StarterDisengagementTimeSeconds);
    } else {
        output << "null";
    }
    output << ",\n  \"starter_disengagement_rpm_observed\": ";
    if (trace.HasStarterDisengagement) {
        AppendDouble(output, trace.StarterDisengagementRpmObserved);
    } else {
        output << "null";
    }
    output << ",\n  \"minimum_post_starter_rpm\": ";
    AppendDouble(output, trace.MinimumPostStarterRpm);
    output << ",\n  \"mean_post_starter_rpm\": ";
    AppendDouble(output, trace.MeanPostStarterRpm);
    output << ",\n  \"produced_native_frames\": "
           << trace.ProducedNativeFrames << ",\n";
    output << "  \"produced_pcm_frames\": "
           << trace.ProducedPcmFrames << ",\n";
    output << "  \"pcm_non_finite_samples\": "
           << trace.PcmNonFiniteSamples << ",\n";
    output << "  \"pcm_peak\": ";
    AppendDouble(output, trace.PcmPeak);
    output << ",\n  \"pcm_rms\": ";
    AppendDouble(output, trace.PcmRms);
    output << ",\n  \"post_start_pcm_rms\": ";
    AppendDouble(output, trace.PostStartPcmRms);
    output << ",\n  \"result\": \""
           << ResultName(trace.Result) << "\",\n";
    output << "  \"failure_reason\": \""
           << EscapeJson(trace.FailureReason) << "\",\n";
    output << "  \"cleanup\": {\n";
    output << "    \"starter_disabled\": "
           << (trace.CleanupStarterDisabled ? "true" : "false") << ",\n";
    output << "    \"ignition_disabled\": "
           << (trace.CleanupIgnitionDisabled ? "true" : "false") << ",\n";
    output << "    \"simulator_destroyed\": "
           << (trace.CleanupSimulatorDestroyed ? "true" : "false") << ",\n";
    output << "    \"fixture_destroyed\": "
           << (trace.CleanupFixtureDestroyed ? "true" : "false") << "\n";
    output << "  },\n";
    output << "  \"cycles\": [\n";
    for (std::size_t index = 0; index < trace.Cycles.size(); ++index) {
        const ColdStartCycleSample &sample = trace.Cycles[index];
        output << "    {\n";
        output << "      \"cycle_index\": "
               << sample.CycleIndex << ",\n";
        output << "      \"simulation_time_seconds\": ";
        AppendDouble(output, sample.SimulationTimeSeconds);
        output << ",\n      \"ignition_enabled\": "
               << (sample.IgnitionEnabled ? "true" : "false") << ",\n";
        output << "      \"starter_enabled\": "
               << (sample.StarterEnabled ? "true" : "false") << ",\n";
        output << "      \"current_rpm\": ";
        AppendDouble(output, sample.CurrentRpm);
        output << ",\n      \"starter_disengagement_rpm_observed\": ";
        if (sample.HasStarterDisengagementRpmObserved) {
            AppendDouble(
                output,
                sample.StarterDisengagementRpmObserved);
        } else {
            output << "null";
        }
        output << ",\n      \"produced_native_frames\": "
               << sample.ProducedNativeFrames << ",\n";
        output << "      \"produced_pcm_frames\": "
               << sample.ProducedPcmFrames << ",\n";
        output << "      \"pcm_non_finite_samples\": "
               << sample.PcmNonFiniteSamples << ",\n";
        output << "      \"pcm_peak\": ";
        AppendDouble(output, sample.PcmPeak);
        output << ",\n      \"pcm_rms\": ";
        AppendDouble(output, sample.PcmRms);
        output << "\n    }";
        if (index + 1 != trace.Cycles.size()) output << ",";
        output << "\n";
    }
    output << "  ]\n";
    output << "}\n";
    return output.str();
}

void ColdStartCalibrationHarness::WriteTraceJson(
    const std::string &path,
    const ColdStartTrace &trace) {
    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(
            outputPath.parent_path());
    }
    std::ofstream stream(
        outputPath,
        std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error(
            "failed to open trace output path");
    }
    const std::string json = SerializeTraceJson(trace);
    stream.write(
        json.data(),
        static_cast<std::streamsize>(json.size()));
    if (!stream) {
        throw std::runtime_error("failed to write trace JSON");
    }
}

} // namespace NextcarEngineSim::Phase0::Calibration
