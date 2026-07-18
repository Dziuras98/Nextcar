param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Continue"
$env:GIT_TERMINAL_PROMPT = "0"
$env:GCM_INTERACTIVE = "Never"
Set-Location $ProjectRoot

Write-Host "NC003B_AUTH_PROBE_BEGIN"
$Gh = Get-Command gh -ErrorAction SilentlyContinue
if ($null -ne $Gh) {
    Write-Host "NC003B_GH_PATH=$($Gh.Source)"
    & gh auth status
    Write-Host "NC003B_GH_AUTH_EXIT=$LASTEXITCODE"
}
else {
    Write-Host "NC003B_GH_PATH=missing"
}

$Headers = @(git config --local --get-all http.https://github.com/.extraheader 2>$null)
Write-Host "NC003B_CHECKOUT_EXTRAHEADER_COUNT=$($Headers.Count)"
& git config --local --unset-all http.https://github.com/.extraheader 2>$null

& git push --dry-run origin HEAD:refs/heads/agent/nc003b-auth-probe
$DryRunExit = $LASTEXITCODE
Write-Host "NC003B_PUSH_DRY_RUN_EXIT=$DryRunExit"

foreach ($Header in $Headers) {
    & git config --local --add http.https://github.com/.extraheader $Header
}
Write-Host "NC003B_AUTH_PROBE_END"
exit 0
