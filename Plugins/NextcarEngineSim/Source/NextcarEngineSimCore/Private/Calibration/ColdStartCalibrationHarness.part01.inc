
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
