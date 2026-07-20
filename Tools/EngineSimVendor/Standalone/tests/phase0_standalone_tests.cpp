#include "SubaruEJ25AtgVideo2Fixture.h"
#include "MinimalMuffling02ImpulseResponse.generated.h"

#include "combustion_chamber.h"
#include "force_generator.h"
#include "gauss_seidel_sle_solver.h"
#include "optimized_nsv_rigid_body_system.h"
#include "piston_engine_simulator.h"
#include "ring_buffer.h"
#include "synthesizer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

void TestRingBuffer() {
    RingBuffer<int> buffer;
    buffer.initialize(4);
    Require(buffer.empty(), "new ring buffer must be empty");
    Require(buffer.size() == 0, "new ring buffer size");
    Require(buffer.freeSpace() == 4, "new ring buffer free space");

    Require(buffer.write(10), "partial write 10");
    Require(buffer.write(20), "partial write 20");
    Require(buffer.size() == 2 && !buffer.empty() && !buffer.full(), "partial occupancy");
    Require(buffer.read(0) == 10 && buffer.read(1) == 20, "partial read order");

    Require(buffer.write(30), "write 30");
    Require(buffer.write(40), "write 40");
    Require(buffer.full(), "exactly full must report full");
    Require(!buffer.empty(), "exactly full must not report empty");
    Require(buffer.size() == 4 && buffer.freeSpace() == 0, "full occupancy");
    Require(!buffer.write(50), "write after full must be rejected");

    std::array<int, 2> first{};
    buffer.readAndRemove(first.size(), first.data());
    Require(first[0] == 10 && first[1] == 20, "read and remove order");
    Require(buffer.size() == 2, "occupancy after remove");

    Require(buffer.write(50), "write after freeing full buffer");
    Require(buffer.write(60), "wraparound write");
    Require(buffer.full(), "wraparound full");
    std::array<int, 4> wrapped{};
    buffer.read(wrapped.size(), wrapped.data());
    Require(wrapped == std::array<int, 4>{30, 40, 50, 60}, "wraparound order");

    Require(buffer.removeBeginning(3), "remove beginning succeeds");
    Require(buffer.size() == 1 && buffer.read(0) == 60, "remove beginning occupancy");
    std::array<int, 2> rejected{{111, 222}};
    Require(!buffer.read(2, rejected.data()), "over-read must be rejected");
    Require(rejected == std::array<int, 2>{111, 222}, "rejected read must not write target");
    Require(!buffer.removeBeginning(2), "over-remove must be rejected");
    Require(buffer.size() == 1 && buffer.read(0) == 60, "rejected operations preserve occupancy");
    Require(buffer.readAndRemove(1, wrapped.data()), "final read and remove");
    Require(buffer.empty() && buffer.size() == 0, "empty after final remove");
    buffer.destroy();
}

void InitializeSynthesizer(Synthesizer &synthesizer, int inputCapacity = 4096, int outputCapacity = 4096) {
    Synthesizer::Parameters parameters;
    parameters.inputChannelCount = 1;
    parameters.inputBufferSize = inputCapacity;
    parameters.audioBufferSize = outputCapacity;
    parameters.inputSampleRate = 44100.0f;
    parameters.audioSampleRate = 44100.0f;
    parameters.initialAudioParameters.airNoise = 0.0f;
    parameters.initialAudioParameters.inputSampleNoise = 0.0f;
    parameters.initialAudioParameters.convolution = 1.0f;
    parameters.initialAudioParameters.dF_F_mix = 0.0f;
    synthesizer.initialize(parameters);
    constexpr std::int16_t identity[] = {32767};
    synthesizer.initializeImpulseResponse(identity, 1, 1.0f, 0);
    synthesizer.setDeterministicSeed(0x12345678u);
}

void WriteSynthFrames(Synthesizer &synthesizer, int count, double value = 1000.0) {
    const double input[] = {value};
    for (int i = 0; i < count; ++i) synthesizer.writeInput(input);
}

void TestSynchronousSynthesizer() {
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer);
        Require(synthesizer.processSynchronous(2000) == 0, "zero pending frames");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer);
        WriteSynthFrames(synthesizer, 37);
        Require(synthesizer.getInputBufferedFrames() == 37, "partial pending count");
        Require(synthesizer.processSynchronous(2000) == 37, "partial production count");
        Require(synthesizer.getAudioBufferedFrames() == 37, "partial audio occupancy");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer, 2500, 2500);
        WriteSynthFrames(synthesizer, 2000);
        Require(synthesizer.processSynchronous(2000) == 2000, "exact cap production");
        Require(synthesizer.getAudioBufferedFrames() == 2000, "exact cap occupancy");
        synthesizer.destroy();
    }
    {
        Synthesizer synthesizer;
        InitializeSynthesizer(synthesizer, 4096, 4096);
        WriteSynthFrames(synthesizer, 2501);
        Require(synthesizer.processSynchronous(2000) == 2000, "over cap production");
        Require(synthesizer.getInputBufferedFrames() == 501, "over cap pending remainder");
        Require(synthesizer.getAudioBufferedFrames() == 2000, "over cap output occupancy");
        synthesizer.destroy();
    }
}

