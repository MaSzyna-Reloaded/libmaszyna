.PHONY: linux-sdk-image linux-sdk-image-push compile-release-linux compile-android-release compile-android-debug godot-version docs compile watch-and-compile api-docs docs-server docs-install docs-pdf cleanup style-check style-fix compile-release-symbols
.DEFAULT_GOAL = compile-debug

# The Godot project the library is built into (its bin/libmaszyna/) and whose addons/ CMake links
# the addon into - relative to this directory or absolute; a game with libmaszyna as a submodule
# passes its own: make -C vendor/libmaszyna compile-debug GODOT_PROJECT_DIR=$(CURDIR)
GODOT_PROJECT_DIR?=demo
CMAKE_PROJECT_ARGS=-DGODOT_PROJECT_DIR=$(GODOT_PROJECT_DIR)
CMAKE_BUILD_JOBS=$(shell cores=$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1); if [ "$$cores" -gt 2 ]; then echo $$((cores - 2)); else echo 1; fi)
CLANG_TIDY_BUILD_DIR=build-clang-tidy
CLANG_TIDY_COMPILE_COMMANDS_FILE=$(CLANG_TIDY_BUILD_DIR)/compile_commands.json
CLANG_TIDY_BINDINGS_FILE=$(CLANG_TIDY_BUILD_DIR)/godot-cpp/gen/include/godot_cpp/classes/node.hpp
LIBMASZYNA_DEBUG:=""
# The extension is built against double precision godot-cpp, so only a Godot built the same way
# can load it - a single precision binary dies with a glibc heap assertion while the module
# initialises. Override when your double precision build is named differently:
#   make compile-debug GODOT=godot-double
GODOT?=$(shell command -v godot-double >/dev/null && echo godot-double || echo godot)
CMAKE_GODOTCPP_API_VERSION=4.7
# f21961238's precision and API file: the double-precision API, dumped from $(GODOT) and never
# versioned; CMakeLists.txt binds it anyway (FORCE)
CMAKE_GODOTCPP_PRECISION=double
CMAKE_GODOTCPP_API_FILE=$(CURDIR)/extension_api.json
# The engine the library is built for - a game exporting with it has to use exactly this one, since
# the export looks its template up by this version
GODOT_VERSION:=4.7.2

# glibc is only forward compatible: a library or template linked against a rolling distribution's
# glibc (2.43-2.44 here) refuses to start on anything older - Ubuntu 22.04/24.04, Debian 12, Mint.
# The Linux release is therefore built in ci/docker/linux-sdk, Godot's own buildroot SDK
# (glibc 2.34). The checkout is mounted at its own path and the build runs as the host user, so
# the cmake cache and every output land exactly where a host build would put them; a game building
# into its own project mounts its checkout, which holds this one (LINUX_SDK_MOUNT).
# Published on ghcr.io and tagged by its Dockerfile: the CI pulls it rather than building it, and a
# changed Dockerfile is a new tag, built here and published with make linux-sdk-image-push
LINUX_SDK_IMAGE:=ghcr.io/maszyna-reloaded/linux-sdk:$(shell sha256sum ci/docker/linux-sdk/Dockerfile | cut -c1-12)
LINUX_SDK_MOUNT?=$(CURDIR)
# The host's ccache directory, mounted, so the container's builds reuse and fill the same cache
CCACHE_DIR?=$(HOME)/.cache/ccache
LINUX_SDK_RUN=mkdir -p $(CCACHE_DIR) && docker run --rm --user $(shell id -u):$(shell id -g) \
    -v $(LINUX_SDK_MOUNT):$(LINUX_SDK_MOUNT) -w $(CURDIR) -v $(CCACHE_DIR):/ccache -e CCACHE_DIR=/ccache \
    -e CMAKE_C_COMPILER_LAUNCHER=ccache -e CMAKE_CXX_COMPILER_LAUNCHER=ccache $(LINUX_SDK_IMAGE)
