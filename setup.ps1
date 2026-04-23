# FleetComm — Windows setup script
# Run once from the repo root as Administrator:
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#   .\setup.ps1

param(
    [string]$VcpkgRoot = "C:\vcpkg"
)

$ErrorActionPreference = "Stop"
$repoRoot = $PSScriptRoot

function Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Ok($msg)   { Write-Host "    OK: $msg" -ForegroundColor Green }
function Warn($msg) { Write-Host "    WARN: $msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------------------
# 1. Winget packages
# ---------------------------------------------------------------------------
Step "Checking / installing runtimes (Go, CMake, protoc)"

$winget = Get-Command winget -ErrorAction SilentlyContinue
if (-not $winget) {
    Write-Error "winget is not available. Install App Installer from the Microsoft Store and re-run."
}

$packages = @(
    @{ Id = "GoLang.Go";        Name = "Go"     },
    @{ Id = "Kitware.CMake";    Name = "CMake"  },
    @{ Id = "Google.Protobuf";  Name = "protoc" }
)

foreach ($pkg in $packages) {
    $installed = winget list --id $pkg.Id 2>&1 | Select-String $pkg.Id
    if ($installed) {
        Ok "$($pkg.Name) already installed"
    } else {
        Write-Host "    Installing $($pkg.Name)..."
        winget install $pkg.Id --accept-package-agreements --accept-source-agreements
        Ok "$($pkg.Name) installed"
    }
}

# Reload PATH so go/cmake are usable in this session
$env:PATH = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
            [System.Environment]::GetEnvironmentVariable("Path", "User")

# ---------------------------------------------------------------------------
# 2. vcpkg
# ---------------------------------------------------------------------------
Step "Setting up vcpkg at $VcpkgRoot"

if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    if (-not (Test-Path $VcpkgRoot)) {
        Write-Host "    Cloning vcpkg..."
        git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
    }
    Write-Host "    Bootstrapping vcpkg..."
    & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
    Ok "vcpkg bootstrapped"
} else {
    Ok "vcpkg already present"
}

# ---------------------------------------------------------------------------
# 3. C++ dependencies + build
# ---------------------------------------------------------------------------
Step "Installing C++ dependencies via vcpkg (zeromq, cppzmq, protobuf)"
Set-Location $repoRoot
& "$VcpkgRoot\vcpkg.exe" install
Ok "C++ dependencies installed"

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue)?.Source
if (-not $cmake) { $cmake = "C:\Program Files\CMake\bin\cmake.exe" }

Step "Configuring CMake"
& $cmake -B build_win -S . `
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows
Ok "CMake configured"

Step "Building C++ simulation"
& $cmake --build build_win --config Release
Ok "fleetcomm.exe built -> build_win\Release\fleetcomm.exe"

# ---------------------------------------------------------------------------
# 4. Go gateway
# ---------------------------------------------------------------------------
Step "Downloading Go dependencies"
$goExe = (Get-Command go -ErrorAction SilentlyContinue)?.Source
if (-not $goExe) { $goExe = "C:\Program Files\Go\bin\go.exe" }

Set-Location "$repoRoot\gateway"
& $goExe mod tidy
Ok "Go dependencies ready"

# ---------------------------------------------------------------------------
# 5. Frontend
# ---------------------------------------------------------------------------
Step "Installing frontend npm packages"
Set-Location "$repoRoot\frontend"
npm install
Ok "npm packages installed"

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
Set-Location $repoRoot
Write-Host ""
Write-Host "================================================================" -ForegroundColor Green
Write-Host "  Setup complete! Start the app with three terminals:"           -ForegroundColor Green
Write-Host ""
Write-Host "  Terminal 1 (C++ sim):"
Write-Host '    $env:PATH = "C:\vcpkg\installed\x64-windows\bin;$env:PATH"'
Write-Host "    .\build_win\Release\fleetcomm.exe"
Write-Host ""
Write-Host "  Terminal 2 (Go gateway):"
Write-Host "    cd gateway; go run main.go hub.go"
Write-Host ""
Write-Host "  Terminal 3 (Frontend):"
Write-Host "    cd frontend; npm run dev"
Write-Host ""
Write-Host "  Then open: http://localhost:5173"
Write-Host "================================================================" -ForegroundColor Green
