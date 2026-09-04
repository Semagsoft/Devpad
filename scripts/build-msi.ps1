param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [string]$InstallerDir = (Join-Path $PSScriptRoot "..\installer"),
    [string]$WixDir = "C:\ProgramData\wix314",
    [string]$Version = "1.04",
    [ValidateSet("x64", "arm64")]
    [string]$Arch = "x64",
    [string]$OutputMsi = "",
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"

if (-not $OutputMsi) {
    if ($Arch -eq "arm64") {
        $OutputMsi = Join-Path $PSScriptRoot "..\Devpad-$Version-arm64.msi"
    } else {
        $OutputMsi = Join-Path $PSScriptRoot "..\Devpad-$Version.msi"
    }
}

# WiX candle architecture flag matches the target arch (x64 or arm64).
# The .wxs Platform attribute is set via -dPlatform to match.
$wixArch = $Arch

# Ensure WiX v3 tools are available (heat/candle/light CLI).
# Search order: explicit -WixDir (dir or bin) -> PATH -> well-known install
# locations (emulated x86 + native ARM64 program-files paths).
$wixBin = $null
if ($WixDir) {
    if (Test-Path (Join-Path $WixDir "heat.exe")) { $wixBin = $WixDir }
    elseif (Test-Path (Join-Path $WixDir "bin\heat.exe")) { $wixBin = Join-Path $WixDir "bin" }
}
if (-not $wixBin) {
    $heatCmd = Get-Command heat.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($heatCmd -and $heatCmd.Source) { $wixBin = Split-Path $heatCmd.Source }
}
if (-not $wixBin) {
    $wixCandidates = @()
    $wixCandidates += Get-ChildItem "C:\Program Files (x86)\WiX Toolset v3*" -Directory -ErrorAction SilentlyContinue
    $wixCandidates += Get-ChildItem "C:\Program Files\WiX Toolset v3*" -Directory -ErrorAction SilentlyContinue
    $first = $wixCandidates | Select-Object -First 1
    if ($first) { $wixBin = Join-Path $first.FullName "bin" }
}
if (-not $wixBin -or -not (Test-Path (Join-Path $wixBin "heat.exe"))) {
    Write-Error "WiX Toolset v3 not found (heat.exe missing). Install via: choco install wixtoolset -y"
    exit 1
}

$env:Path = "$wixBin;$env:Path"

# Step 1: Deploy Qt + runtime dependencies into a self-contained dist
# (skipped when CI already deployed via deploy-windows-deps.ps1).
$distDir = Join-Path $BuildDir "dist"
if (-not $SkipDeploy) {
    & (Join-Path $PSScriptRoot "deploy-windows-deps.ps1") -BuildDir $BuildDir -DistDir $distDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Dependency deployment failed (deploy-windows-deps.ps1)"
        exit 1
    }
}

# Step 2: Ensure we're in the installer directory
Push-Location $InstallerDir
try {
    # Step 3: Harvest the dist directory into a WiX fragment with heat
    Write-Host "Harvesting dist directory with heat..."
    & heat.exe dir $distDir -nologo -gg -srd -sreg -dr INSTALLDIR -cg DeployedFiles -var var.DistDir -out Heat.wxs
    if ($LASTEXITCODE -ne 0) { throw "heat failed" }

    # Step 4: Compile .wxs to .wixobj
    Write-Host "Compiling Heat.wxs ($wixArch)..."
    & candle.exe -arch $wixArch "-dProductVersion=$Version" "-dPlatform=$wixArch" "-dDistDir=$distDir" -out Heat.wixobj Heat.wxs
    if ($LASTEXITCODE -ne 0) { throw "candle Heat.wxs failed" }

    Write-Host "Compiling Devpad.wxs ($wixArch)..."
    & candle.exe -arch $wixArch "-dProductVersion=$Version" "-dPlatform=$wixArch" -out Devpad.wixobj Devpad.wxs
    if ($LASTEXITCODE -ne 0) { throw "candle failed" }

    # Step 5: Link .wixobj to .msi
    Write-Host "Linking Devpad.msi..."
    & light.exe -out $OutputMsi -ext WixUIExtension Devpad.wixobj Heat.wixobj
    if ($LASTEXITCODE -ne 0) { throw "light failed" }

    Write-Host "MSI created: $OutputMsi"
    $size = (Get-Item $OutputMsi).Length / 1MB
    Write-Host "Size: $([math]::Round($size, 1)) MB"
}
finally {
    Pop-Location
    # Clean up intermediate files
    Remove-Item (Join-Path $InstallerDir "Devpad.wixobj") -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $InstallerDir "Devpad.wixpdb") -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $InstallerDir "Heat.wxs") -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $InstallerDir "Heat.wixobj") -ErrorAction SilentlyContinue
}
