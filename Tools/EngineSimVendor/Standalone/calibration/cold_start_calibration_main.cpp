#include "ColdStartCalibrationHarness.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

using NextcarEngineSim::Phase0::Calibration::ColdStartCalibrationHarness;
using NextcarEngineSim::Phase0::Calibration::ColdStartCandidate;
using NextcarEngineSim::Phase0::Calibration::ColdStartResult;

const char *RequireValue(int argc, char **argv, int &index) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(
            std::string("missing value for ") + argv[index]);
    }
    ++index;
    return argv[index];
}

double ParseDouble(const char *value, const char *name) {
    errno = 0;
    char *end = nullptr;
    const double parsed = std::strtod(value, &end);
    if (errno != 0 || end == value || *end != '\0') {
        throw std::invalid_argument(
            std::string("invalid ") + name + ": " + value);
    }
    return parsed;
}

int ParseInt(const char *value, const char *name) {
    errno = 0;
    char *end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0'
        || parsed < std::numeric_limits<int>::min()
        || parsed > std::numeric_limits<int>::max()) {
        throw std::invalid_argument(
            std::string("invalid ") + name + ": " + value);
    }
    return static_cast<int>(parsed);
}

std::uint32_t ParseSeed(const char *value) {
    errno = 0;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 0);
    if (errno != 0 || end == value || *end != '\0'
        || parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(
            std::string("invalid seed: ") + value);
    }
    return static_cast<std::uint32_t>(parsed);
}

} // namespace

int main(int argc, char **argv) {
    try {
        ColdStartCandidate candidate;
        std::string outputPath;

        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--output") {
                outputPath = RequireValue(argc, argv, index);
            } else if (argument == "--candidate-id") {
                candidate.CandidateId =
                    RequireValue(argc, argv, index);
            } else if (argument == "--trial-index") {
                candidate.TrialIndex = ParseInt(
                    RequireValue(argc, argv, index),
                    "trial index");
            } else if (argument == "--seed") {
                candidate.Seed = ParseSeed(
                    RequireValue(argc, argv, index));
            } else if (argument == "--startup-throttle") {
                candidate.StartupThrottle = ParseDouble(
                    RequireValue(argc, argv, index),
                    "startup throttle");
            } else if (argument == "--disengagement-rpm") {
                candidate.StarterDisengagementRpm = ParseDouble(
                    RequireValue(argc, argv, index),
                    "starter disengagement RPM");
            } else if (argument == "--post-min-rpm") {
                candidate.PostStarterMinimumRpm = ParseDouble(
                    RequireValue(argc, argv, index),
                    "post-starter minimum RPM");
            } else if (argument == "--stability-window") {
                candidate.StabilityWindowSeconds = ParseDouble(
                    RequireValue(argc, argv, index),
                    "stability window");
            } else if (argument == "--maximum-time") {
                candidate.MaximumStartupSimulationSeconds =
                    ParseDouble(
                        RequireValue(argc, argv, index),
                        "maximum startup time");
            } else if (
                argument
                == "--disable-ignition-after-disengagement") {
                candidate.DisableIgnitionAfterDisengagement = true;
            } else {
                throw std::invalid_argument(
                    "unknown argument: " + argument);
            }
        }

        if (outputPath.empty()) {
            throw std::invalid_argument("--output is required");
        }

        ColdStartCalibrationHarness harness;
        const auto trace = harness.Run(candidate);
        ColdStartCalibrationHarness::WriteTraceJson(
            outputPath,
            trace);

        std::cout
            << "TRACE result="
            << ColdStartCalibrationHarness::ResultName(
                trace.Result)
            << " candidate="
            << candidate.CandidateId
            << " trial="
            << candidate.TrialIndex
            << " output="
            << outputPath
            << '\n';

        return trace.Result == ColdStartResult::InternalError
            ? EXIT_FAILURE
            : EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
