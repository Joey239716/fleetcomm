# FleetComm

A real-time vehicle fleet simulation that lets you compare two driving paradigms side-by-side:

- **Human mode** — cars navigate using sight cones and reaction-time delays, the way a human driver would
- **V2V mode** (Vehicle-to-Vehicle) — cars broadcast their position and hazard status to nearby vehicles, allowing the fleet to react collectively to breakdowns and slow traffic

72 cars (Tesla Model 3, Ford Transit, Toyota Prius, BMW i3, Honda Civic) drive autonomously around a hardcoded road graph in San Francisco. You watch them move on a live map and can toggle modes at any time to see the difference.

## Architecture

```
C++ simulation  ──ZMQ pub (5555)──►  Go gateway  ──WebSocket──►  React frontend
                ◄──ZMQ pull (5556)──             (port 8080)      (port 5173)
```

- **C++ engine** (`src/`) — runs the physics, pathfinding (Dijkstra), collision/breakdown logic, and publishes `WorldState` protobuf frames
- **Go gateway** (`gateway/`) — subscribes to ZMQ frames, converts to JSON, and fans them out to all connected browser clients via WebSocket; also accepts mode-switch commands from the frontend and forwards them to the sim
- **React frontend** (`frontend/`) — renders cars on a Leaflet map in real time; has a toggle to switch between human/V2V modes

## Prerequisites

| Tool | Minimum version |
|---|---|
| Node.js + npm | 18+ |
| Go | 1.21+ |
| CMake | 3.20+ |
| Visual Studio Build Tools (Windows) | 2019+ |

> On Windows, the `setup.ps1` script installs Go, CMake, and protoc automatically.  
> Visual Studio Build Tools must be installed manually — download from [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) → **Build Tools for Visual Studio**.

## Quick start (Windows)

**1. Clone the repo**
```powershell
git clone https://github.com/Joey239716/fleetcomm.git
cd fleetcomm
```

**2. Run the setup script** *(one-time, run as Administrator)*
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\setup.ps1
```

This installs all dependencies, builds the C++ simulation, and installs frontend/Go packages. Takes ~10 minutes the first time.

**3. Open three terminals and run each component**

Terminal 1 — C++ simulation:
```powershell
$env:PATH = "C:\vcpkg\installed\x64-windows\bin;$env:PATH"
.\build_win\Release\fleetcomm.exe
```

Terminal 2 — Go gateway:
```powershell
cd gateway
go run main.go hub.go
```

Terminal 3 — React frontend:
```powershell
cd frontend
npm run dev
```

**4. Open [http://localhost:5173](http://localhost:5173)**

## Quick start (macOS / Linux)

Install dependencies via Homebrew (macOS) or your package manager:
```bash
brew install cmake zeromq cppzmq protobuf go node
```

Then build and run:
```bash
cmake -B build -S .
cmake --build build
./build/fleetcomm &          # terminal 1
cd gateway && go run main.go hub.go &   # terminal 2
cd frontend && npm install && npm run dev  # terminal 3
```

## Switching modes

Once the app is running, use the toggle in the top-right corner of the map to switch between **Human** and **V2V** mode. The change takes effect immediately in the live simulation.

## Project structure

```
fleetcomm/
├── src/                  C++ simulation source
│   ├── main.cpp          entry point, ZMQ publisher loop
│   ├── traffic_simulation.cpp  human driving mode
│   ├── v2v_simulation.cpp      V2V driving mode
│   ├── car.cpp / car.hpp       vehicle model, sight cones
│   ├── graph.cpp               road graph + Dijkstra
│   └── spatial_grid.cpp        proximity queries
├── proto/                protobuf schema
├── gateway/              Go WebSocket gateway
├── frontend/             React + Leaflet UI
├── vcpkg.json            C++ dependency manifest
└── setup.ps1             Windows one-time setup script
```