GODOT_SOURCE_DIR:=build-godot-$(GODOT_VERSION)
GODOT_BIN:=$(GODOT_SOURCE_DIR)/bin
GODOT_SCONS=scons precision=double production=yes -j$(CMAKE_BUILD_JOBS)
LINUX_TEMPLATE:=$(GODOT_BIN)/godot.linuxbsd.template_release.double.x86_64
ANDROID_TEMPLATES:=$(GODOT_BIN)/android_release.apk $(GODOT_BIN)/android_debug.apk

#Helper for CLion so it would see generated bindings
generate-bindings: $(CMAKE_GODOTCPP_API_FILE)
	cmake -B cmake-build-debug -DGODOTCPP_PRECISION=$(CMAKE_GODOTCPP_PRECISION) -DGODOTCPP_CUSTOM_API_FILE=$(CMAKE_GODOTCPP_API_FILE)
	cmake --build cmake-build-debug --target generate_bindings


docs:
	cd demo && $(GODOT) --doctool .. --gdextension-docs


cleanup:
	rm -rf bin
	rm -rf demo/bin
	rm -rf demo/addons/gut


cleanup-build-debug:
	rm -rf build-debug


cleanup-build-release:
	rm -rf build-release


cleanup-builds: cleanup-build-debug cleanup-build-release
	

compile-debug: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	cmake -B build-debug $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Debug -DGODOTCPP_TARGET=template_debug -DLIBMASZYNA_DEBUG=$(LIBMASZYNA_DEBUG) -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	cmake --build build-debug --parallel $(CMAKE_BUILD_JOBS)


compile-release:
	cmake -B build-release $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	cmake --build build-release --parallel $(CMAKE_BUILD_JOBS)


# Optimized, but still the template_debug library the editor loads - the one to profile on.
# compile-debug builds the vendored Mover at -O0, which makes the physics several times slower
# than it is in a shipped build and sends any frame-time investigation after the wrong subsystem.
# Overwrites the same .so as compile-debug; run that to go back.
compile-profiling:
	cmake -B build-profiling $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGODOTCPP_TARGET=template_debug -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	cmake --build build-profiling --parallel $(CMAKE_BUILD_JOBS)


# A shipped build that keeps its symbols, for turning a core dump into names. The switch that
# matters is the build type, not a flag of ours: godot-cpp links with -s unless DEBUG_SYMBOLS is
# on (its cmake/common_compiler_flags.cmake), and DEBUG_SYMBOLS is Debug or RelWithDebInfo. The
# price is -O2 instead of -O3, so this build is for diagnosing a crash and not for measuring frame
# times. It writes the same library as compile-release, so rebuild that one afterwards.
compile-release-symbols:
	cmake -B build-release-symbols $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	cmake --build build-release-symbols --parallel $(CMAKE_BUILD_JOBS)


compile-all: compile-debug compile-release


cross-compile-release: compile-release compile-windows-release


cross-compile-debug: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	cmake -B build-linux64-debug $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DLIBMASZYNA_DEBUG=ON \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	cmake --build build-linux64-debug --parallel $(CMAKE_BUILD_JOBS)
	cmake -B build-win64-debug $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PRECISION=$(CMAKE_GODOTCPP_PRECISION) -DGODOTCPP_CUSTOM_API_FILE=$(CMAKE_GODOTCPP_API_FILE) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	cmake --build build-win64-debug --parallel $(CMAKE_BUILD_JOBS)


compile-windows-debug:
	cmake -B build-win64-debug $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	cmake --build build-win64-debug --parallel $(CMAKE_BUILD_JOBS)


compile-windows-release:
	cmake -B build-win64-release $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Release \
          -DGODOTCPP_TARGET="template_release" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	cmake --build build-win64-release --parallel $(CMAKE_BUILD_JOBS)


