# Toolchain for the Linux release. glibc is only forward compatible, so a library or a template
# built on the host's glibc refuses to start on any older distribution. Godot's own buildroot SDK
# carries a sysroot with an old glibc (2.34) and a current GCC, and it is the SDK the official
# Godot 4.7 builds use (godotengine/build-containers, Dockerfile.linux). The checkout is never
# copied in: the Makefile mounts it (see release-linux).
# Debian's official image from the AWS mirror, never Docker Hub: its anonymous pulls time out the CI
FROM public.ecr.aws/docker/library/debian:bookworm-slim
# links the ghcr.io package to its repository
LABEL org.opencontainers.image.source=https://github.com/MaSzyna-Reloaded/libmaszyna

ARG SDK_RELEASE=godot-2026.05.x-1
ARG SDK_NAME=x86_64-godot-linux-gnu_sdk-buildroot

# The SDK brings its own cmake, python, scons and wayland-scanner; ccache keeps the host's compiler
# cache in use (the Makefile mounts it)
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl bzip2 make ccache \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL "https://github.com/godotengine/buildroot/releases/download/${SDK_RELEASE}/${SDK_NAME}.tar.bz2" \
        | tar -xj -C /opt \
    && cd "/opt/${SDK_NAME}" && ./relocate-sdk.sh

ENV PATH="/opt/${SDK_NAME}/bin:${PATH}"
ENV CC=x86_64-godot-linux-gnu-gcc
ENV CXX=x86_64-godot-linux-gnu-g++
ENV HOME=/tmp
