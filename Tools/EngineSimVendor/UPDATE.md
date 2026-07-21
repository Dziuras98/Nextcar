# Updating NC-003B pinned source evidence

> Normal build and verification use only repository-contained vendored code and `Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim`. No network access, sibling checkout, submodule or SourceInputs tree is required.

The engine-sim and solver pins remain immutable unless a separate manager-approved update task changes them. For an explicit update:

1. obtain the approved pinned source outside build, test and runtime processes;
2. independently verify its repository, commit, tree and archive provenance;
3. derive the minimal fixture-source path set programmatically from the 118 parity records plus the pinned Subaru script and `es/objects/objects.mr`;
4. replace only the indexed files under `FixtureSourceSnapshot/engine-sim`;
5. regenerate `FixtureSourceSnapshot/SNAPSHOT.json` deterministically with UTF-8, LF, sorted keys and stable path order;
6. run `python Tools/EngineSimVendor/verify_fixture_source_snapshot.py`;
7. run `python Tools/EngineSimVendor/verify_subaru_ej25_fixture.py`;
8. run both mutation suites and the full Windows exact-head matrix.

`--source-root` on the fixture verifier is an explicit update/diagnostic override. It is not used by normal repository verification. Historical SourceInputs paths retained in `SOURCE_MANIFEST.json` identify the provenance of already-vendored files and are not active filesystem dependencies.

## Final Phase 0 closure validation evidence

Status at this commit:

```text
Phase 0 WIP — final closure candidate Windows-validated and evidence recorded;
exact evidence-head publication gate required before manager submission.

Phase 1 not started.
```

Target publication requires a separate full Windows exact-head run against this evidence commit.

Validated Checkpoint 2 and publication boundary:

```text
checkpoint SHA: 7cc91bf136554ca6a40bbbeb90a557ec4bef078c
checkpoint tree: b616b2ae12a51b4df83ba05858ce96cbbdce75e6
target SHA: 9bb6464123228f1d9638d467e7160a7cb2aa98c8
ahead: 2
behind: 0
workflow-fix SHA: a25c6af112e667c47ee77aaba33c5cd9d992ea6a
```

Closure-candidate Windows run and artifact:

```text
run ID: 29861088654
attempt: 1
artifact ID: 8507410452
artifact name: NC-003B-Phase0-final-closure-7cc91bf136554ca6a40bbbeb90a557ec4bef078c
GitHub digest: sha256:072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989
independent ZIP SHA-256: 072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989
file count: 64
hash-manifest entries: 63
hash-manifest: 63/63 PASS
```

The closure artifact identity is defined by GitHub artifact metadata: ID 8507410452, name `NC-003B-Phase0-final-closure-7cc91bf136554ca6a40bbbeb90a557ec4bef078c`, and ZIP SHA-256 `072228b427afa63f191c547fbb1a7aa8ad1b754fe2c1b31ceaf881159264f989`.

The artifact's inherited `toolchain-versions.json` `artifact_name` field uses a cold-start validator label and is non-authoritative.

Runner and full toolchain:

```text
runner: GitHub-hosted windows-2022
runner image: windows-2022/20260714.244.1
runner version: 2.336.0
operating system: Windows Server 2022 x64
Visual Studio: 17.14.37411.7
MSVC: Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35228 for x64
VC tools: 14.44.35207
Windows SDK: 10.0.26100.0
CMake: cmake version 3.31.6
Python: Python 3.12.10
Git: git version 2.55.0.windows.2
```

Boundary and dependency proofs:

```text
autocrlf-proof: PASS
boundary-proof: PASS
SourceInputs absence: PASS
SourceInputs directory absent: PASS
tracked SourceInputs count: 0
network scan: PASS
network scan hit count: 0
runtime WAV scan: PASS
runtime WAV scan hit count: 0
post-matrix cleanup: PASS
git status clean after matrix: PASS
temporary workflow absent from candidate tree: PASS
```

Validation matrix:

```text
repository validation: PASS
git diff --check: PASS
snapshot verifier: PASS
snapshot tests: 7/7 PASS
fixture mutation tests: 24/24 PASS
fixture parity: 118/118 PASS
IR mutation tests: 11/11 PASS
IR verifier: PASS
IR generator determinism: PASS
cold-start mutation tests: 15/15 PASS
cold-start verifier: PASS
cold-start generator determinism: PASS
profile/header consistency: PASS
MSVC x64 Release: PASS
MSVC x64 Debug /RTC1: PASS
MSVC x64 AddressSanitizer: PASS
CTest: PASS
direct runtime: PASS
selected trials: 10/10 PASS
timeout: PASS
stall: PASS
project warning count: 0
clean reproducibility: PASS
post-matrix cleanup: PASS
```
