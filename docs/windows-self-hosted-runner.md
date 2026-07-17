# Windows 11 self-hosted runner for Nextcar

This runbook configures a Windows 11 x64 computer to execute the `Full Unreal Engine CI` workflow for Nextcar with Unreal Engine 5.8. The default configuration installs the GitHub Actions runner as a Windows service that starts automatically with the computer.

## Security decision for the current repository

`Dziuras98/Nextcar` is private, so the runner may operate persistently as a Windows service. This removes the public-fork exposure that blocked permanent operation in the previous version of this runbook.

A private repository does not make self-hosted execution isolated. Workflow code runs directly on the Windows host and may persist changes between jobs. Limit repository access to trusted collaborators, review changes that affect `.github/workflows/**`, build scripts, project files, and executable source code, and keep `ENABLE_UNREAL_CI=false` when full Unreal CI is not needed.

The service should run under a dedicated unprivileged local Windows account. Do not use `LocalSystem` or an administrator account unless a verified toolchain requirement makes that necessary.

## 1. Required software

Install the following on the runner computer:

1. Windows 11 x64 with current updates.
2. Git for Windows, installed for all users and added to the system `PATH`.
3. Epic Games Launcher and Unreal Engine 5.8.
4. Visual Studio 2022, current stable release.

In Visual Studio Installer select these workloads:

- .NET desktop development;
- Desktop development with C++;
- .NET Multi-platform App UI development;
- Game development with C++.

Under the C++ workload, ensure that these components are installed:

- MSVC v143 C++ x64/x86 build tools;
- Windows 10 or Windows 11 SDK;
- C++ profiling tools;
- C++ AddressSanitizer;
- .NET 8 SDK/runtime.

In the Unreal Engine 5.8 installation options keep at least:

- Core Components;
- Templates and Feature Packs.

Starter Content and editor debugging symbols are optional for this project.

Recommended practical capacity for this runner is 32 GB RAM and at least 100 GB of free SSD space for the engine, checkout, intermediate files, logs, and reports.

## 2. Configure machine-level toolchain settings

Open PowerShell as Administrator. Adjust the Unreal path if Unreal Engine 5.8 was installed elsewhere.

```powershell
$UE = 'C:\Program Files\Epic Games\UE_5.8'

if (-not (Test-Path "$UE\Engine\Build\BatchFiles\Build.bat")) {
    throw "Build.bat was not found under $UE"
}

if (-not (Test-Path "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")) {
    throw "UnrealEditor-Cmd.exe was not found under $UE"
}

[Environment]::SetEnvironmentVariable('UE_ROOT', $UE, 'Machine')
$env:UE_ROOT = $UE

git config --system core.longpaths true

Write-Host "UE_ROOT=$env:UE_ROOT"
where.exe git
```

`UE_ROOT` must be a machine-level variable because the Windows service does not inherit the interactive user's environment. Git and any other command-line dependencies required by jobs must also be available through the system `PATH`, not only the user's `PATH`.

Restart the runner service after changing any machine-level environment variable.

## 3. Perform a local Unreal preflight

Run the build and tests interactively before registering the service. This separates Unreal or Visual Studio configuration failures from runner-service failures.

Sign in to GitHub through Git Credential Manager if the private repository clone requests authentication.

```powershell
New-Item -ItemType Directory -Force 'C:\src' | Out-Null

git clone `
    --branch agent/initial-driving-prototype `
    --single-branch `
    https://github.com/Dziuras98/Nextcar.git `
    C:\src\Nextcar

$Project = 'C:\src\Nextcar\Nextcar.uproject'
$BuildTool = Join-Path $env:UE_ROOT 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $env:UE_ROOT 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ReportDir = 'C:\src\Nextcar\Saved\AutomationReports'

& $BuildTool NextcarEditor Win64 Development $Project -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "NextcarEditor build failed with exit code $LASTEXITCODE"
}

New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null

& $EditorCmd $Project `
    '-ExecCmds=Automation RunTests Nextcar; Quit' `
    '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$ReportDir" `
    -unattended `
    -nop4 `
    -nosplash `
    -NullRHI `
    -stdout `
    -FullStdOutLogOutput `
    -UTF8Output

if ($LASTEXITCODE -ne 0) {
    throw "Unreal Automation Tests failed with exit code $LASTEXITCODE"
}

$IndexFile = Join-Path $ReportDir 'index.json'
if (-not (Test-Path $IndexFile)) {
    throw "Automation report was not produced at $IndexFile"
}

