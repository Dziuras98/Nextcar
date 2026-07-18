param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"
$Repo = "Dziuras98/Nextcar"
$Tag = "nc003b-phase0-transfer-20260718"
$ApiBase = "https://api.github.com/repos/$Repo"
$LogDirectory = Join-Path $ProjectRoot "Saved/BuildLogs"
$LogPath = Join-Path $LogDirectory "NC003BTransferRelease.json"

$env:GIT_TERMINAL_PROMPT = "0"
$env:GCM_INTERACTIVE = "Never"
Set-Location $ProjectRoot
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null

$CheckoutHeaders = @(git config --local --get-all http.https://github.com/.extraheader 2>$null)
git config --local --unset-all http.https://github.com/.extraheader 2>$null

try {
    $CredentialLines = @("protocol=https", "host=github.com", "") | git credential fill
    if ($LASTEXITCODE -ne 0) {
        throw "git credential fill failed with exit code $LASTEXITCODE"
    }

    $Credential = @{}
    foreach ($Line in $CredentialLines) {
        if ($Line -match "^([^=]+)=(.*)$") {
            $Credential[$Matches[1]] = $Matches[2]
        }
    }
    if (-not $Credential.ContainsKey("username") -or -not $Credential.ContainsKey("password")) {
        throw "Git credential manager did not return a writable GitHub credential."
    }

    $BasicBytes = [Text.Encoding]::ASCII.GetBytes("$($Credential['username']):$($Credential['password'])")
    $Headers = @{
        Authorization = "Basic $([Convert]::ToBase64String($BasicBytes))"
        Accept = "application/vnd.github+json"
        "X-GitHub-Api-Version" = "2022-11-28"
        "User-Agent" = "NC-003B-Phase0-Transfer"
    }

    $Releases = @(Invoke-RestMethod -Method Get -Uri "$ApiBase/releases?per_page=100" -Headers $Headers)
    $Release = $Releases | Where-Object { $_.tag_name -eq $Tag } | Select-Object -First 1
    if ($null -eq $Release) {
        $Body = @{
            tag_name = $Tag
            target_commitish = "agent/nc-003b-engine-sim-core"
            name = "NC-003B Phase 0 transfer"
            body = "Temporary private transfer asset for the atomic NC-003B Git Data API publication. Delete after successful publication."
            draft = $true
            prerelease = $true
        } | ConvertTo-Json
        $Release = Invoke-RestMethod -Method Post -Uri "$ApiBase/releases" -Headers $Headers -ContentType "application/json" -Body $Body
    }

    $Result = [ordered]@{
        release_id = [int64]$Release.id
        tag_name = [string]$Release.tag_name
        upload_url = ([string]$Release.upload_url -replace '\{\?name,label\}$', '')
        draft = [bool]$Release.draft
        head_sha = (git rev-parse HEAD).Trim()
    }
    $Result | ConvertTo-Json | Set-Content -LiteralPath $LogPath -Encoding utf8
    Write-Host "NC003B_TRANSFER_RELEASE_ID=$($Result.release_id)"
    Write-Host "NC003B_TRANSFER_RELEASE_TAG=$($Result.tag_name)"
}
finally {
    foreach ($Header in $CheckoutHeaders) {
        git config --local --add http.https://github.com/.extraheader $Header
    }
}

exit 0
