[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$canaryPath = Join-Path `
    $env:RUNNER_TEMP `
    'nc003b-powershell-canary.txt'

$policyPath = Join-Path `
    $env:EVIDENCE `
    'powershell-execution-policy.txt'

Get-ExecutionPolicy -List |
    Format-Table -AutoSize |
    Out-String |
    Set-Content -Encoding utf8 -LiteralPath $policyPath

@(
    "edition=$($PSVersionTable.PSEdition)"
    "version=$($PSVersionTable.PSVersion)"
    "executable=$((Get-Process -Id $PID).Path)"
    "process_execution_policy=$(Get-ExecutionPolicy -Scope Process)"
    "effective_execution_policy=$(Get-ExecutionPolicy)"
) |
    Set-Content -Encoding utf8 `
        -LiteralPath (Join-Path $env:EVIDENCE 'powershell-session.txt')

[System.IO.File]::WriteAllText(
    $canaryPath,
    'POWERSHELL_EXECUTED',
    [System.Text.UTF8Encoding]::new($false)
)

Write-Host 'POWERSHELL_CANARY=CREATED'
