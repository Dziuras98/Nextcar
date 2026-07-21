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

if ([string]::IsNullOrWhiteSpace($SelectedCandidatePath)) {
    if ($Phase -ne 'Discovery') {
        throw 'SelectedCandidatePath is required for the Selected phase'
    }
    $SelectedCandidatePath = 'Tests/EngineSimCore/Fixtures/subaru_ej25_cold_start_sweep.json'
}

$PartRoot = Join-Path $PSScriptRoot 'validate_cold_start_windows.parts'
$Parts = @(
    @{ Name = 'part00.ps1frag'; Sha256 = 'eac0aa60d6fc3b38a7fd1f65191d64e7cd3551e822cd2e7ae2cfa42c89365904' },
    @{ Name = 'part01.ps1frag'; Sha256 = 'c3c09b792002ba019c7c674c78ca79a63577c97c18d92a90204bb15db14c89ac' },
    @{ Name = 'part02.ps1frag'; Sha256 = 'f2dbf747330411387a4e568e5e5eda2806b70a1ca6c2d228f64badc582e0a89f' },
    @{ Name = 'part03.ps1frag'; Sha256 = '9d5741a3cf78450448953aff7912e0f3074fb82a8d2380442415e3cda69ddec8' },
    @{ Name = 'part04.ps1frag'; Sha256 = '956367b601930b1c7821aed6109d02acf2b512c39d54bc14e9e5b24bbd9b1db8' }
)
$Builder = [System.Text.StringBuilder]::new()
$Utf8 = [System.Text.UTF8Encoding]::new($false)
foreach ($Part in $Parts) {
    $Path = Join-Path $PartRoot $Part.Name
    $ActualPartHash = (Get-FileHash -Algorithm SHA256 -Path $Path).Hash.ToLowerInvariant()
    if ($ActualPartHash -ne $Part.Sha256) {
        throw "cold-start Windows source fragment hash mismatch: $($Part.Name)"
    }
    $Bytes = [System.IO.File]::ReadAllBytes($Path)
    [void]$Builder.Append($Utf8.GetString($Bytes))
}
$Source = $Builder.ToString()
$SourceBytes = $Utf8.GetBytes($Source)
$Hasher = [System.Security.Cryptography.SHA256]::Create()
try {
    $SourceHash = ([System.BitConverter]::ToString(
        $Hasher.ComputeHash($SourceBytes))).Replace('-', '').ToLowerInvariant()
}
finally {
    $Hasher.Dispose()
}
if ($SourceHash -ne '10ee201b5136864012a3e975989d8ed9ded47dbf7813e1c9d0e11e82900d4f13') {
    throw 'cold-start Windows source hash mismatch'
}
$Implementation = [ScriptBlock]::Create($Source)
& $Implementation `
    -CandidateSha $CandidateSha `
    -Phase $Phase `
    -SelectedCandidatePath $SelectedCandidatePath `
    -ArtifactRoot $ArtifactRoot
