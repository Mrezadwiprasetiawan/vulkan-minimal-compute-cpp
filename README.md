# Vulkan Minimal Compute C++

This repository provides a quick reference and library to performing compute operations using Vulkan in C++.

## Features

- Vulkan Compute Pipeline: Demonstrates setting up a compute pipeline with Vulkan.
- Shader Integration: Includes example compute shaders.
- CMake Build System: Utilizes CMake for project configuration and building.

## Prerequisites

Ensure you have the following installed:

- Vulkan SDK: https://vulkan.lunarg.com/sdk/home
- CMake: https://cmake.org/download/
- A C++ compiler supporting C++11 or later

## Installation

1. Clone the repository:

```bash
git clone https://github.com/Mrezadwiprasetiawan/vulkan-minimal-compute-cpp.git
cd vulkan-minimal-compute-cpp
```

2. Configure the project with CMake:
```bash
cmake .
```

# Usage
1. Build the quick project:
```bash
make quick
```
2. Build the library
```bash
make vkmincomp
```

## Directory Structure
```txt
vulkan-minimal-compute-cpp/
├── CMakeLists.txt         # CMake configuration file
├── lib/                   # Vulkan minimal compute library
│   ├── src/               # Source files for the library
│   │   ├── compute.cxx    # Core Vulkan compute implementation
│   │   └── ...
│   ├── include/           # Header files for the library
│   │   ├── vkmincomp.hxx  # Public interface for Vulkan compute
│   │   └── ...
├── quick/                 # Quick reference example
│   ├── main.cxx           # Quick compute example
│   └── ...
├── reference/             # Vulkan documentation from Khronos
│   ├── vkspecs.pdf        # Vulkan reference materials
│   └── ...
├── shaders/               # Shader files
│   ├── compute.spv        # Compiled compute shader
│   └── ...
└── README.md              # Project documentation
```
## Contributing

Contributions are welcome. Please fork the repository and submit a pull request with your changes.

## License

This project is licensed under the MIT License. See the !(LICENSE) file for details.