# Any NDK will do for a GDExtension; GitHub's runners carry one in ANDROID_NDK_LATEST_HOME
ANDROID_NDK_ROOT?=$(ANDROID_NDK_LATEST_HOME)

compile-android-debug:
	cmake -B build-android-debug $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Debug -DGODOTCPP_TARGET=template_debug -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) -DGODOTCPP_PLATFORM=android -DANDROID_NDK_ROOT=$(ANDROID_NDK_ROOT)
	cmake --build build-android-debug --parallel $(CMAKE_BUILD_JOBS)


compile-android-release:
	cmake -B build-android-release $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) -DGODOTCPP_PLATFORM=android -DANDROID_NDK_ROOT=$(ANDROID_NDK_ROOT)
	cmake --build build-android-release --parallel $(CMAKE_BUILD_JOBS)


linux-sdk-image:
	docker image inspect $(LINUX_SDK_IMAGE) > /dev/null 2>&1 || docker pull -q $(LINUX_SDK_IMAGE) \
	    || docker build -q -t $(LINUX_SDK_IMAGE) ci/docker/linux-sdk


linux-sdk-image-push: linux-sdk-image
	docker push $(LINUX_SDK_IMAGE)



# The SDK container has no Godot, so the API the build binds against is dumped on the host first
$(CMAKE_GODOTCPP_API_FILE):
	cd $(CURDIR) && $(GODOT) --headless --dump-extension-api


compile-release-linux: $(CMAKE_GODOTCPP_API_FILE) linux-sdk-image
	$(LINUX_SDK_RUN) sh -c 'cmake -B build-release-linux $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) && cmake --build build-release-linux --parallel $(CMAKE_BUILD_JOBS)'


# The runtime of the cab Python screens, built in the SDK like the release. PythonScreenServer
# looks for it in the game dir: cp -a build-python-runtime/python2.7 <game_dir>/
PYTHON_RUNTIME_BUILD_DIR:=build-python-runtime
PYTHON_RUNTIME_DIR:=$(PYTHON_RUNTIME_BUILD_DIR)/python2.7

.PHONY: python-runtime
python-runtime: $(PYTHON_RUNTIME_DIR)/lib/libpython2.7.so.1.0

$(PYTHON_RUNTIME_DIR)/lib/libpython2.7.so.1.0: scripts/build-python-runtime | linux-sdk-image
	$(LINUX_SDK_RUN) scripts/build-python-runtime $(PYTHON_RUNTIME_DIR) $(PYTHON_RUNTIME_BUILD_DIR)


$(GODOT_SOURCE_DIR):
	git clone --depth 1 --branch $(GODOT_VERSION)-stable https://github.com/godotengine/godot.git $@


# Godot publishes no double precision build, so the engine is built here, once per engine version.
# CI does it in .github/workflows/godot-engine.yml and every other run fetches the result into
# $(GODOT_BIN) with ci/fetch-godot.sh, which is what keeps these rules from firing there.
godot-version:
	@echo $(GODOT_VERSION)


# The editor too is built in the SDK - the host's own is linked against the host's glibc, and this
# one runs on the CI runner as well
$(GODOT_BIN)/godot.linuxbsd.%.double.x86_64: | $(GODOT_SOURCE_DIR) linux-sdk-image
	$(LINUX_SDK_RUN) sh -c 'cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=linuxbsd arch=x86_64 target=$*'


# Direct3D 12 is not supported by the project, so the template is built without it and needs none
# of its SDK
$(GODOT_BIN)/godot.windows.%.double.x86_64.exe: | $(GODOT_SOURCE_DIR)
	cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=windows arch=x86_64 target=$* d3d12=no


# The export presets do not use a gradle build, so the export takes the ready APKs; scons puts the
# native libraries where gradle packs them from. Needs ANDROID_HOME and a JDK 17. Built without
# the Swappy frame pacing library, which scons refuses to build without otherwise.
$(ANDROID_TEMPLATES) &: | $(GODOT_SOURCE_DIR)
	cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=android arch=arm64 target=template_release swappy=no \
	    && $(GODOT_SCONS) platform=android arch=arm64 target=template_debug swappy=no \
	    && cd platform/android/java && ./gradlew generateGodotTemplates


