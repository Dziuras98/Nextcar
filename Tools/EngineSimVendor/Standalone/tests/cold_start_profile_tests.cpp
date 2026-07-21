#include "SubaruEJ25ColdStartProfile.generated.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

using namespace NextcarEngineSim::Generated;

void Require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void TestGeneratedProfileConstants() {
    Require(
        std::strcmp(
            SubaruEJ25ColdStartProfileFixtureId,
            "SubaruEJ25AtgVideo2") == 0,
        "fixture ID");
    Require(SubaruEJ25ColdStartProfileTrialCount >= 10, "trial count");
    Require(SubaruEJ25ColdStartProfileSuccessCount
        == SubaruEJ25ColdStartProfileTrialCount, "success count");
    Require(
        SubaruEJ25ColdStartProfileStartupThrottle >= 0.0
            && SubaruEJ25ColdStartProfileStartupThrottle <= 1.0,
        "startup throttle");
    Require(
        SubaruEJ25ColdStartProfileStarterDisengagementRpm > 0.0,
        "disengagement RPM");
    Require(
        SubaruEJ25ColdStartProfilePostStarterMinimumRpm > 0.0,
        "minimum RPM");
    Require(
        SubaruEJ25ColdStartProfileStabilityWindowSeconds > 0.0,
        "stability window");
    Require(
        SubaruEJ25ColdStartProfileMaximumStartupSimulationSeconds > 0.0,
        "maximum startup time");
    Require(
        std::isfinite(SubaruEJ25ColdStartProfileStartupThrottle),
        "finite throttle");
    Require(
        std::strlen(SubaruEJ25ColdStartProfilePayloadSha256) == 64,
        "payload hash");
}

} // namespace

int main() {
    try {
        TestGeneratedProfileConstants();
    } catch (const std::exception &) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
