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
