#pragma once

#include "SubaruEJ25AtgVideo2FixtureSpec.h"

#include "engine.h"
#include "camshaft.h"
#include "standard_valvetrain.h"
#include "impulse_response.h"
#include "function.h"

#include <array>
#include <cstdint>

namespace NextcarEngineSim::Phase0 {

class SubaruEJ25AtgVideo2Fixture {
public:
    static constexpr const char *FixtureId = "SubaruEJ25AtgVideo2";
    static constexpr const char *FixtureSlug = "subaru_ej25_atg_video_2_01";
    static constexpr int SimulationFrequencyHz = 20000;
    static constexpr std::uint32_t DeterministicSeed = 0x4E433033u;

    SubaruEJ25AtgVideo2Fixture();
    ~SubaruEJ25AtgVideo2Fixture();
    SubaruEJ25AtgVideo2Fixture(const SubaruEJ25AtgVideo2Fixture &) = delete;
    SubaruEJ25AtgVideo2Fixture &operator=(const SubaruEJ25AtgVideo2Fixture &) = delete;

    Engine &GetEngine() { return EngineInstance; }
    const Engine &GetEngine() const { return EngineInstance; }
    const FixtureTranscriptionSnapshot &GetTranscriptionSnapshot() const { return Snapshot; }
    void Destroy();

private:
    static void AddHarmonicCamLobe(Function &Target, double DurationAt50Thou, double Gamma, double Lift, int Steps);
    void BuildFunctions();
    void BuildEngine();

    FixtureTranscriptionSnapshot Snapshot;
    Engine EngineInstance;
    ImpulseResponse Impulse;
    Function FuelTurbulence;
    Function MeanPistonSpeedTurbulence;
    Function IntakeFlow;
    Function ExhaustFlow;
    Function IntakeLobe;
    Function ExhaustLobe;
    Function TimingCurve;
    std::array<Camshaft, 4> Camshafts;
    std::array<StandardValvetrain, 2> Valvetrains;
    bool IsDestroyed = false;
};

} // namespace NextcarEngineSim::Phase0
