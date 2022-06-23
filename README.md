# coelacanth
[![GitHub Actions Status](https://github.com/Latimeriidae/coelacanth/actions/workflows/workflow.yml/badge.svg)](https://github.com/Latimeriidae/coelacanth/actions)

coelacanth random code generator

## Prerequisites
### Option 1: Use docker container interactively:
```bash
make container
docker attach coelacanth_container
```
### Option 2: Send commands to docker container:
```bash
make in_docker COMMAND=<command>
```

### Option 3: config your system
Basic requirements include recent versions of a `C++ compiler`, `CMake` and `Conan`.
`Conan` will do the rest.


## Config, build, test and install
### Option 1: Use make wrapper with preconfigured recommended settings
```bash
# make in_docker TARGET=<...>
make conan
make config
make build
make test
make install
```
Note that when using docker container non-interactively there is a shortcut for these makefile targets:
```bash
make in_docker TARGET=<makefile target>
# e.g.:
make in_docker TARGET=conan
make in_docker TARGET=config
# <... etc. ...>
```

### Option 2: Manually use Conan + CMake
```bash
conan install --build missing --install-folder build scripts/conanfile.py
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
cmake --build build
ctest --test-dir build
cmake --install build --prefix build/install/
```
