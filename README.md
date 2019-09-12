# Telegram VOIP library

This repository is originally intended to be a submodule of Telegram Desktop client. The present fork introduces a standalone CMake build with an intention to build 3rd party Telegram clients with VOIP support.

## Standalone build with CMake

Make sure the build system has CMake, Opus (e.g. opus-dev) and a C/C++ compiler (e.g. gcc):

```
mkdir build
cd build
cmake ..
make -j48
```

