# Telegram VOIP library

This repository is originally intended to be a submodule of Telegram Desktop client. The present fork introduces a standalone CMake build with an intention to build 3rd party Telegram clients with VOIP support.

## Standalone build with CMake

Make sure the build system has CMake, Opus (e.g. opus-dev) and a C/C++ compiler (e.g. gcc):

```bash
mkdir build
cd build
cmake ..
make -j48
```

## Unit testing

Note admin rights are required for proper sockets processing. Alternatively, you may want to fine-tune permissions of a regular user.

```bash
sudo ./tgvoip_test_audio
```

```
[       OK ] TestAudio.testPacketTimeout (5047 ms)
[----------] 4 tests from TestAudio (48875 ms total)

[----------] Global test environment tear-down
[==========] 4 tests from 1 test suite ran. (48875 ms total)
[  PASSED  ] 4 tests.
```

