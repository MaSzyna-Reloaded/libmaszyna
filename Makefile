.PHONY: libreloaded-local libreloaded-submodule links compile-debug compile-release compile-profiling compile-release-linux \
	compile-windows-release compile-android-release import test style-check style-fix clang-tidy-database \
	check-staged-uids install-git-hooks docs docs-site docs-install docs-server docs-pdf \
	python-runtime-linux python-runtime-windows
.DEFAULT_GOAL = compile-debug

# libmaszyna - MaSzyna's vehicles, sceneries and drawing - is a plugin of Rails Reloaded built into libreloaded's library. libreloaded is the
# vendor/libreloaded submodule; for development against a local checkout of it instead (its
# uncommitted work included): make libreloaded-local LIBRELOADED_LOCAL=<path>; the path is kept in
# .libreloaded-local (not versioned) until make libreloaded-submodule.
LIBRELOADED_LOCAL_FILE:=.libreloaded-local
LIBRELOADED:=$(or $(shell cat $(LIBRELOADED_LOCAL_FILE) 2>/dev/null),$(CURDIR)/vendor/libreloaded)
LIBRELOADED_MAKE=$(MAKE) -C $(LIBRELOADED) GODOT_PROJECT_DIR=$(CURDIR)/demo "LIBRELOADED_PLUGINS=$(CURDIR)"
# The addons demo/ takes from the checkout in use; libreloaded's build links only those missing, so
# a switch between the submodule and a local checkout links them here
DEMO_ADDONS:=addons/libreloaded vendor/gut/addons/gut vendor/gnd-sfx/addons/gnd_sfx
# What the test project (demo/) takes of libreloaded's own demo: linked from the checkout in use
DEMO_LINKS:=vfx addons/debug_menu tests/simulation_runtime_hook.gd tests/simulation_runtime_hook.gd.uid \
	tests/reloaded_gut_test.gd tests/reloaded_gut_test.gd.uid
GODOT?=godot-double
# One test script, as res://tests/<name>.gd: make test TEST=<name>; every script without it
TEST?=

libreloaded-local:
	@test -n "$(LIBRELOADED_LOCAL)" || { echo "Which libreloaded checkout? make libreloaded-local LIBRELOADED_LOCAL=<path>" >&2; exit 1; }
	@test -d "$(LIBRELOADED_LOCAL)/addons/libreloaded" || { echo "$(LIBRELOADED_LOCAL) is not a libreloaded checkout" >&2; exit 1; }
	realpath "$(LIBRELOADED_LOCAL)" > $(LIBRELOADED_LOCAL_FILE)
	$(MAKE) links

libreloaded-submodule:
	rm -f $(LIBRELOADED_LOCAL_FILE)
	git submodule update --init vendor/libreloaded
	$(MAKE) links

links:
	@test -d "$(LIBRELOADED)/addons/libreloaded" || { echo "No libreloaded at $(LIBRELOADED): make libreloaded-submodule, or make libreloaded-local LIBRELOADED_LOCAL=<path>" >&2; exit 1; }
	mkdir -p demo/addons
	for link in $(DEMO_LINKS); do ln -sfn "$(LIBRELOADED)/demo/$$link" "demo/$$link" || exit 1; done
	for addon in $(DEMO_ADDONS); do ln -sfn "$(LIBRELOADED)/$$addon" "demo/addons/$$(basename $$addon)" || exit 1; done
	ln -sfn ../../addons/libmaszyna demo/addons/libmaszyna

compile-debug: links
	$(LIBRELOADED_MAKE) compile-debug

compile-release: links
	$(LIBRELOADED_MAKE) compile-release

# The release library of each platform, built by libreloaded's rules (the Linux one in its SDK
# container, for an old glibc) - what the CI builds to prove the plugin compiles everywhere
compile-release-linux compile-windows-release compile-android-release compile-profiling: links
	$(LIBRELOADED_MAKE) $@

import: links
	cd demo && timeout 45 $(GODOT) --headless --import

