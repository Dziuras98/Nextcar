## Summary

Describe the smallest implemented change and its user-visible or developer-visible effect.

## Evidence

List the verified repository facts, user requirements, measurements, test output, or authoritative primary documentation that justify this change.

- [ ] I inspected the relevant existing code and configuration.
- [ ] Every technical claim in this PR is supported by evidence.
- [ ] No material requirement or behavior remains uncertain.
- [ ] Any material uncertainty was resolved with the user before implementation.

## Tests

List every exact command that was run and its result.

```text
python scripts/validate_repository.py
# Result: replace with the actual result
```

- [ ] Repository validation passed.
- [ ] All area-specific tests passed.
- [ ] A required build was executed and passed, or this change cannot affect compilation.
- [ ] No required test was skipped or unavailable.

## Merge readiness

- [ ] The PR targets `main`.
- [ ] The diff contains no unrelated changes.
- [ ] The change is ready to be merged immediately after checks pass.
