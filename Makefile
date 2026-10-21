.PHONY: libreloaded-local libreloaded-submodule links compile-debug compile-release import test style-check style-fix
.DEFAULT_GOAL = compile-debug

# libmaszyna - MaSzyna's vehicles, sceneries and drawing - is a plugin of Rails Reloaded built into libreloaded's library. libreloaded is the
# vendor/libreloaded submodule; for development against a local checkout of it instead (its
# uncommitted work included): make libreloaded-local LIBRELOADED_LOCAL=<path>; the path is kept in
# .libreloaded-local (not versioned) until make libreloaded-submodule.
LIBRELOADED_LOCAL_FILE:=.libreloaded-local
LIBRELOADED:=$(or $(shell cat $(LIBRELOADED_LOCAL_FILE) 2>/dev/null),$(CURDIR)/vendor/libreloaded)
LIBRELOADED_MAKE=$(MAKE) -C $(LIBRELOADED) GODOT_PROJECT_DIR=$(CURDIR)/demo "LIBRELOADED_PLUGINS=$(CURDIR)"
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
	for link in $(DEMO_LINKS); do ln -sfn "$(LIBRELOADED)/demo/$$link" "demo/$$link"; done

compile-debug: links
	$(LIBRELOADED_MAKE) compile-debug

compile-release: links
	$(LIBRELOADED_MAKE) compile-release

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
