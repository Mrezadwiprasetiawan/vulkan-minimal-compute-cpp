# Vulkan Minimal Compute C++

This repository provides a minimal example of performing compute operations using Vulkan in C++.

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

2. Create a build directory and navigate into it:

```bash
mkdir build
cd build
```

3. Configure the project with CMake:

```bash
cmake ..
```

4. Build the project:


```bash
make help
```

## Usage

After building, run the application:

```bash
./vulkan_compute_example
```

This will execute a simple compute operation using Vulkan and display the results.

## Directory Structure

vulkan-minimal-compute-cpp/
  ├── CMakeLists.txt      # CMake configuration file
  ├── src/                # Source code
  │   ├── main.cpp        # Main application code
  │   └── ...
  ├── shaders/            # Shader source files
  │   ├── compute_shader.comp
  │   └── ...
  └── README.md           # Project documentation

## Contributing

Contributions are welcome. Please fork the repository and submit a pull request with your changes.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
