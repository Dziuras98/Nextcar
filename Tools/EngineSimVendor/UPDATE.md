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
