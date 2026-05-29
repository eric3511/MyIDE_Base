
[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "Both")]
    [string]$Configuration = "Both",

    [string]$InstallDrive = $null,

    [string]$InstallDir = "IDElib",

    [switch]$Clean,

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

# ---- Resolve project root ----
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path "$scriptDir\.."
Write-Host "Project root: $projectRoot"

# ---- Resolve drive ----
if (-not $InstallDrive) {
    $InstallDrive = (Split-Path -Qualifier $projectRoot)
}
$installPrefix = Join-Path "$InstallDrive\" $InstallDir
Write-Host "Install prefix: $installPrefix"

# ---- Qt setup ----
$QtDir = "C:\Qt\6.8.3\msvc2022_64"
$QtBin = Join-Path $QtDir "bin"
if (-not (Test-Path $QtBin)) {
    Write-Error "Qt not found at $QtDir"
    exit 1
}

# ---- MSVC environment ----
function Invoke-VsDevShell {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        Write-Error "vswhere.exe not found — Visual Studio may not be installed"
        exit 1
    }
    $vsPath = & $vswhere -latest -products * -property installationPath
    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvars)) {
        Write-Error "vcvars64.bat not found at $vcvars"
        exit 1
    }

    Write-Host "Loading MSVC environment from: $vsPath"
    $bat = """$vcvars"" >nul 2>&1 && set"
    cmd /c $bat | ForEach-Object {
        if ($_ -match '^(.*?)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }
}

# ---- CMake configure ----
function Invoke-Configure {
    param([string]$BuildDir, [string]$BuildType)
    if ($BuildType -notin @("Debug", "Release")) {
        throw "Invalid BuildType: '$BuildType'. Must be Debug or Release."
    }
    Write-Host "  Configuring $BuildType (Qt: $QtDir) ..."
    & cmake -S $projectRoot -B $BuildDir -G Ninja @(
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DCMAKE_PREFIX_PATH=$QtDir"
    )
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for $BuildType" }
}

# ---- CMake build ----
function Invoke-Build {
    param([string]$BuildDir, [string]$BuildType)
    Write-Host "  Building $BuildType ..."
    cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "Build failed for $BuildType" }
}

# ---- CMake install ----
function Invoke-Install {
    param([string]$BuildDir, [string]$BuildType)
    $dest = Join-Path $installPrefix $BuildType
    Write-Host "  Installing $BuildType to: $dest"

    # Clean destination
    if ($Clean -and (Test-Path $dest)) {
        Remove-Item -Recurse -Force $dest
    }

    cmake --install $BuildDir --prefix $dest --config $BuildType
    if ($LASTEXITCODE -ne 0) { throw "CMake install failed for $BuildType" }
}

# ---- Deploy Qt DLLs (for runtime convenience) ----
function Invoke-Windeployqt {
    param([string]$BuildType)
    $dest = Join-Path (Join-Path $installPrefix $BuildType) "bin"
    $windeployqt = Join-Path $QtBin "windeployqt.exe"
    $exe = Join-Path $dest "MyIDE.exe"

    if (-not (Test-Path $exe)) {
        Write-Warning "  MyIDE.exe not found at $exe — skipping windeployqt"
        return
    }
    Write-Host "  Running windeployqt for $BuildType ..."
    & $windeployqt $exe --no-translations --no-compiler-runtime 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Warning "  windeployqt might have issues (exit=$LASTEXITCODE)" }
}

# ==== Main ====

Invoke-VsDevShell

$debugBuildDir = Join-Path $projectRoot "build\Desktop_Qt_6_8_3_MSVC2022_64bit-Debug"
$releaseBuildDir = Join-Path $projectRoot "build\Desktop_Qt_6_8_3_MSVC2022_64bit-Release"

$targets = @()
if ($Configuration -eq "Debug"   -or $Configuration -eq "Both") { $targets += @{Dir=$debugBuildDir;   Type="Debug"}   }
if ($Configuration -eq "Release" -or $Configuration -eq "Both") { $targets += @{Dir=$releaseBuildDir; Type="Release"} }

Write-Host ""
Write-Host "===== Phase 1: Configure & Build ====="
foreach ($t in $targets) {
    Write-Host ""
    Write-Host "--- $($t.Type) ---"
    Invoke-Configure -BuildDir $t.Dir -BuildType $t.Type
    if (-not $SkipBuild) {
        Invoke-Build -BuildDir $t.Dir -BuildType $t.Type
    }
}

Write-Host ""
Write-Host "===== Phase 2: Install ====="
foreach ($t in $targets) {
    Write-Host ""
    Write-Host "--- $($t.Type) ---"
    Invoke-Install -BuildDir $t.Dir -BuildType $t.Type
}

Write-Host ""
Write-Host "===== Phase 3: Deploy Qt runtime ====="
foreach ($t in $targets) {
    Write-Host ""
    Write-Host "--- $($t.Type) ---"
    Invoke-Windeployqt -BuildType $t.Type
}

Write-Host ""
Write-Host "===== Done ====="
Write-Host "Installed to: $installPrefix"
Get-ChildItem $installPrefix -Recurse -Depth 2 | Where-Object { -not $_.PSIsContainer } | ForEach-Object {
    $rel = $_.FullName.Replace($installPrefix, '')
    Write-Host "  $rel"
}
