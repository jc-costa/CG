## Building

### Prerequisites
- CMake 3.28 or higher
- C++20 compatible compiler (GCC, Clang, or MSVC)
- OpenGL development libraries

### Build Instructions

#### On macOS/Linux:
```bash
mkdir build
cd build
cmake ..
make
```

The executable will be located at `build/App/App`

#### On Windows (Visual Studio):
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The executable will be located at `build/App/Release/App.exe`

**Note:** Dependencies (GLFW, GLAD, GLM) are automatically downloaded during the CMake configuration step.

Tested on Windows 11 using Visual Studio 2022, more testing to follow.

## Third-Party Dependencies
Uses the following third-party dependencies:
- [GLFW 3.4](https://github.com/glfw/glfw)
- [Glad](https://github.com/Dav1dde/glad)
- [glm](https://github.com/g-truc/glm)
