[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$stage = 'initialize publication'

try {
    $stage = 'working tree cleanliness'
    if (@(git status --short).Count -ne 0) {
        throw 'Working tree dirty before result push'
    }

    $stage = 'result branch absence'
    $existing = @(git ls-remote --heads origin "refs/heads/$env:RESULT_BRANCH")
    if ($LASTEXITCODE -ne 0) {
        throw 'Result branch probe failed'
    }
    if ($existing.Count -ne 0) {
        throw 'Result branch appeared before controlled push'
    }

    $stage = 'result branch push'
    git push origin "HEAD:refs/heads/$env:RESULT_BRANCH"
    if ($LASTEXITCODE -ne 0) {
        throw 'Result branch push failed'
    }

    $stage = 'remote result head verification'
    $remote = @(git ls-remote --heads origin "refs/heads/$env:RESULT_BRANCH")
    if (
        $remote.Count -ne 1 `
        -or ($remote[0] -split '\s+')[0] -ne $env:FINAL_HEAD
    ) {
        throw 'Remote result branch identity mismatch'
    }

    $stage = 'remote result tree verification'
    $remoteTree = (git rev-parse "$($env:FINAL_HEAD)^{tree}").Trim()
    if ($LASTEXITCODE -ne 0) {
        throw 'Remote result tree resolution failed'
    }
    if ($remoteTree -ne $env:FINAL_TREE) {
        throw 'Remote result tree identity mismatch'
    }

    @(
        "head=$env:FINAL_HEAD"
        "tree=$env:FINAL_TREE"
        'force=false'
        "run_id=$env:GITHUB_RUN_ID"
    ) |
        Set-Content -Encoding utf8 `
            -LiteralPath (Join-Path $env:EVIDENCE 'published-branch.txt')

    @(
        'PUBLISHED — WINDOWS EXACT-HEAD VALIDATED'
        "head=$env:FINAL_HEAD"
        "tree=$env:FINAL_TREE"
        'force=false'
        "run_id=$env:GITHUB_RUN_ID"
    ) |
        Set-Content -Encoding utf8 `
            -LiteralPath (Join-Path $env:EVIDENCE 'final-status.txt')
}
catch {
    $reason = "STOP — result publication: $stage: $($_.Exception.Message)"
    $reason |
        Set-Content -Encoding utf8 `
            -LiteralPath (Join-Path $env:EVIDENCE 'final-status.txt')
    throw
}
