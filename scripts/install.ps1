# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

[CmdletBinding()]
param(
    [string]$Archive,
    [string]$Version = "latest",
    [string]$InstallDir = (Join-Path $HOME ".codon"),
    [switch]$AddToPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ($env:OS -ne "Windows_NT") {
    throw "Use scripts/install.sh on Linux or macOS. This installer requires Windows."
}
$architecture = if ($env:PROCESSOR_ARCHITEW6432) { $env:PROCESSOR_ARCHITEW6432 } else { $env:PROCESSOR_ARCHITECTURE }
if ($architecture -ne "AMD64" -or -not [Environment]::Is64BitProcess) {
    throw "The Windows SDK requires x86-64 Windows and a 64-bit PowerShell session."
}
if ($Archive -and $PSBoundParameters.ContainsKey("Version")) {
    throw "Use either -Archive or -Version, not both."
}
$InstallDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($InstallDir)
if (Test-Path -LiteralPath $InstallDir) {
    throw "The destination already exists: $InstallDir. Choose a new -InstallDir or move the existing installation first."
}

$temporary = Join-Path ([IO.Path]::GetTempPath()) ("codon-install-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    $assetName = "codon-windows-x86_64.zip"
    if ($Archive) {
        $Archive = (Resolve-Path -LiteralPath $Archive).ProviderPath
    } else {
        [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
        $releasePath = if ($Version -eq "latest") { "latest" } else { "tags/" + [Uri]::EscapeDataString($Version) }
        $release = Invoke-RestMethod -Uri "https://api.github.com/repos/exaloop/codon/releases/$releasePath" -Headers @{ "User-Agent" = "Codon-Installer" }
        $asset = $release.assets | Where-Object { $_.name -eq $assetName } | Select-Object -First 1
        if (-not $asset) {
            throw "Release $($release.tag_name) has no Windows SDK. Download a successful Windows CI artifact and rerun with -Archive <zip>."
        }
        $Archive = Join-Path $temporary $assetName
        Invoke-WebRequest -UseBasicParsing -Uri $asset.browser_download_url -OutFile $Archive
    }
    $expanded = Join-Path $temporary "expanded"
    Expand-Archive -LiteralPath $Archive -DestinationPath $expanded
    $sdk = Join-Path $expanded "codon-deploy"
    if (-not (Test-Path -LiteralPath $sdk -PathType Container)) {
        $nested = Join-Path $expanded $assetName
        if (Test-Path -LiteralPath $nested -PathType Leaf) {
            $expanded = Join-Path $temporary "sdk"
            Expand-Archive -LiteralPath $nested -DestinationPath $expanded
            $sdk = Join-Path $expanded "codon-deploy"
        }
    }
    foreach ($required in @("bin/codon.exe", "bin/codonc.dll", "bin/codonrt.dll", "lib/codon/stdlib", "python/setup.py")) {
        if (-not (Test-Path -LiteralPath (Join-Path $sdk $required))) {
            throw "The archive is not a complete Windows Codon SDK: missing $required."
        }
    }
    & (Join-Path $sdk "bin/codon.exe") --version
    if ($LASTEXITCODE -ne 0) {
        throw "The SDK could not run. Check the x64 Visual C++ Redistributable before installing."
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $InstallDir) | Out-Null
    Move-Item -LiteralPath $sdk -Destination $InstallDir
    $bin = Join-Path $InstallDir "bin"
    if ($AddToPath) {
        $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
        $entries = @($userPath -split ";" | Where-Object { $_ })
        if (@($entries | ForEach-Object { $_.TrimEnd("\") }) -notcontains $bin.TrimEnd("\")) {
            [Environment]::SetEnvironmentVariable("Path", (($entries + $bin) -join ";"), "User")
        }
    }
    $env:PATH = "$bin;$env:PATH"
    $env:CODON_DIR = $InstallDir
    Write-Host "Codon installed at $InstallDir"
    Write-Host "Open a new terminal to use a persisted PATH, or use this PowerShell session."
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force
}