test: links
	cd demo && timeout 240 $(GODOT) --headless -s addons/gut/gut_cmdln.gd -gconfig= \
		-gpre_run_script=res://tests/simulation_runtime_hook.gd -gexit \
		$(if $(TEST),-gtest=res://tests/$(TEST).gd,-gdir=res://tests)

style-check:
	$(MAKE) -C $(LIBRELOADED) style-check "LIBRELOADED_PLUGINS=$(CURDIR)" $(if $(STYLE_FILE),STYLE_FILE=$(abspath $(STYLE_FILE)))

style-fix:
	$(MAKE) -C $(LIBRELOADED) style-fix "LIBRELOADED_PLUGINS=$(CURDIR)" $(if $(STYLE_FILE),STYLE_FILE=$(abspath $(STYLE_FILE)))

# The compilation database of libreloaded's clang-tidy build (this plugin's sources included),
# linked here as build-clang-tidy for clangd (.clangd) and the editors
clang-tidy-database:
	$(MAKE) -C $(LIBRELOADED) build-clang-tidy/compile_commands.json \
		build-clang-tidy/godot-cpp/gen/include/godot_cpp/classes/node.hpp "LIBRELOADED_PLUGINS=$(CURDIR)"
	ln -sfn "$(LIBRELOADED)/build-clang-tidy" build-clang-tidy

# The CPython runtimes the cab Python screens run on (PythonScreenServer), 2.7 and 3.14, each a
# directory the server finds beside the game's executable (or, as a fallback, in the game dir) -
# the game ships them: build-python-runtime/<linux|windows>/python2.7 and python3.14. Linux's are
# built in libreloaded's Linux SDK container, like the release; Windows' are python.org's builds
PYTHON_RUNTIME_BUILD_DIR:=$(CURDIR)/build-python-runtime
PYTHON_RUNTIMES:=2.7 3.14
PYTHON_RUNTIME_LINUX:=$(foreach version,$(PYTHON_RUNTIMES),$(PYTHON_RUNTIME_BUILD_DIR)/linux/python$(version))
PYTHON_RUNTIME_WINDOWS:=$(foreach version,$(PYTHON_RUNTIMES),$(PYTHON_RUNTIME_BUILD_DIR)/windows/python$(version))

python-runtime-linux: $(PYTHON_RUNTIME_LINUX)

python-runtime-windows: $(PYTHON_RUNTIME_WINDOWS)

# The runtime $(1) built into $(2); LINUX_HOST=1 builds it on the host, as a game's release library
# then (its make): no Docker, a runtime for this machine only
PYTHON_RUNTIME_LINUX_BUILD=$(CURDIR)/scripts/build-python-runtime $(1) $(2) $(PYTHON_RUNTIME_BUILD_DIR)/work

$(PYTHON_RUNTIME_BUILD_DIR)/linux/python%: scripts/build-python-runtime | links
	$(if $(LINUX_HOST),$(call PYTHON_RUNTIME_LINUX_BUILD,$*,$@),$(LIBRELOADED_MAKE) linux-sdk-run \
	    "LINUX_SDK_COMMAND=$(call PYTHON_RUNTIME_LINUX_BUILD,$*,$@)")

$(PYTHON_RUNTIME_BUILD_DIR)/windows/python%: scripts/fetch-python-runtime-windows
	scripts/fetch-python-runtime-windows $* $@ $(PYTHON_RUNTIME_BUILD_DIR)/work

# A staged script or shader goes in with its .uid (scripts/check-staged-uids)
check-staged-uids:
	@scripts/check-staged-uids

# The repository's hooks (scripts/git-hooks): pre-commit runs check-staged-uids
install-git-hooks:
	git config core.hooksPath scripts/git-hooks

# The documentation is libreloaded's tooling run with this repository as its plugin: libmaszyna's
# site - this docs/ and the class reference of libreloaded and libmaszyna - assembled into
# build-docs/ (libreloaded's make docs-site, docs-server, docs-pdf with DOCS_SOURCE=libmaszyna). make docs refreshes doc_classes/ of both from the library built into demo/, each class's
# XML going to the repository declaring it (libreloaded's scripts/update-doc-classes)
DOCS_MAKE=$(LIBRELOADED_MAKE) DOCS_SOURCE=libmaszyna DOCS_BUILD_DIR=$(CURDIR)/build-docs

docs: links
	$(LIBRELOADED_MAKE) docs GODOT=$(GODOT)

docs-site docs-install docs-server docs-pdf:
	$(DOCS_MAKE) $@
