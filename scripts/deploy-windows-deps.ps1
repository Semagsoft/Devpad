<#
.SYNOPSIS
Deploys the Qt and MSYS2 runtime dependencies of Devpad into a self-contained
`dist` directory.

windeployqt only copies Qt libraries and plugins. Non-Qt dependencies such as
QScintilla, the FFmpeg runtime or the ICU/graphite2/brotli/gettext libraries used
by Qt itself are resolved here by walking the full transitive closure of DLL
imports. Finally, every import is verified to be satisfiable from `dist` or a
Windows system DLL, and the script fails with a non-zero exit code if any import
cannot be resolved.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File scripts/deploy-windows-deps.ps1 -BuildDir build -DistDir build\dist -MsysPrefix C:\msys64\ucrt64
#>

param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "..\build"),
    [string]$DistDir = (Join-Path $BuildDir "dist"),
    [string]$MsysPrefix = "",
    [string[]]$ExtraSearchDirs = @(),
    [switch]$SkipVerify
)

$ErrorActionPreference = "Stop"

function Write-Step([string]$Msg) { Write-Host "==> $Msg" }
function Write-Done([string]$Msg) { Write-Host "[+] $Msg" }
function Write-Fail([string]$Msg) { Write-Host "[E] $Msg"; exit 1 }

if (-not (Test-Path (Join-Path $BuildDir "Devpad.exe"))) {
    Write-Fail "Devpad.exe not found in $BuildDir. Build the project first."
}

# ── Locate the MSYS2 prefix ───────────────────────────────────
function Find-MsysPrefix {
    # Explicitly provided via -MsysPrefix.
    if ($script:MsysPrefix) { return $script:MsysPrefix }
    # Exported by the MSYS2 UCRT64/MINGW64/CLANG64 shells.
    if ($env:MSYSTEM_PREFIX) { return $env:MSYSTEM_PREFIX }

    # Infer from windeployqt on PATH: <prefix>\bin\windeployqt.exe
    $cmd = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($cmd) {
        $prefix = Split-Path (Split-Path $cmd.Source -Parent) -Parent
        if ($prefix -and (Test-Path (Join-Path $prefix "bin"))) { return $prefix }
    }

    # Infer from objdump on PATH: <msysBase>\usr\bin\objdump.exe
    $cmd = Get-Command objdump -ErrorAction SilentlyContinue
    if ($cmd) {
        $dir = Split-Path $cmd.Source -Parent
        $dir = Split-Path $dir -Parent
        $msysBase = Split-Path $dir -Parent
        if ($msysBase) {
            foreach ($sub in @("ucrt64", "mingw64", "clang64")) {
                $candidate = Join-Path $msysBase $sub
                if (Test-Path (Join-Path $candidate "bin")) { return $candidate }
            }
        }
    }

    if (Test-Path "C:\msys64\ucrt64") { return "C:\msys64\ucrt64" }
    if (Test-Path "C:\msys64\mingw64") { return "C:\msys64\mingw64" }
    return ""
}

$MsysPrefix = Find-MsysPrefix
$msysBase = if ($MsysPrefix) { Split-Path $MsysPrefix -Parent } else { "" }

# ── Locate a tool on PATH or under the MSYS2 tree ──────────────
function Find-InPath([string]$Exe, [string[]]$ExtraDirs) {
    $cmd = Get-Command $Exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($d in $ExtraDirs) {
        if (-not $d) { continue }
        foreach ($candidate in @((Join-Path $d "$Exe.exe"), (Join-Path $d $Exe))) {
            if (Test-Path $candidate) { return $candidate }
        }
    }
    return $null
}

# ── Step 1: windeployqt ───────────────────────────────────────
if (-not (Test-Path (Join-Path $DistDir "Devpad.exe"))) {
    Write-Step "Running windeployqt..."
    $windeployqt = Find-InPath "windeployqt" @("$MsysPrefix\bin", "$msysBase\bin")
    if (-not $windeployqt) { Write-Fail "windeployqt not found on PATH or under the MSYS2 prefix." }
    & $windeployqt (Join-Path $BuildDir "Devpad.exe") "--dir" $DistDir
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[!] windeployqt exited with code $LASTEXITCODE; continuing (verification gate will catch any gaps)"
    }
    New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
    Copy-Item (Join-Path $BuildDir "Devpad.exe") $DistDir -Force
    Copy-Item (Join-Path $BuildDir "devpad_*.qm") $DistDir -ErrorAction SilentlyContinue
    Write-Done "windeployqt complete"
}

if (-not (Test-Path (Join-Path $DistDir "Devpad.exe"))) {
    Write-Fail "$DistDir does not contain Devpad.exe"
}

# Sanity-check that critical Qt plugins were deployed (a fully failed
# windeployqt would otherwise go unnoticed, since plugins have no imports
# to verify against in dist).
if (-not (Test-Path (Join-Path $DistDir "platforms\qwindows.dll"))) {
    Write-Fail "Missing critical Qt platform plugin: platforms\qwindows.dll (windeployqt likely failed)"
}

# ── Step 2: transitive dependency resolution ───────────────────
Write-Step "Resolving DLL dependencies (transitive closure)..."

$objdump = Find-InPath "objdump" @("$msysBase\usr\bin", "$MsysPrefix\bin", "$msysBase\bin")
if (-not $objdump) { Write-Fail "objdump not found on PATH or under the MSYS2 prefix." }

