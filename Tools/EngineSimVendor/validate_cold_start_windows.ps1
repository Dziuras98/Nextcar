[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateSha,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Discovery', 'Selected')]
    [string]$Phase,

    [string]$SelectedCandidatePath = '',

    [string]$ArtifactRoot = 'nc003b-cold-start-artifact'
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
Set-StrictMode -Version Latest

$RepositoryRoot = (Get-Location).Path
Set-Location $RepositoryRoot
$ArtifactPath = Join-Path $RepositoryRoot $ArtifactRoot
$LogPath = Join-Path $ArtifactPath 'logs'
New-Item -ItemType Directory -Force -Path $LogPath | Out-Null

$Failure = $null
$ProjectWarningCount = -1
$CleanReproducibility = 'NOT RUN'
$ReleaseResult = 'NOT RUN'
$DebugResult = 'NOT RUN'
$AsanResult = 'NOT RUN'
$SelectedTrialsResult = 'NOT RUN'
$TimeoutResult = 'NOT RUN'
$StallResult = 'NOT RUN'
$PythonTests = 'NOT RUN'

function Compact([string]$Value) {
    $Text = $Value `
        -replace [regex]::Escape($RepositoryRoot), '<repo>' `
        -replace '[\r\n]+', ' ' `
        -replace '\s+', ' '
    $Text = $Text.Trim()
    if ($Text.Length -gt 500) {
        return $Text.Substring(0, 500)
    }
    return $Text
}

function Invoke-Logged(
    [string]$Name,
    [string]$LogName,
    [scriptblock]$Command
) {
    $Output = @(& $Command 2>&1)
    $ExitCode = $LASTEXITCODE
    $Output | Tee-Object -FilePath (Join-Path $LogPath $LogName) | Out-Host
    if ($ExitCode -ne 0) {
        $Tail = @(
            $Output |
                ForEach-Object { ([string]$_).Trim() } |
                Where-Object { $_ } |
                Select-Object -Last 12
        )
        throw "$Name failed with exit code $ExitCode -- $(Compact ($Tail -join ' / '))"
    }
    return ,$Output
}

function Configure([string]$Directory, [string[]]$Extra = @()) {
    $Arguments = @(
        '-S', 'Tools/EngineSimVendor/Standalone',
        '-B', $Directory,
        '-G', 'Visual Studio 17 2022',
        '-A', 'x64',
        '-T', 'v143'
    ) + $Extra
    Invoke-Logged "$Directory configure" "$Directory-configure.log" {
        cmake @Arguments
    } | Out-Null
}

function Build(
    [string]$Directory,
    [string]$Configuration,
    [string]$Prefix
) {
    Invoke-Logged "$Prefix build" "$Prefix-build.log" {
        cmake --build $Directory --config $Configuration --verbose
    } | Out-Null
}

function Test-CMake(
    [string]$Directory,
    [string]$Configuration,
    [string]$Prefix
) {
    Invoke-Logged "$Prefix CTest" "$Prefix-ctest.log" {
        ctest --test-dir $Directory -C $Configuration --output-on-failure
    } | Out-Null
}

function Get-Executable(
    [string]$Directory,
    [string]$Configuration,
    [string]$Name
) {
    return Join-Path $RepositoryRoot "$Directory/$Configuration/$Name.exe"
}

function Write-HashManifest([string]$Root, [string]$OutputPath) {
    $ResolvedRoot = (Resolve-Path $Root).Path
    $Entries = @(
        Get-ChildItem -Path $ResolvedRoot -File -Recurse |
            Where-Object { $_.FullName -ne $OutputPath } |
            Sort-Object FullName |
            ForEach-Object {
                [ordered]@{
                    path = $_.FullName.Substring($ResolvedRoot.Length + 1).Replace('\', '/')
                    sha256 = (Get-FileHash -Algorithm SHA256 -Path $_.FullName).Hash.ToLowerInvariant()
                    size = $_.Length
                }
            }
    )
    $Json = ConvertTo-Json -InputObject $Entries -Depth 4
    [System.IO.File]::WriteAllText(
        $OutputPath,
        $Json + "`n",
        [System.Text.UTF8Encoding]::new($false)
    )
}

