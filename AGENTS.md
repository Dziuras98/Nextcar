# Repository execution policy

These instructions apply to every task and every file in this repository.

## 1. Evidence and factual grounding

- Base every change on verified facts from the repository, user-provided requirements, reproducible measurements, test output, or authoritative primary documentation.
- Inspect the relevant code, configuration, history, and existing tests before changing them.
- Do not present assumptions, guesses, inferred APIs, file paths, engine behavior, or compatibility claims as facts.
- Record the factual basis for the change in the pull request under **Evidence**.
- When external technical information is required, prefer official Unreal Engine, platform, compiler, library, or standards documentation.

## 2. Uncertainty requires user input

- If any material requirement, target behavior, compatibility constraint, acceptance criterion, repository state, or destructive action is uncertain, stop and ask the user before modifying the repository.
- Do not silently choose between materially different implementations.
- Minor implementation details may only be selected without asking when they are directly implied by existing code, documented project conventions, or a verified standard practice and do not change user-visible behavior.

## 3. Tests are mandatory

- Every change must have an appropriate validation plan before implementation.
- Run all tests relevant to the changed area, including the repository validation script.
- For Unreal Engine code, a successful editor or target build is required when the change can affect compilation. Run relevant Unreal Automation Tests when they exist or add them when the behavior can be tested automatically.
- A test is not considered passed when it was skipped, unavailable, not run, or only reasoned about.
- Do not merge code with failing tests, unexecuted required tests, unresolved warnings that indicate a defect, or unverifiable behavior.
- Include exact commands and results in the pull request under **Tests**.

## 4. Merge and branch lifecycle policy

- `main` is the sole integration branch.
- Work on a short-lived branch. Do not commit feature or fix work directly to `main`.
- Open a pull request targeting `main`.
- Merge the pull request immediately after all required tests pass and the factual basis has been verified.
- Do not leave a tested, approved change waiting without a concrete blocker.
- Prefer squash merge unless preserving separate commits is technically necessary.
- After a pull request is successfully merged, automatically delete its source branch when the branch belongs to this repository.
- Never automatically delete `main`, `master`, a protected branch, an unmerged branch, or a branch from a fork.

## 5. Required task sequence

1. Inspect the repository and identify the exact files and behavior involved.
2. State or document the verified facts supporting the proposed change.
3. Ask the user about every material uncertainty before editing.
4. Implement the smallest change that satisfies the confirmed requirement.
5. Run `python scripts/validate_repository.py` and all area-specific tests.
6. Review the final diff for unrelated changes and unsupported claims.
7. Open a pull request to `main` with completed **Evidence** and **Tests** sections.
8. Merge immediately after all checks pass.
9. Confirm that the merged pull request source branch was deleted automatically; delete it manually only when the automation cannot do so and the branch is safe to remove.

## 6. Prohibited practices

- No fabricated test results, build results, benchmarks, citations, file contents, API behavior, or completion claims.
- No merging because a change merely looks correct.
- No replacing required testing with static inspection unless the affected behavior is purely static and the validation demonstrably covers it.
- No speculative compatibility claims.
- No unrelated refactors bundled with a requested change.
- No destructive repository operation without explicit user authorization, except automatic deletion of a successfully merged same-repository work branch under the policy above.

If a required test environment is unavailable, report the blocker accurately and do not merge the affected change.
