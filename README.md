# Vulkan Minimal Compute C++

This repository provides a quick reference and library for performing minimal computations with Vulkan in C++.

## Features

- Minimal Vulkan Compute Operations: Basic implementations to get started with Vulkan compute operations.
- Example Shaders: Includes sample shaders for demonstration purposes.
- CMake Build System: Configured with CMake for easy compilation.

## Prerequisites

Before you begin, ensure you have the following installed:

- Vulkan SDK: https://vulkan.lunarg.com/sdk/home
- CMake: https://cmake.org/download/
- A C++ compiler that supports C++11 or later

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

3. Run CMake and compile:

```bash
  cmake ..
  make
  ```

## Usage

After a successful build, you can run the application with the following command:

```bash
  ./vulkan_compute_example
  ```

The application will execute a simple computation using Vulkan and display the results in the console.

## Directory Structure

The main directory structure of this repository is as follows:

vulkan-minimal-compute-cpp/
  ├── lib/                # Third-party libraries
  ├── quick/              # Vulkan compute implementations
  ├── shaders/            # Shader source code
  ├── CMakeLists.txt      # CMake configuration file
  └── LICENSE             # Project license
7
## Contributing

Contributions are welcome. Please fork this repository and submit a pull request for any improvements or new features.

## License

This project is licensed under the GPL-3.0 License. See the LICENSE file for details.