try {
    git config --local core.autocrlf false
    git config --local core.eol lf

    $ActualSha = (git rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $ActualSha -ne $CandidateSha) {
        throw "expected candidate $CandidateSha, checked out $ActualSha"
    }
    if (git status --porcelain) {
        throw 'candidate checkout is not clean'
    }

    $PythonVersion = ((python --version 2>&1) -join ' ').Trim()
    $CMakeVersion = ((cmake --version | Select-Object -First 1) -join ' ').Trim()
    $GitVersion = ((git --version) -join ' ').Trim()

    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $VsWhere)) {
        throw 'vswhere.exe not found'
    }
    $VisualStudioPath = (
        & $VsWhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
    ).Trim()
    $VisualStudioVersion = (
        & $VsWhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationVersion
    ).Trim()
    if (-not $VisualStudioPath -or -not $VisualStudioVersion.StartsWith('17.')) {
        throw "Visual Studio 2022 C++ tools not found: $VisualStudioVersion"
    }

    $DevCmd = Join-Path $VisualStudioPath 'Common7\Tools\VsDevCmd.bat'
    cmd.exe /s /c "`"$DevCmd`" -arch=x64 -host_arch=x64 >nul && set" |
        ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                [Environment]::SetEnvironmentVariable(
                    $matches[1],
                    $matches[2],
                    'Process'
                )
            }
        }
    if ($LASTEXITCODE -ne 0) {
        throw 'VsDevCmd.bat failed'
    }

    $ClOutput = @(cmd.exe /d /c 'cl.exe 2>&1')
    $MsvcVersion = [string](
        $ClOutput |
            Where-Object { ([string]$_) -match 'Compiler Version' } |
            Select-Object -First 1
    )
    if (-not $MsvcVersion) {
        throw 'MSVC version line not found'
    }

    Invoke-Logged 'Python py_compile' 'python-pycompile.log' {
        python -m py_compile `
            Tools/EngineSimVendor/run_cold_start_calibration.py `
            Tools/EngineSimVendor/generate_cold_start_profile_header.py `
            Tools/EngineSimVendor/verify_cold_start_profile.py `
            Tools/EngineSimVendor/record_cold_start_calibration_evidence.py `
            Tools/EngineSimVendor/tests/test_cold_start_profile_verifier.py
    } | Out-Null
    $UnitOutput = Invoke-Logged 'Python unit tests' 'python-unittest.log' {
        python -m unittest -v `
            Tools/EngineSimVendor/tests/test_cold_start_profile_verifier.py
    }
    if (($UnitOutput -join "`n") -notmatch 'Ran 15 tests') {
        throw 'Python mutation suite did not run all 15 tests'
    }
    Invoke-Logged 'Python orchestrator self-test' 'python-self-test.log' {
        python Tools/EngineSimVendor/run_cold_start_calibration.py self-test
    } | Out-Null
    $PythonTests = 'PASS'

    Configure 'build-cold-start-release'
    Build 'build-cold-start-release' 'Release' 'release'
    Test-CMake 'build-cold-start-release' 'Release' 'release'
    $ReleaseResult = 'PASS'

    Configure 'build-cold-start-debug'
    $DebugFlags = @(
        Get-Content 'build-cold-start-debug/CMakeCache.txt' |
            Where-Object { $_ -match '^CMAKE_CXX_FLAGS_DEBUG:' }
    )
    if (($DebugFlags -join "`n") -notmatch '/RTC1(?:\s|$)') {
        throw 'Debug configuration does not contain /RTC1'
    }
    Build 'build-cold-start-debug' 'Debug' 'debug'
    Test-CMake 'build-cold-start-debug' 'Debug' 'debug'
    $DebugResult = 'PASS'

    Configure 'build-cold-start-asan' @(
        '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/O1 /Zi /fsanitize=address',
        '-DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG /INCREMENTAL:NO'
    )
    Build 'build-cold-start-asan' 'RelWithDebInfo' 'asan'
    $AsanDll = @(
        Get-ChildItem `
            -Path $env:VCToolsInstallDir `
            -Filter 'clang_rt.asan_dynamic-x86_64.dll' `
            -File `
            -Recurse `
            -ErrorAction SilentlyContinue
    ) | Select-Object -Last 1
    if (-not $AsanDll) {
        throw 'MSVC AddressSanitizer runtime DLL not found'
    }
    $env:PATH = "$($AsanDll.DirectoryName);$env:PATH"
    $env:ASAN_OPTIONS = 'abort_on_error=true:alloc_dealloc_mismatch=true'
    Test-CMake 'build-cold-start-asan' 'RelWithDebInfo' 'asan'
    $AsanResult = 'PASS'

    $ReleaseCalibration = Get-Executable `
        'build-cold-start-release' `
        'Release' `
        'nc003b_cold_start_calibration'
    $SmokePath = Join-Path $ArtifactPath 'smoke-timeout.json'
    Invoke-Logged 'calibration smoke' 'calibration-smoke.log' {
        & $ReleaseCalibration `
            --output $SmokePath `
            --candidate-id smoke-timeout `
            --trial-index 0 `
            --seed 0x4E433033 `
            --startup-throttle 0.05 `
            --disengagement-rpm 100000 `
            --post-min-rpm 400 `
            --stability-window 0.5 `
            --maximum-time 0.016666666666666666
    } | Out-Null
    $Smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
    if ($Smoke.result -ne 'Timeout') {
        throw "smoke returned $($Smoke.result), expected Timeout"
    }
    if (
        -not $Smoke.cleanup.starter_disabled -or
        -not $Smoke.cleanup.ignition_disabled -or
        -not $Smoke.cleanup.simulator_destroyed -or
        -not $Smoke.cleanup.fixture_destroyed
    ) {
        throw 'smoke cleanup is incomplete'
    }

    if ($Phase -eq 'Discovery') {
        $DiscoveryOutput = Join-Path $ArtifactPath 'discovery'
        Invoke-Logged 'discovery sweep' 'discovery-sweep.log' {
            python Tools/EngineSimVendor/run_cold_start_calibration.py discovery `
                --executable $ReleaseCalibration `
                --sweep Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json `
                --output $DiscoveryOutput `
                --candidate-sha $CandidateSha
        } | Out-Null
        Copy-Item `
            Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json `
            (Join-Path $ArtifactPath 'subaru_ej25_cold_start_sweep.json')
    }
    else {
        if (-not $SelectedCandidatePath) {
            throw 'Selected phase requires -SelectedCandidatePath'
        }
        $SelectedCandidate = (Resolve-Path $SelectedCandidatePath).Path
        $CalibrationOutput = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration'
        Remove-Item -Recurse -Force $CalibrationOutput -ErrorAction SilentlyContinue

        Invoke-Logged 'selected trials' 'selected-trials.log' {
            python Tools/EngineSimVendor/run_cold_start_calibration.py selected `
                --executable $ReleaseCalibration `
                --sweep Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json `
                --selected-candidate $SelectedCandidate `
                --output $CalibrationOutput `
                --repo-root $RepositoryRoot `
                --candidate-sha $CandidateSha `
                --trials 10
        } | Out-Null
        $SelectedTrialsResult = '10/10 PASS'
        $TimeoutResult = 'PASS'
        $StallResult = 'PASS'

        $RunMetadataPath = Join-Path $CalibrationOutput 'windows-run-metadata.json'
        $RunMetadata = [ordered]@{
            schema_version = 1
            candidate_source_sha = $CandidateSha
            run_id = $env:GITHUB_RUN_ID
            artifact_id = 'pending-at-upload'
            artifact_name = "NC-003B-cold-start-selected-profile-$CandidateSha"
            runner_image = "windows-2022/$env:ImageVersion"
            visual_studio_version = $VisualStudioVersion
            msvc_version = (Compact $MsvcVersion)
            vc_tools_version = $env:VCToolsVersion.Trim()
            windows_sdk_version = $env:WindowsSDKVersion.Trim('\')
            cmake_version = $CMakeVersion
            python_version = $PythonVersion
            git_version = $GitVersion
            python_tests = $PythonTests
            release_result = $ReleaseResult
            debug_result = $DebugResult
            asan_result = $AsanResult
            selected_trials_result = $SelectedTrialsResult
            timeout_result = $TimeoutResult
            stall_result = $StallResult
            clean_reproducibility = 'PENDING'
            project_warning_count = -1
        }
        [System.IO.File]::WriteAllText(
            $RunMetadataPath,
            (ConvertTo-Json $RunMetadata -Depth 4) + "`n",
            [System.Text.UTF8Encoding]::new($false)
        )

        $ProfilePath = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json'
        $HeaderPath = Join-Path `
            $RepositoryRoot `
            'Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/SubaruEJ25ColdStartProfile.generated.h'
        $EvidencePath = Join-Path `
            $RepositoryRoot `
            'Tools/EngineSimVendor/COLD_START_CALIBRATION.md'

        Invoke-Logged 'finalize profile/header/evidence' 'finalize-profile.log' {
            python Tools/EngineSimVendor/record_cold_start_calibration_evidence.py `
                --profile-candidate (Join-Path $CalibrationOutput 'profile-candidate.json') `
                --selected-summary (Join-Path $CalibrationOutput 'selected-summary.json') `
                --run-metadata $RunMetadataPath `
                --repo-root $RepositoryRoot `
                --output-profile $ProfilePath `
                --output-header $HeaderPath `
                --output-evidence $EvidencePath
        } | Out-Null

        Invoke-Logged 'profile verifier' 'profile-verifier.log' {
            python Tools/EngineSimVendor/verify_cold_start_profile.py `
                --profile $ProfilePath `
                --repo-root $RepositoryRoot
        } | Out-Null

        Remove-Item -Recurse -Force build-cold-start-release
        Configure 'build-cold-start-release'
        Build 'build-cold-start-release' 'Release' 'profile-release'
        Test-CMake 'build-cold-start-release' 'Release' 'profile-release'
        Invoke-Logged 'profile direct runtime' 'profile-direct-runtime.log' {
            & (Get-Executable `
                'build-cold-start-release' `
                'Release' `
                'nc003b_cold_start_profile_tests')
        } | Out-Null

        $OriginalHashes = @(
            Get-ChildItem -Path $CalibrationOutput -File -Recurse |
                Where-Object {
                    $_.Name -match '^(trial-\d+|negative-(timeout|stall))\.json$'
                } |
                Sort-Object FullName |
                ForEach-Object {
                    (Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant()
                }
        )
        $ReproOutput = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_repro'
        Remove-Item -Recurse -Force $ReproOutput -ErrorAction SilentlyContinue
        Invoke-Logged 'clean selected reproducibility' 'selected-repro.log' {
            python Tools/EngineSimVendor/run_cold_start_calibration.py selected `
                --executable $ReleaseCalibration `
                --sweep Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json `
                --selected-candidate $SelectedCandidate `
                --output $ReproOutput `
                --repo-root $RepositoryRoot `
                --candidate-sha $CandidateSha `
                --trials 10
        } | Out-Null
        $ReproHashes = @(
            Get-ChildItem -Path $ReproOutput -File -Recurse |
                Where-Object {
                    $_.Name -match '^(trial-\d+|negative-(timeout|stall))\.json$'
                } |
                Sort-Object FullName |
                ForEach-Object {
                    (Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant()
                }
        )
        if (($OriginalHashes -join "`n") -ne ($ReproHashes -join "`n")) {
            throw 'clean reproducibility trace hashes differ'
        }
        Remove-Item -Recurse -Force $ReproOutput
        $CleanReproducibility = 'PASS'
    }

    $WarningLines = @()
    Get-ChildItem $LogPath -Filter '*-build.log' |
        ForEach-Object {
            $WarningLines += @(
                Select-String `
                    -Path $_.FullName `
                    -Pattern 'warning C\d+:' |
                    ForEach-Object { $_.Line.Trim() }
            )
        }
    $ProjectWarnings = @(
        $WarningLines |
            Sort-Object -Unique |
            Where-Object {
                $_ -match 'NextcarEngineSimCore' -or
                $_ -match 'Standalone[\\/](calibration|tests)' -or
                $_ -match 'ColdStartCalibration'
            }
    )
    $ProjectWarningCount = $ProjectWarnings.Count
    $ProjectWarnings |
        Set-Content `
            -Path (Join-Path $ArtifactPath 'project-warnings.txt') `
            -Encoding utf8
    if ($ProjectWarningCount -ne 0) {
        throw "project warning count is $ProjectWarningCount"
    }

    if ($Phase -eq 'Selected') {
        $RunMetadataPath = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/windows-run-metadata.json'
        $RunMetadata = Get-Content -Raw $RunMetadataPath | ConvertFrom-Json
        $RunMetadata.clean_reproducibility = $CleanReproducibility
        $RunMetadata.project_warning_count = $ProjectWarningCount
        [System.IO.File]::WriteAllText(
            $RunMetadataPath,
            (ConvertTo-Json $RunMetadata -Depth 4) + "`n",
            [System.Text.UTF8Encoding]::new($false)
        )
        $ProfileCandidatePath = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/profile-candidate.json'
        $SelectedSummaryPath = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration/selected-summary.json'
        $FinalProfilePath = Join-Path `
            $RepositoryRoot `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_profile.json'
        $FinalHeaderPath = Join-Path `
            $RepositoryRoot `
            'Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/SubaruEJ25ColdStartProfile.generated.h'
        $FinalEvidencePath = Join-Path `
            $RepositoryRoot `
            'Tools/EngineSimVendor/COLD_START_CALIBRATION.md'

        Invoke-Logged 'final profile regeneration' 'final-profile-regeneration.log' {
            python Tools/EngineSimVendor/record_cold_start_calibration_evidence.py `
                --profile-candidate $ProfileCandidatePath `
                --selected-summary $SelectedSummaryPath `
                --run-metadata $RunMetadataPath `
                --repo-root $RepositoryRoot `
                --output-profile $FinalProfilePath `
                --output-header $FinalHeaderPath `
                --output-evidence $FinalEvidencePath
        } | Out-Null
        Invoke-Logged 'final profile verifier' 'final-profile-verifier.log' {
            python Tools/EngineSimVendor/verify_cold_start_profile.py `
                --profile $FinalProfilePath `
                --repo-root $RepositoryRoot
        } | Out-Null

        Remove-Item -Recurse -Force build-cold-start-release
        Configure 'build-cold-start-release'
        Build 'build-cold-start-release' 'Release' 'final-profile-release'
        Test-CMake 'build-cold-start-release' 'Release' 'final-profile-release'
        Invoke-Logged 'final profile direct runtime' 'final-profile-direct-runtime.log' {
            & (Get-Executable `
                'build-cold-start-release' `
                'Release' `
                'nc003b_cold_start_profile_tests')
        } | Out-Null

        $FinalProjectWarnings = @()
        Get-ChildItem $LogPath -Filter '*-build.log' |
            ForEach-Object {
                $FinalProjectWarnings += @(
                    Select-String `
                        -Path $_.FullName `
                        -Pattern 'warning C\d+:' |
                        ForEach-Object { $_.Line.Trim() }
                )
            }
        $FinalProjectWarnings = @(
            $FinalProjectWarnings |
                Sort-Object -Unique |
                Where-Object {
                    $_ -match 'NextcarEngineSimCore' -or
                    $_ -match 'Standalone[\/](calibration|tests)' -or
                    $_ -match 'ColdStartCalibration'
                }
        )
        if ($FinalProjectWarnings.Count -ne 0) {
            throw "final project warning count is $($FinalProjectWarnings.Count)"
        }

        Copy-Item `
            -Recurse `
            -Force `
            'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_calibration' `
            (Join-Path $ArtifactPath 'selected-calibration')
        Copy-Item `
            -Force `
            $FinalProfilePath `
            (Join-Path $ArtifactPath 'subaru_ej25_cold_start_profile.json')
        Copy-Item `
            -Force `
            $FinalHeaderPath `
            (Join-Path $ArtifactPath 'SubaruEJ25ColdStartProfile.generated.h')
        Copy-Item `
            -Force `
            $FinalEvidencePath `
            (Join-Path $ArtifactPath 'COLD_START_CALIBRATION.md')
    }
}
catch {
    $Failure = Compact $_.Exception.Message
    Write-Host "FAIL $Failure"
}
finally {
    $Status = [ordered]@{
        schema_version = 1
        phase = $Phase
        candidate_sha = $CandidateSha
        status = $(if ($Failure) { 'FAIL' } else { 'PASS' })
        failure = $Failure
        python_tests = $PythonTests
        release_result = $ReleaseResult
        debug_result = $DebugResult
        asan_result = $AsanResult
        selected_trials_result = $SelectedTrialsResult
        timeout_result = $TimeoutResult
        stall_result = $StallResult
        clean_reproducibility = $CleanReproducibility
        project_warning_count = $ProjectWarningCount
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $ArtifactPath 'validation-status.json'),
        (ConvertTo-Json $Status -Depth 4) + "`n",
        [System.Text.UTF8Encoding]::new($false)
    )
    Write-HashManifest `
        $ArtifactPath `
        (Join-Path $ArtifactPath 'hash-manifest.json')
}

if ($Failure) {
    exit 1
}
exit 0
