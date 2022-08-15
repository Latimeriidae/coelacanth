# coelacanth
[![GitHub Actions Status](https://github.com/Latimeriidae/coelacanth/actions/workflows/workflow.yml/badge.svg)](https://github.com/Latimeriidae/coelacanth/actions)

coelacanth random code generator

## Config, build, test and install
### Option 1: Send commands to docker container
**Requirements:** `make`, `docker`, `python3`

```bash
make in_docker TARGET=conan
make in_docker TARGET=config
make in_docker TARGET=build
make in_docker TARGET=test
make in_docker TARGET=install
```

### Option 2: Use docker container interactively
**Requirements:** `make`, `docker`, `python3`

```bash
make container
docker attach cpp_contests_container
make conan
make config
make build
make test
make install
```

### Option 3: Manually config your system
**Requirements:** `C++ compiler`, `CMake`, `make`, `python3`, `Conan`, `Ninja`  
**Optional requirements:** `FileCheck`, `lit`, `llvm-profdata`, `llvm-cov`, `gcovr`, specific `Conan` profiles

`CI` in this repo also uses custom `Conan` profiles, which allow to use different compilers and sanitizers. Although it is not required, you may want to use similar profiles.
In this case refer to scripts, which were used to create docker images:
[Linux](https://github.com/rudenkornk/docker_cpp#3-use-scripts-from-this-repository-to-setup-your-own-system),
[Windows](https://github.com/rudenkornk/docker_cpp_windows/#2-use-scripts-from-this-repository-to-setup-your-own-system).

```bash
make conan
make config
make build
make test
make install
```

### Option 4: Manually config your system and manually run commands
**Requirements:** `C++ compiler`, `CMake`, `python3`, `Conan`

```bash
conan install --build missing --install-folder build scripts/conanfile.py
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
cmake --build build
ctest --test-dir build
cmake --install build --prefix build/install/
```

