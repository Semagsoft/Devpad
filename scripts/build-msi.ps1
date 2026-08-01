param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [string]$InstallerDir = (Join-Path $PSScriptRoot "..\installer"),
    [string]$WixDir = "C:\ProgramData\wix314",
    [string]$Version = "1.01",
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

# Step 1: Deploy Qt dependencies if dist doesn't contain the exe
$distDir = Join-Path $BuildDir "dist"
if (-not (Test-Path (Join-Path $distDir "Devpad.exe"))) {
    Write-Host "Running windeployqt..."
    & "windeployqt.exe" (Join-Path $BuildDir "Devpad.exe") "--dir" $distDir
    Copy-Item (Join-Path $BuildDir "Devpad.exe") $distDir
    Copy-Item (Join-Path $BuildDir "devpad_*.qm") $distDir -ErrorAction SilentlyContinue

    # Copy runtime dependencies not already deployed by windeployqt
    if (Get-Command objdump -ErrorAction SilentlyContinue) {
        $targets = @((Get-ChildItem $distDir -Recurse -Filter *.dll -ErrorAction SilentlyContinue).FullName) + @(Join-Path $BuildDir "Devpad.exe")
        $names = @($targets | ForEach-Object { (& objdump -p $_ 2>$null | Select-String 'DLL Name').Line } |
            ForEach-Object { ($_ -replace '.*DLL Name:\s*', '').Trim() } | Sort-Object -Unique)
        $prefix = if ($env:MSYSTEM_PREFIX) { $env:MSYSTEM_PREFIX } else { "C:\msys64\ucrt64" }
        foreach ($name in $names) {
            $src = Get-ChildItem (Join-Path $prefix "bin") -Filter $name -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($src -and -not (Test-Path (Join-Path $distDir $name))) {
                Copy-Item $src.FullName $distDir
            }
        }
    }
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