Write-Host 'Local Unreal preflight passed.'
```

The expected outputs are:

- a successful `NextcarEditor` Development build;
- exit code `0` from `UnrealEditor-Cmd.exe`;
- `C:\src\Nextcar\Saved\AutomationReports\index.json`.

## 4. Create a dedicated Windows service account

Open PowerShell as Administrator. Create a local, non-administrator account for the runner service. Retain its password because the runner configuration will request it.

```powershell
$RunnerUser = 'NextcarRunner'
$ExistingUser = Get-LocalUser -Name $RunnerUser -ErrorAction SilentlyContinue

if ($null -eq $ExistingUser) {
    $RunnerPassword = Read-Host `
        "Enter a strong password for .\$RunnerUser" `
        -AsSecureString

    New-LocalUser `
        -Name $RunnerUser `
        -Password $RunnerPassword `
        -AccountNeverExpires `
        -PasswordNeverExpires `
        -UserMayNotChangePassword `
        -Description 'GitHub Actions runner for Nextcar Unreal CI'
}
else {
    Write-Host ".\$RunnerUser already exists. Use its current password during runner configuration."
}

New-Item -ItemType Directory -Force 'C:\actions-runner' | Out-Null

$RunnerIdentity = "$env:COMPUTERNAME\$RunnerUser"
icacls 'C:\actions-runner' /grant "${RunnerIdentity}:(OI)(CI)M"
```

Do not add `NextcarRunner` to the local Administrators group. The account needs:

- read and execute access to the Unreal Engine and Visual Studio installations;
- modify access to `C:\actions-runner` and its `_work` directory;
- access to machine-level `UE_ROOT` and the system `PATH`;
- outbound HTTPS access to GitHub.

The runner configuration normally grants the selected account the Windows **Log on as a service** right while installing the service.

## 5. Register and install the runner as a service

On GitHub:

1. Open the `Dziuras98/Nextcar` repository.
2. Go to **Settings → Actions → Runners**.
3. Select **New self-hosted runner**.
4. Select **Windows** and **x64**.
5. Copy the generated download, checksum, extraction, and configuration commands. Use the commands generated for the repository instead of hard-coding a runner version. The registration token expires after one hour.

On the Windows computer, open PowerShell as Administrator and use `C:\actions-runner` as the runner directory. After downloading and extracting the runner, start configuration with the current token:

```powershell
cd C:\actions-runner

.\config.cmd `
    --url https://github.com/Dziuras98/Nextcar `
    --token '<TOKEN_FROM_GITHUB>' `
    --name 'NEXTCAR-UE58' `
    --labels 'unreal-5.8' `
    --work '_work'
```

Answer the service prompts as follows:

- **Run runner as a service?** — `Y`;
- **User account to use for the service** — `.\NextcarRunner`;
- **Password** — the password created in section 4.

Interactive configuration is preferred because it avoids placing the Windows account password in PowerShell command history. Do not store the registration token or service-account password in a script or commit either value to the repository.

GitHub automatically adds the default labels `self-hosted`, `Windows`, and `X64`. The custom `unreal-5.8` label is required because the workflow uses:

```yaml
runs-on: [self-hosted, Windows, X64, unreal-5.8]
```

### Existing runner configured with `run.cmd`

Windows service installation is part of `config.cmd`. An existing runner configured without a service cannot be converted in place through a separate Windows service installation command.

To migrate it:

1. Stop `run.cmd` with `Ctrl+C`.
2. On GitHub, open **Settings → Actions → Runners → NEXTCAR-UE58 → Remove**.
3. Copy and run the removal command and time-limited removal token shown by GitHub.
4. Run `config.cmd` again using the procedure above and answer `Y` to the service prompt.

## 6. Verify and start the Windows service

The configuration process should install and start the service. Verify the exact service identity, startup type, and status in an elevated PowerShell window:

```powershell
$RunnerService = Get-CimInstance Win32_Service |
    Where-Object {
        $_.Name -like 'actions.runner.*' -and
        $_.DisplayName -like '*NEXTCAR-UE58*'
    } |
    Select-Object -First 1

if ($null -eq $RunnerService) {
    throw 'The NEXTCAR-UE58 Windows service was not found.'
}

Set-Service -Name $RunnerService.Name -StartupType Automatic
Start-Service -Name $RunnerService.Name

Get-CimInstance Win32_Service -Filter "Name='$($RunnerService.Name)'" |
    Select-Object Name, DisplayName, StartName, StartMode, State
```

The expected state is:

- `StartName` identifies the dedicated `NextcarRunner` account;
- `StartMode` is `Auto`;
- `State` is `Running`.

GitHub should show `NEXTCAR-UE58` as **Idle** under **Settings → Actions → Runners**. Reboot Windows once and confirm that the service returns to **Running** and the runner returns to **Idle** without an interactive login.

The computer must remain awake to accept jobs. Disable automatic sleep while connected to AC power. The service may run without an interactive user session, but it cannot execute jobs while the computer is asleep or powered off.

## 7. Enable the Nextcar Unreal workflow

