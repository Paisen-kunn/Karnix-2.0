# Karnix

Karnix is a research-oriented computing workspace that brings together C++ mathematical computing, neural-network building blocks, spine image screening, and a web-based spine analytics dashboard.

The repository is organized as several applications and reusable components rather than one monolithic build. Use the section for the component you want to run.

## Components

- **Mathematical and autograd demo**: `main.c++` exercises vector and matrix operations, calculus, graph visualizations, computational graphs, automatic differentiation, and neural-network concepts.
- **Spine Edge**: `karnix_spine_edge/` contains an offline C++ spine screening CLI with image analysis, structured vertebral-column analysis, clinical scoring, JSON records, and optional voice control on macOS.
- **Spine Analytics Dashboard**: `spine_analytics_dashboard/` contains a Vite/React frontend and an Express/PostgreSQL backend for viewing and serving analytics.
- **Reusable C++ libraries**: `mathematical/` and `neural_network/` contain matrix, vector, calculus, tensor, layer, optimizer, loss, and computational-graph code.
- **3D viewer**: `spine_3d/` contains the C++ spine visualization application.
- **Karnix OS experiments**: `os/` and `private/os copy/` contain separate operating-system experiments with their own Makefiles.

## Requirements

Install the tools needed by the component you plan to use:

- macOS with Apple Clang or another C++14-compatible compiler
- CMake 3.10 or newer for the Spine Edge CMake build
- Node.js and npm for the analytics dashboard
- PostgreSQL for dashboard persistence, if the backend is configured to use it
- Python 3 and macOS Speech commands for voice control
- Optional native dependencies described by the individual component documentation

## Mathematical Demo

Build and run the root C++ demonstration from the repository root:

```bash
clang++ -std=c++14 -I. main.c++ -o karnix_demo
./karnix_demo
```

The program prints demonstrations for:

- vector and matrix arithmetic
- trace, rank, eigenvalue, eigenvector, and projection examples
- elementary and multivariate calculus
- differentiation, integration, gradients, Jacobians, and Hessians
- ASCII graph visualizations
- computational graphs, backpropagation, and activation gradients

The source is intentionally demonstration-oriented. It is not a packaged library or production training pipeline.

## Spine Edge CLI

The detailed guide for the screening application is [karnix_spine_edge/readme.md](karnix_spine_edge/readme.md). The shortest manual build path is:

```bash
cd karnix_spine_edge
clang++ -std=c++14 -O3 spine_cli.c++ -o spine_cli
./spine_cli
```

The CMake path is:

```bash
cd karnix_spine_edge
cmake -S . -B build
cmake --build build
./build/spine_cli
```

Place supported input images in `karnix_spine_edge/data/images/`. Screening records are written to `karnix_spine_edge/data/spine_records.json`.

On macOS, the voice-control wrapper can be used after reviewing its setup requirements:

```bash
cd karnix_spine_edge
./voice.sh help
./voice.sh setup
./voice.sh scan
```

If the optional ONNX models are unavailable, the CLI can run in simulation or feature-based development mode as described in [karnix_spine_edge/models/README.txt](karnix_spine_edge/models/README.txt).

## Analytics Dashboard

The dashboard has independent frontend and backend projects. Install dependencies in each directory:

```bash
cd spine_analytics_dashboard/backend
npm install

cd ../frontend
npm install
```

Start the API and frontend in separate terminals:

```bash
# Terminal 1
cd spine_analytics_dashboard/backend
npm start

# Terminal 2
cd spine_analytics_dashboard/frontend
npm run dev
```

The Vite development server prints the local URL when it starts. Configure backend environment variables using the project files in `spine_analytics_dashboard/backend/config/` and the dashboard documentation:

- [spine_analytics_dashboard/README.md](spine_analytics_dashboard/README.md)
- [spine_analytics_dashboard/QUICKSTART.md](spine_analytics_dashboard/QUICKSTART.md)
- [spine_analytics_dashboard/API_REFERENCE.md](spine_analytics_dashboard/API_REFERENCE.md)

Useful frontend commands:

```bash
cd spine_analytics_dashboard/frontend
npm run build
npm run preview
```

Useful backend commands:

```bash
cd spine_analytics_dashboard/backend
npm run dev
./test_api.sh
```

## Repository Layout

```text
.
├── main.c++                     # Mathematical and autograd demonstration
├── mathematical/                # C++ mathematics and visualization utilities
├── neural_network/              # C++ tensors, layers, losses, and optimizers
├── karnix_spine_edge/           # Offline spine screening CLI
├── spine_analytics_dashboard/   # React frontend and Express backend
├── spine_3d/                    # C++ spine viewer
├── os/                          # Operating-system experiment
├── data/                        # Shared data and image assets
└── package.json                 # Root JavaScript dependency metadata
```

## Medical and Research Disclaimer

The spine screening and analytics code is intended for research and educational use. It is not a medical device and must not be used as a substitute for diagnosis, treatment, or advice from a qualified healthcare professional. Any generated finding must be independently reviewed by an appropriately qualified clinician.

## Development Notes

- Keep component-specific dependencies and build outputs inside their component directories.
- Treat files under `data/` as runtime or sample data unless a component explicitly documents them as source data.
- Review the component README files before enabling optional model, database, or voice integrations.