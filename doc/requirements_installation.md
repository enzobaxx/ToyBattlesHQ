# 3.1 Requirements & Installation

## 3.1.1 Requirements
- Python >= 3.10
- CMake >= 3.26
- GCC >= 14 or MSVC >= 14.10 (Clang: untested)
- C++23 standard or newer

## 3.1.2 Installation
### 3.1.2.1 Automatic Linux Setup
If you want an easy setup for Linux (based on Docker), then [this repository](https://github.com/ToyBattles/MicroVolts-Server-Docker) is the best tool for you. Follow the steps shown on it and you'll be ready in only a few minutes.


### 3.1.2.2 Manual setup (Windows)
1) Clone this repository then go to its folder: `cd <YourEmulatorProjectPath>` - make sure you are inside the ToyBattlesHQ folder (root of this repository)
2) Clone vcpkg inside ExternalLibraries: `git clone https://github.com/microsoft/vcpkg.git ExternalLibraries\vcpkg`
3) Bootstrap it: `.\ExternalLibraries\vcpkg\bootstrap-vcpkg.bat`
4) Generate build files: `cmake -B build -S . -A x64 -DCMAKE_TOOLCHAIN_FILE=ExternalLibraries/vcpkg/scripts/buildsystems/vcpkg.cmake`
5) Build the project: `cmake --build build --config Release`
6) Output (exes) inside Release folder

### 3.1.2.3 Manual setup (Linux)
1) Clone this repository then go to its folder: `cd <YourEmulatorProjectPath>` - make sure you are inside the ToyBattlesHQ folder (root of this repository)
2) Clone vcpkg inside ExternalLibraries: `git clone https://github.com/microsoft/vcpkg.git ExternalLibraries/vcpkg`
3) Bootstrap it: `./ExternalLibraries/vcpkg/bootstrap-vcpkg.sh`
4) Generate build files: `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=ExternalLibraries/vcpkg/scripts/buildsystems/vcpkg.cmake`
5) Build the project: `cmake --build build --config Release`
6) Move the generated elf files in an output folder: `mkdir -p Output` and next `mv AuthServer.elf MainServer.elf CastServer.elf Output/`

## Next
[3.2 Setting up the emulator](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/setting_up.md)