The current pull-request workflow executes its Unreal job only when the repository variable `ENABLE_UNREAL_CI` equals `true`.

1. Go to **Settings → Secrets and variables → Actions**.
2. Open the **Variables** tab.
3. Create or update the repository variable:

   - name: `ENABLE_UNREAL_CI`;
   - value: `true`.

4. Open pull request #1.
5. Re-run the existing **Full Unreal Engine CI** workflow.
6. If the existing skipped run remains skipped after re-running, trigger a new pull-request synchronization by updating the PR branch from current `main`. Do not create a meaningless commit only to trigger CI.

The job should be routed to `NEXTCAR-UE58`. The workflow will:

1. clean and check out the pull-request commit;
2. verify `UE_ROOT`, `Build.bat`, `UnrealEditor-Cmd.exe`, and `Nextcar.uproject`;
3. build `NextcarEditor Win64 Development`;
4. run all `Nextcar.*` Unreal Automation Tests with `-NullRHI`;
5. require `Saved\AutomationReports\index.json`;
6. upload `Saved\AutomationReports` and `Saved\Logs` as an artifact.

## 8. Verify the result

A valid successful run must show all of these steps as successful:

- Verify Unreal Engine runner;
- Build NextcarEditor;
- Run Unreal Automation Tests;
- Require automation report;
- Upload Unreal logs and test reports.

Download the `unreal-ci-results-*` artifact and retain at least:

- `Saved\AutomationReports\index.json`;
- the relevant files under `Saved\Logs`.

Do not treat a skipped workflow or a missing report as a passed test.

## 9. Normal service operation

After a Gate 0 run:

1. Set `ENABLE_UNREAL_CI` back to `false` unless every trusted pull request should run full Unreal CI automatically.
2. Leave the Windows service installed and running. An idle runner consumes the next matching job but does not require an open terminal or an interactive Windows login.
3. Confirm that GitHub shows the runner as **Idle**, not **Offline**.

The runner application does not require inbound port forwarding. It initiates outbound HTTPS connections to GitHub; outbound TCP port 443 must be available.

Service management commands:

```powershell
Get-Service 'actions.runner.*'
Stop-Service 'actions.runner.*'
Start-Service 'actions.runner.*'
Restart-Service 'actions.runner.*'
```

Restart the service after changing `UE_ROOT`, the system `PATH`, permissions, the service account, or installed toolchain components.

## 10. Troubleshooting

### Job remains queued

- Confirm that the service is running and GitHub shows the runner as idle.
- Confirm that it has all four labels: `self-hosted`, `Windows`, `X64`, and `unreal-5.8`.
- Confirm that no other job is already using the runner.

### Workflow is skipped

- Confirm repository variable `ENABLE_UNREAL_CI=true`.
- A manually re-run old workflow may retain a skipped job; trigger a new trusted PR synchronization if required.

### `UE_ROOT` is empty

Verify the machine variable:

```powershell
[Environment]::GetEnvironmentVariable('UE_ROOT', 'Machine')
```

Correct the variable and restart the service:

```powershell
Get-Service 'actions.runner.*' | Restart-Service
```

### `Build.bat` or `UnrealEditor-Cmd.exe` is missing

- Correct `UE_ROOT` so it points to the Unreal Engine installation root, for example `C:\Program Files\Epic Games\UE_5.8`.
- Do not point it at the `Engine` subdirectory itself.
- Confirm that the service account has read and execute access to the installation.

### Git or another command is not found

A Windows service may not receive tools installed only in a user's `PATH`. Install required tools for all users, add them to the machine-level `PATH`, and restart the runner service.

### C++ compiler or Windows SDK error

- Open Visual Studio Installer.
- Modify Visual Studio 2022.
- Verify the C++ workloads, MSVC v143 toolset, Windows SDK, and .NET 8 components.
- Confirm that the components were installed for the machine rather than only made available through an interactive user's environment.

### No automation report

- Inspect `Saved\Logs` for test discovery, module loading, project startup, or command-line parsing failures.
- The run must remain failed until `Saved\AutomationReports\index.json` is produced.

### Access denied or service logon failure

Inspect the service account and state:

```powershell
Get-CimInstance Win32_Service |
    Where-Object { $_.Name -like 'actions.runner.*' } |
    Select-Object Name, StartName, State, ExitCode
```

Then verify:

- the stored password for `.\NextcarRunner` is current;
- the account has **Log on as a service** rights;
- the account can read the Unreal and Visual Studio installations;
- the account can modify `C:\actions-runner\_work`;
- endpoint protection has not blocked `Runner.Listener.exe`, `Runner.Worker.exe`, UBT, or UnrealEditor-Cmd.exe.

If the service identity must change, remove and reconfigure the runner through GitHub rather than manually rewriting the service command line.