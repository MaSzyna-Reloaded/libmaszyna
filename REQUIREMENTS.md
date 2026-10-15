# Requirements

What building libmaszyna and what it releases needs.

## Linux binaries: glibc 2.34

Every Linux binary of MaSzyna Reloaded that is released - a library (`.so`), an engine template, an
executable - is built against **glibc 2.34**, in libmaszyna's Linux SDK image
`ghcr.io/maszyna-reloaded/linux-sdk` (libmaszyna's `ci/docker/linux-sdk`: Godot's own buildroot
SDK, whose sysroot carries glibc 2.34; the image is tagged by the start of its Dockerfile's sha256).
glibc is only forward compatible: a binary linked against a newer host's glibc refuses to start on
any older distribution (Ubuntu 22.04/24.04, Debian 12, Mint). A binary built on the host is for that
machine only, never released. Every repository building or going to build binaries keeps this rule.

`make compile-release-linux` (and the engine's Linux templates) run in that image (`LINUX_SDK_RUN`,
Docker); `make linux-sdk-image-push` publishes a changed Dockerfile's image.

## Engine

Godot 4.7.2, double precision (`godot-double`): libmaszyna's `GODOT_VERSION`, built once by
libmaszyna's CI and published as its release `godot-4.7.2-double` (editor and export templates),
which the other repositories fetch (libmaszyna's `ci/fetch-godot.sh`). Single precision builds -
Godot's own, Terrain3D's own - do not load.

## Windows binaries

Cross-compiled on Linux by MinGW-w64 with posix threads (`x86_64-w64-mingw32-g++-posix`, set by
`update-alternatives` as the CI does).

## Android

Any NDK for the GDExtension (`ANDROID_NDK_ROOT`, GitHub's runners carry one); the engine's Android
templates need `ANDROID_HOME` and a JDK 17.

## Tools

- CMake 3.30 or newer, Python 3 (README.md).
- Docker, for the Linux release.