# The class reference pages of the Jekyll site in docs/ (docs/api/, not versioned): C++ classes
# from doc_classes/*.xml - refresh those with `make docs` - and GDScript classes from their ##
# comments
api-docs:
	scripts/make-api-docs docs/api


docs-install:
	cd docs && make install


# Generates the class reference and serves the site; without Ruby on the host, both run in Docker
docs-server:
	cd docs && make runserver


# The site as one PDF: the home page, the guides in the order of its menu and the class reference
# (scripts/make-docs-book). The diagrams are drawn by mermaid-cli and the book typeset by pandoc
# with XeLaTeX, both in Docker, as the host user so that bin/docs stays the host's
DOCS_PDF_DIR:=bin/docs
DOCS_BOOK_DIR:=$(DOCS_PDF_DIR)/book
DOCS_PDF:=$(DOCS_PDF_DIR)/maszyna-reloaded-core.pdf
DOCS_PANDOC_IMAGE:=pandoc/extra:3.7
DOCS_MERMAID_IMAGE:=minlag/mermaid-cli:11.4.2
DOCS_DOCKER_RUN:=docker run --rm --user "$(shell id -u):$(shell id -g)" -v "$(CURDIR):/data" -w /data
docs-pdf:
	rm -rf $(DOCS_BOOK_DIR)
	mkdir -p $(DOCS_BOOK_DIR)
	scripts/make-api-docs $(DOCS_BOOK_DIR)/api
	scripts/make-docs-book $(DOCS_BOOK_DIR)
	for diagram in $(DOCS_BOOK_DIR)/*.mmd; do \
		$(DOCS_DOCKER_RUN) $(DOCS_MERMAID_IMAGE) -q -b white -s 2 -i "/data/$$diagram" -o "/data/$${diagram%.mmd}.png" || exit 1; \
	done
	$(DOCS_DOCKER_RUN) $(DOCS_PANDOC_IMAGE) $(DOCS_BOOK_DIR)/book.md --metadata-file=scripts/docs-pdf.yaml \
		--resource-path=$(DOCS_BOOK_DIR):docs --pdf-engine=xelatex --toc --toc-depth=2 \
		--top-level-division=chapter -o $(DOCS_PDF)
	@echo "Documentation: $(DOCS_PDF)"


watch-and-compile:
	sh scripts/autocompile.sh


$(CLANG_TIDY_COMPILE_COMMANDS_FILE): $(CMAKE_GODOTCPP_API_FILE)
	@echo "Style: configuring clang-tidy database..." && \
	cmake --log-level=ERROR -S . -B $(CLANG_TIDY_BUILD_DIR) -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)

$(CLANG_TIDY_BINDINGS_FILE): $(CLANG_TIDY_BUILD_DIR)/CMakeCache.txt
	@echo "Style: generating clang-tidy bindings..." && \
	cmake --build $(CLANG_TIDY_BUILD_DIR) --target generate_bindings

style-check: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	@scripts/style-check $(STYLE_FILE)


style-fix:
	@scripts/style-fix $(STYLE_FILE)

# A staged script or shader goes in with its .uid (scripts/check-staged-uids)
.PHONY: check-staged-uids install-git-hooks
check-staged-uids:
	@scripts/check-staged-uids

# The repository's hooks (scripts/git-hooks): pre-commit runs check-staged-uids
install-git-hooks:
	git config core.hooksPath scripts/git-hooks

docker-build-tests:
	docker build -t godot-tests .


docker-run-tests: docker-build-tests
	docker run --rm godot-tests


run-tests: compile-debug
	godot-double --path demo --headless -s addons/gut/gut_cmdln.gd -gdir=res://tests/ -gexit
