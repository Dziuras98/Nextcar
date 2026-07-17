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

## 4. Manager role and parallel delegation

- When an agent is explicitly instructed to act as the manager, its primary responsibility is orchestration, repository review, change control, and verification rather than direct feature implementation.
- Before planning or delegating work, the manager must read and analyze the entire `AGENTS.md`, the complete current contents of `docs/manager-history.md`, the complete reachable repository history, the current repository state, and all existing notes from previous changes.
- The complete reachable repository history must be inspected across all available branches and tags. Do not rely only on the latest commit, a summary, cached context, or the most recent manager entry.
- The manager must review the relevant code, configuration, documentation, tests, open changes, and integration state before issuing implementation instructions.
- The manager must identify the task dependency graph, divide the work into the maximum number of independent workstreams, and dispatch as many programmer agents as can safely work in parallel.
- Every programmer-agent command must define the objective, exact file or component ownership, dependencies, acceptance criteria, required tests, evidence to collect, prohibited out-of-scope changes, and the expected handoff format.
- Do not assign overlapping writable scopes to parallel programmer agents unless the manager defines an explicit coordination and integration plan. Shared integration files must be reserved for the manager or handled sequentially.
- The manager must review every returned change, diff, test result, and claim; resolve conflicts; verify integration behavior; and reject incomplete or unsupported work.
- The manager is responsible for documenting all delegated tasks, decisions, changes, test results, unresolved risks, and follow-up work.

## 5. Persistent manager history

- `docs/manager-history.md` is the canonical, append-only record of manager activity and notes from previous changes.
- At the start of every manager run, read and analyze the entire file. Before completing every manager run, append a new entry describing the final state of that run.
- A manager entry is mandatory even when no code changed, the task was blocked, all work was delegated, or the result was only analysis.
- Each entry must include: timestamp; user request; baseline branch and commit; repository history reviewed; repository state found; workstream decomposition and programmer-agent assignments; files and behavior changed; evidence and exact tests with results; decisions and integration notes; unresolved risks or blockers; and next steps.
- Never delete, reorder, silently rewrite, or summarize away previous entries. Corrections must be appended as new traceable notes.
- Update the manager history in the same pull request or commit as the managed changes, after integration, so the entry records the actual final diff and validation results.
- A manager must not claim the task is complete until the new history entry has been persisted.

## 6. Merge and branch lifecycle policy

- `main` is the sole integration branch.
- Work on a short-lived branch. Do not commit feature or fix work directly to `main`.
- Open a pull request targeting `main`.
- Merge the pull request immediately after all required tests pass and the factual basis has been verified.
- Do not leave a tested, approved change waiting without a concrete blocker.
- Prefer squash merge unless preserving separate commits is technically necessary.
- After a pull request is successfully merged, automatically delete its source branch when the branch belongs to this repository.
- Never automatically delete `main`, `master`, a protected branch, an unmerged branch, or a branch from a fork.

## 7. Required task sequence

1. Inspect the repository and identify the exact files and behavior involved.
2. State or document the verified facts supporting the proposed change.
3. Ask the user about every material uncertainty before editing.
4. If acting as manager, complete the history review, decomposition, delegation, review, and persistent logging requirements in sections 4 and 5.
5. Implement the smallest change that satisfies the confirmed requirement.
6. Run `python scripts/validate_repository.py` and all area-specific tests.
7. Review the final diff for unrelated changes and unsupported claims.
8. Open a pull request to `main` with completed **Evidence** and **Tests** sections.
9. Merge immediately after all checks pass.
10. Confirm that the merged pull request source branch was deleted automatically; delete it manually only when the automation cannot do so and the branch is safe to remove.

## 8. Prohibited practices

- No fabricated test results, build results, benchmarks, citations, file contents, API behavior, or completion claims.
- No merging because a change merely looks correct.
- No replacing required testing with static inspection unless the affected behavior is purely static and the validation demonstrably covers it.
- No speculative compatibility claims.
- No unrelated refactors bundled with a requested change.
- No destructive repository operation without explicit user authorization, except automatic deletion of a successfully merged same-repository work branch under the policy above.
- No manager completion without reading the full repository history and previous notes, maximizing safe parallel delegation, reviewing agent output, and appending the manager history entry.

If a required test environment is unavailable, report the blocker accurately and do not merge the affected change.
