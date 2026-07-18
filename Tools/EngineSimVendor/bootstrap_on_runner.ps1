param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"
$EngineCommit = "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630"
$SolverCommit = "e009f4ff1c9c4c5874e865e893cdb62e208fb2b3"
$ExpectedWavBlob = "6d3f8688e170cb6e5f4bfec42f580f3900514d72"
$WavRelativePath = "es/sound-library/new/minimal_muffling_02.wav"
$WorkRoot = Join-Path $env:TEMP "nextcar-nc003b-vendor-bootstrap"
$EngineCheckout = Join-Path $WorkRoot "engine-sim"
$SolverCheckout = Join-Path $WorkRoot "simple-2d-constraint-solver"
$BundleRoot = Join-Path $WorkRoot "bundle"
$BuildLogDir = Join-Path $ProjectRoot "Saved/BuildLogs"
$ArtifactPath = Join-Path $BuildLogDir "NC003B-trusted-vendor-inputs.zip"

Remove-Item -LiteralPath $WorkRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $WorkRoot, $BundleRoot, $BuildLogDir -Force | Out-Null

function Invoke-GitChecked {
    param([string]$WorkingDirectory, [string[]]$Arguments, [string]$Failure)
    Push-Location $WorkingDirectory
    try {
        & git @Arguments
        if ($LASTEXITCODE -ne 0) { throw "$Failure (git exit $LASTEXITCODE)" }
    }
    finally { Pop-Location }
}

& git clone --no-checkout https://github.com/Dziuras98/engine-sim.git $EngineCheckout
if ($LASTEXITCODE -ne 0) { throw "engine-sim clone failed" }
Invoke-GitChecked $EngineCheckout @("fetch", "--depth=1", "origin", $EngineCommit) "Pinned engine-sim fetch failed"
Invoke-GitChecked $EngineCheckout @("checkout", "--detach", $EngineCommit) "Pinned engine-sim checkout failed"
$ResolvedEngineCommit = (& git -C $EngineCheckout rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $ResolvedEngineCommit -ne $EngineCommit) {
    throw "Wrong engine-sim commit: $ResolvedEngineCommit"
}
$WavPath = Join-Path $EngineCheckout $WavRelativePath
$WavBlob = (& git -C $EngineCheckout hash-object --no-filters -- $WavRelativePath).Trim()
if ($LASTEXITCODE -ne 0 -or $WavBlob -ne $ExpectedWavBlob) {
    throw "Unexpected WAV blob: $WavBlob"
}
$WavSha256 = (Get-FileHash -LiteralPath $WavPath -Algorithm SHA256).Hash.ToLowerInvariant()
$WavItem = Get-Item -LiteralPath $WavPath

& git clone --no-checkout https://github.com/ange-yaghi/simple-2d-constraint-solver.git $SolverCheckout
if ($LASTEXITCODE -ne 0) { throw "solver clone failed" }
Invoke-GitChecked $SolverCheckout @("fetch", "--depth=1", "origin", $SolverCommit) "Pinned solver fetch failed"
Invoke-GitChecked $SolverCheckout @("checkout", "--detach", $SolverCommit) "Pinned solver checkout failed"
$ResolvedSolverCommit = (& git -C $SolverCheckout rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $ResolvedSolverCommit -ne $SolverCommit) {
    throw "Wrong solver commit: $ResolvedSolverCommit"
}

$EngineBundle = Join-Path $BundleRoot "engine-sim"
$SolverBundle = Join-Path $BundleRoot "simple-2d-constraint-solver"
New-Item -ItemType Directory -Path $EngineBundle, $SolverBundle -Force | Out-Null
function Copy-Relative {
    param([string]$SourceRoot, [string]$DestinationRoot, [string]$RelativePath)
    $SourcePath = Join-Path $SourceRoot $RelativePath
    $DestinationPath = Join-Path $DestinationRoot $RelativePath
    $DestinationParent = Split-Path -Parent $DestinationPath
    if (-not [string]::IsNullOrWhiteSpace($DestinationParent)) {
        New-Item -ItemType Directory -Path $DestinationParent -Force | Out-Null
    }
    Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Recurse -Force
}
foreach ($Path in @("include", "src", "es", "assets/engines/atg-video-2/01_subaru_ej25_eh.mr", "LICENSE")) {
    Copy-Relative $EngineCheckout $EngineBundle $Path
}
foreach ($Path in @("include", "src", "LICENSE")) {
    Copy-Relative $SolverCheckout $SolverBundle $Path
}

$Verification = [ordered]@{
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    engine_repository = "Dziuras98/engine-sim"
    engine_commit = $ResolvedEngineCommit
    solver_repository = "ange-yaghi/simple-2d-constraint-solver"
    solver_commit = $ResolvedSolverCommit
    wav_path = $WavRelativePath
    wav_git_blob = $WavBlob
    wav_sha256 = $WavSha256
    wav_size_bytes = $WavItem.Length
}
$Verification | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $BundleRoot "trusted-checkout-verification.json") -Encoding utf8

Remove-Item -LiteralPath $ArtifactPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $BundleRoot "*") -DestinationPath $ArtifactPath -CompressionLevel Optimal
$ArtifactSha256 = (Get-FileHash -LiteralPath $ArtifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host "NC003B_BOOTSTRAP_ARTIFACT=$ArtifactPath"
Write-Host "NC003B_BOOTSTRAP_ARTIFACT_SHA256=$ArtifactSha256"
Write-Host "NC003B_WAV_BLOB=$WavBlob"
Write-Host "NC003B_WAV_SHA256=$WavSha256"
