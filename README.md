# coelacanth
[![GitHub Actions Status](https://github.com/Latimeriidae/coelacanth/actions/workflows/workflow.yml/badge.svg)](https://github.com/Latimeriidae/coelacanth/actions)

coelacanth random code generator

## Prerequisites
### Option 1: Use docker container interactively:
```bash
make coelacanth_container
docker attach coelacanth_container
```
### Option 2: Send commands to docker container:
```bash
make in_docker COMMAND=<command>
```

### Option 3: TODO: create dependencies file and reference it here

## Config, build, test and install
### Option 1 (for users): Just use CMake
```bash
cmake -B build
cmake --build build
ctest --test-dir build
cmake --install build --prefix build/install/
```

### Option 2 (for developers): Use make wrapper with preconfigured recommended settings
```bash
make config
make build
make test
make install
```