function Get-DllImports([string]$Path) {
    $names = New-Object System.Collections.Generic.List[string]
    $out = & $objdump -p $Path 2>$null
    foreach ($line in $out) {
        $m = [regex]::Match($line, '^\s*DLL Name:\s*(.+?)\s*$')
        if ($m.Success) {
            $n = $m.Groups[1].Value.Trim()
            if ($n) { $names.Add($n) }
        }
    }
    return ,$names
}

$searchDirs = New-Object System.Collections.Generic.List[string]
if ($MsysPrefix) { $searchDirs.Add((Join-Path $MsysPrefix "bin")) }
if ($BuildDir) {
    $searchDirs.Add($BuildDir)
    $searchDirs.Add((Join-Path $BuildDir "_deps"))
}
foreach ($d in $ExtraSearchDirs) { if ($d) { $searchDirs.Add($d) } }

# Lower-cased DLL name -> full path, across all search locations.
$srcMap = @{}
foreach ($d in ($searchDirs | Select-Object -Unique)) {
    if (-not (Test-Path $d)) { continue }
    foreach ($f in (Get-ChildItem $d -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -eq ".dll" })) {
        $key = $f.Name.ToLowerInvariant()
        if (-not $srcMap.ContainsKey($key)) { $srcMap[$key] = $f.FullName }
    }
}

$present = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$queue = [System.Collections.Generic.Queue[string]]::new()
$processed = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

foreach ($f in (Get-ChildItem $DistDir -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in @(".dll", ".exe") })) {
    [void]$present.Add($f.Name)
    $queue.Enqueue($f.FullName)
}

$copiedCount = 0
while ($queue.Count -gt 0) {
    $file = $queue.Dequeue()
    if ($processed.Contains($file)) { continue }
    [void]$processed.Add($file)

    foreach ($name in (Get-DllImports $file)) {
        if ($present.Contains($name)) { continue }
        $key = $name.ToLowerInvariant()
        if ($srcMap.ContainsKey($key)) {
            $destName = Split-Path $srcMap[$key] -Leaf
            Copy-Item $srcMap[$key] (Join-Path $DistDir $destName) -Force
            [void]$present.Add($destName)
            $queue.Enqueue((Join-Path $DistDir $destName))
            $copiedCount++
            Write-Host "    + $destName"
        }
    }
}
Write-Done "Copied $copiedCount additional DLL(s) into $DistDir"

# ── Step 3: verification gate ──────────────────────────────────
if ($SkipVerify) { exit 0 }

Write-Step "Verifying that every import is satisfied by dist or a Windows system DLL..."

$osSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($n in @(
    "advapi32.dll", "authz.dll", "bcrypt.dll", "cfgmgr32.dll", "combase.dll",
    "comctl32.dll", "comdlg32.dll", "crypt32.dll", "cryptbase.dll", "cryptsp.dll",
    "d2d1.dll", "d3d11.dll", "d3d12.dll", "dbghelp.dll", "dcomp.dll", "dnsapi.dll",
    "dsound.dll", "dwmapi.dll", "dwrite.dll", "dxgi.dll", "dxva2.dll", "fltlib.dll",
    "gdi32.dll", "gdiplus.dll", "glu32.dll", "imm32.dll", "iphlpapi.dll",
    "kernel32.dll", "kernelbase.dll", "mpr.dll", "msimg32.dll", "msvcp140.dll",
    "msvcp140_1.dll", "msvcp140_2.dll", "msvcp140_codecvt_ids.dll", "msvcrt.dll",
    "netapi32.dll", "ntdll.dll", "ntmarta.dll", "ole32.dll", "oleaut32.dll",
    "opengl32.dll", "powrprof.dll", "propsys.dll", "rpcrt4.dll", "secur32.dll",
    "setupapi.dll", "shell32.dll", "shlwapi.dll", "user32.dll", "userenv.dll",
    "uxtheme.dll", "vcruntime140.dll", "vcruntime140_1.dll", "version.dll",
    "wininet.dll", "winmm.dll", "winspool.drv", "wlanapi.dll", "ws2_32.dll",
    "wtsapi32.dll", "xinput1_3.dll", "xinput1_4.dll", "xinput9_1_0.dll"
)) { [void]$osSet.Add($n) }

$system32 = if ($env:SystemRoot) { Join-Path $env:SystemRoot "System32" } else { $null }

$missing = New-Object System.Collections.Generic.List[string]
foreach ($f in (Get-ChildItem $DistDir -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in @(".dll", ".exe") })) {
    foreach ($name in (Get-DllImports $f.FullName)) {
        if ($present.Contains($name)) { continue }
        if ($osSet.Contains($name)) { continue }
        if ($name -match "^(api-ms-win-|ext-ms-)") { continue }
        if ($system32 -and (Test-Path (Join-Path $system32 $name))) { continue }
        $missing.Add("$name  (required by $($f.Name))")
    }
}

if ($missing.Count -gt 0) {
    Write-Host "[E] Unresolved imports:"
    foreach ($m in ($missing | Sort-Object -Unique)) { Write-Host "    $m" }
    Write-Fail "Deployment is incomplete; the packaged app would fail to start."
}
Write-Done "All imports resolved ($($missing.Count) unresolved)"
exit 0