class ChamberRngProbe : public CombustionChamber {
public:
    double Next() { return nextUnitRandom(); }
};

void TestDeterministicRng() {
    ChamberRngProbe first;
    ChamberRngProbe second;
    ChamberRngProbe different;
    first.setDeterministicSeed(0x10203040u);
    second.setDeterministicSeed(0x10203040u);
    different.setDeterministicSeed(0x10203041u);

    std::array<double, 16> sequence{};
    for (double &value : sequence) value = first.Next();
    for (double value : sequence) Require(value == second.Next(), "same seed sequence mismatch");

    different.setDeterministicSeed(0x10203041u);
    bool differs = false;
    for (double value : sequence) differs = differs || value != different.Next();
    Require(differs, "different seeds must differ");

    ChamberRngProbe isolatedA;
    ChamberRngProbe isolatedB;
    isolatedA.setDeterministicSeed(77u);
    isolatedB.setDeterministicSeed(77u);
    const double a0 = isolatedA.Next();
    (void)isolatedA.Next();
    const double b0 = isolatedB.Next();
    Require(a0 == b0, "instances must not share mutable RNG state");
}

class ConstantForce final : public atg_scs::ForceGenerator {
public:
    void apply(atg_scs::SystemState *system) override {
        system->applyForce(0.0, 0.0, 10.0, 0.0, 0);
    }
};

void TestSolverRuntime() {
    atg_scs::OptimizedNsvRigidBodySystem system;
    system.initialize(new atg_scs::GaussSeidelSleSolver);
    atg_scs::RigidBody body;
    body.reset();
    body.m = 2.0;
    body.I = 1.0;
    ConstantForce force;
    system.addRigidBody(&body);
    system.addForceGenerator(&force);
    system.process(0.1, 1);
    Require(std::isfinite(body.p_x) && std::isfinite(body.v_x), "solver finite state");
    Require(body.v_x > 0.0 && body.p_x > 0.0, "solver force integration");
    system.reset();
}

void TestGeneratedImpulseResponse() {
    using namespace NextcarEngineSim::Generated;
    Require(MinimalMuffling02SampleCount > 1, "generated impulse response must not be identity PCM");
    Require(MinimalMuffling02SampleRate == 44100, "generated impulse response sample rate");
    Require(MinimalMuffling02Channels == 1, "generated impulse response channel count");
    Require(MinimalMuffling02BitsPerSample == 16, "generated impulse response bit depth");
    Require(
        MinimalMuffling02ByteCount
            == MinimalMuffling02SampleCount * sizeof(MinimalMuffling02Pcm[0]),
        "generated impulse response byte count");
    Require(
        !(MinimalMuffling02SampleCount == 1 && MinimalMuffling02Pcm[0] == 32767),
        "fixture impulse response must not be the identity placeholder");

    std::size_t accessibleSamples = 0;
    std::int64_t checksum = 0;
    for (std::size_t i = 0; i < MinimalMuffling02SampleCount; ++i) {
        checksum += MinimalMuffling02Pcm[i];
        ++accessibleSamples;
    }
    Require(accessibleSamples == MinimalMuffling02SampleCount, "full generated PCM must be accessible");
    Require(checksum != 32767, "full generated PCM must differ from identity PCM");
}

void TestFixtureAndSimulator() {
    NextcarEngineSim::Phase0::SubaruEJ25AtgVideo2Fixture fixture;
    Engine &engine = fixture.GetEngine();
    Require(engine.getCylinderCount() == 4, "fixture cylinder count");
    Require(engine.getCylinderBankCount() == 2, "fixture bank count");
    Require(engine.getExhaustSystemCount() == 1, "fixture exhaust count");
    Require(engine.getDisplacement() > 0.0 && std::isfinite(engine.getDisplacement()), "fixture displacement");

    Simulator *simulator = engine.createSimulator();
    Require(simulator != nullptr, "headless simulator creation");
    Require(simulator->getEngine() == &engine, "headless simulator load");

    const double input[] = {1.0};
    for (int i = 0; i < 2501; ++i) simulator->synthesizer().writeInput(input);
    const int pendingBefore = simulator->synthesizer().getInputBufferedFrames();
    Require(pendingBefore > Simulator::MaximumSynchronousFrames, "Simulator cap precondition");
    const int produced = simulator->endFrame(5000);
    Require(produced == Simulator::MaximumSynchronousFrames, "Simulator::endFrame cap");
    Require(
        simulator->synthesizer().getInputBufferedFrames()
            == pendingBefore - Simulator::MaximumSynchronousFrames,
        "Simulator cap remainder");

    simulator->destroy();
    delete simulator;
    fixture.Destroy();
}

} // namespace

int main() {
    try {
        TestRingBuffer();
        std::cout << "PASS ring-buffer\n";
        TestSynchronousSynthesizer();
        std::cout << "PASS synchronous-synthesizer\n";
        TestDeterministicRng();
        std::cout << "PASS deterministic-rng\n";
        TestSolverRuntime();
        std::cout << "PASS solver-runtime\n";
        TestGeneratedImpulseResponse();
        std::cout << "PASS impulse-response-integrity\n";
        TestFixtureAndSimulator();
        std::cout << "PASS fixture-simulator-smoke\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
