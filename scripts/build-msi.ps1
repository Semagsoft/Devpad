param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [string]$InstallerDir = (Join-Path $PSScriptRoot "..\installer"),
    [string]$WixDir = "C:\ProgramData\wix314",
    [string]$Version = "1.03",
    [string]$OutputMsi = (Join-Path $PSScriptRoot "..\Devpad-$Version.msi")
)

$ErrorActionPreference = "Stop"

# Ensure WiX tools are available
$wixBin = if (Test-Path $WixDir) { $WixDir } else { (Get-Command candle.exe -ErrorAction SilentlyContinue).Directory }
if (-not $wixBin) {
    $wixCandidates = Get-ChildItem "C:\Program Files (x86)\WiX Toolset v3*" -Directory -ErrorAction SilentlyContinue
    if ($wixCandidates) {
        $wixBin = $wixCandidates[0].FullName + "\bin"
    }
}
if (-not $wixBin) {
    Write-Error "WiX Toolset not found. Install via: winget install WiXToolset.WiXToolset"
    exit 1
}

$env:Path = "$wixBin;$env:Path"

# Step 1: Deploy Qt + runtime dependencies into a self-contained dist
$distDir = Join-Path $BuildDir "dist"
& (Join-Path $PSScriptRoot "deploy-windows-deps.ps1") -BuildDir $BuildDir -DistDir $distDir
if ($LASTEXITCODE -ne 0) {
    Write-Error "Dependency deployment failed (deploy-windows-deps.ps1)"
    exit 1
}

# Step 2: Ensure we're in the installer directory
Push-Location $InstallerDir
try {
    # Step 3: Harvest the dist directory into a WiX fragment with heat
    Write-Host "Harvesting dist directory with heat..."
    & heat.exe dir $distDir -nologo -gg -srd -dr INSTALLDIR -cg DeployedFiles "-var var.DistDir" -out Heat.wxs
    if ($LASTEXITCODE -ne 0) { throw "heat failed" }

    # Step 4: Compile .wxs to .wixobj
    Write-Host "Compiling Heat.wxs..."
    & candle.exe -arch x64 "-dProductVersion=$Version" "-dDistDir=$distDir" -out Heat.wixobj Heat.wxs
    if ($LASTEXITCODE -ne 0) { throw "candle Heat.wxs failed" }

    Write-Host "Compiling Devpad.wxs..."
    & candle.exe -arch x64 "-dProductVersion=$Version" -out Devpad.wixobj Devpad.wxs
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
