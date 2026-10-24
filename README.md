# libmaszyna

MaSzyna's vehicles, sceneries and drawing for Rails Reloaded: a plugin of libreloaded, built into its
library. Licensed under the Mozilla Public License 2.0 (`LICENSE`), as MaSzyna is.

## About the project

libmaszyna is the port of the MaSzyna train simulator on **libreloaded**, the core of Rails Reloaded.
libreloaded holds what is common to any railway simulation - the servers, the cabin system, the
player, the generic vehicle layers, tracks and their rendering, the test harness; libmaszyna brings
what is MaSzyna's onto it:

- the vehicles simulated on MaSzyna's own physics, the vendored Mover, behind the core's vehicle
  interfaces,
- the original's formats: FIZ vehicle definitions, MMD cabs, E3D/T3D models, `.scn` sceneries and
  their scenarios, signalling and events,
- the cabs, the python screens, the materials and the drawing of MaSzyna's data.

It is made for MaSzyna Reloaded, which builds libreloaded with this plugin in it.

### Layout

| Path | What it is |
|------|------------|
| `src/` | C++, compiled into libreloaded's library (`plugin.cmake`); the vendored Mover in `src/maszyna/maszyna-mover` |
| `addons/libmaszyna/` | the Godot addon: GDScript, scenes, shaders, editor plugins |
| `doc_classes/` | the class reference of the C++ classes |
| `demo/` | the test project: a demo scene, the tests (`demo/tests/`) and their fixtures (`demo/tests/fixtures/`) |
| `docs/` | libmaszyna's guides, on the site libreloaded's tooling assembles (`make docs-site`) |
| `vendor/libreloaded` | libreloaded, as a submodule |
| `plugin.cmake` | what libreloaded's CMake builds of this repository (`LIBRELOADED_PLUGINS`) |

The C++ classes and singletons are registered by `MaszynaPlugin` (`src/maszyna/MaszynaPlugin.cpp`),
through libreloaded's plugin stages; the addon's autoloads and types by
`addons/libmaszyna/libmaszyna.gd`.

## Requirements

- Godot 4.7, **double precision** (`godot-double`; another name with `GODOT=<binary>`) - the library
  is built against double precision godot-cpp, and a single precision Godot does not load it
- what libreloaded's build needs: CMake and Python 3 (see libreloaded's README), a C++ compiler

## Getting started

Clone with the submodule:

```bash
git clone --recursive <url>
```

or, in a clone made without it:

```bash
make libreloaded-submodule
```

Then build, import the test project and run the tests:

```bash
make compile-debug        # or just make; make compile-release for a release build
make import
make test TEST=<name>     # one test script, res://tests/<name>.gd; without TEST, all of them
```

`make compile-debug` builds libreloaded's library with libmaszyna in it into `demo/`, and links
into `demo/` the pieces of libreloaded's own demo the test project uses (`make links`: `vfx`,
`addons/debug_menu`, `tests/simulation_runtime_hook.gd`, `tests/reloaded_gut_test.gd`).

### Developing against a local libreloaded

To build against a checkout of libreloaded instead of the submodule - with its uncommitted work:

```bash
make libreloaded-local LIBRELOADED_LOCAL=<path>
```

The path is kept in `.libreloaded-local` (not versioned) until `make libreloaded-submodule` goes
back to the submodule. A change libmaszyna needs in libreloaded is made in libreloaded's repository,
never in `vendor/libreloaded`; the submodule is moved to it once it is pushed.

## Simulation timing

The vehicles on the Mover are stepped on the rendered frame, not on Godot's fixed tick, as in the
original: libreloaded's `SimulationServer` advances its clock on `SceneTree`'s `process_frame`,
which is emitted before every node's `_process` (measured on Godot 4.7.2, `FINDINGS.md`), and
`VehicleServer` hands the frame to each vehicle implementation - here `MaszynaMoverVehicleServer`.
Drawing, the cabin, the HUD and the cameras therefore see the position of *this* frame.

`MaszynaMoverVehicleServer::stepping_advance()` integrates the whole frame, in sub-steps no longer
than `PHYSICS_STEP` (10 ms), because that is what the coupler springs were tuned for - a stiff
spring integrated with a much larger step kicks the trainset.

## Rendering transparent elements

E3D submodels flagged `material_transparent` (the original engine's own translucent-pass flag)
render with a hard alpha-scissor cutout by default (`MaszynaMaterialManager.Transparency.AlphaScissor`,
threshold 0.5, no real blending) - this matches how most such content actually looks (window
light masks, foliage) and avoids transparency sorting issues.

Some content needs real alpha blending instead - most commonly a self-contained cabin interior,
where the scissor cutout looks visibly wrong across the board (glass, instrument backlight glow).
Two opt-in overrides exist on `MaszynaE3DModelInstance` (`addons/libmaszyna/e3d/maszyna_e3d_model_instance.gd`),
both only affecting submodels already flagged `material_transparent` - opaque submodels are never
pulled into the alpha-blended pass:

- `force_alpha` (bool) - every `material_transparent` submodel in this model instance gets real
  alpha blending. Set by `MmdCabinInstancer.build_into()` for every cabin's `CabModel`.
- `force_alpha_submodel_paths` (`Array[NodePath]`) - a surgical alternative for forcing just one
  named submodel subtree (and its descendants) within an otherwise alpha-scissor model. Submodel
  names alone aren't unique across the tree, so entries are resolved via `MaszynaE3DModel.get_node_or_null`.
  `MmdCabinInstancer._resolve_force_alpha_submodel_paths()` populates this automatically for MMD
  `i-*:` indicator descriptors whose `MmdSemanticCatalog` entry has `force_alpha: true` set (e.g.
  `i-instrumentlight`'s `<base>_on`/`<base>_off` pair).

Known limitation: the original engine's own shader (`mat_default.frag`) combines an alpha-test
discard against a per-*material* `opacity` threshold with real per-pixel blending in the same
pass; this port only has the discard (alpha-scissor) and the blend (alpha) as two separate,
mutually exclusive modes, with no per-material threshold. Real alpha blending on content whose
alpha channel isn't clean binary data (e.g. padding colors baked outside the intended cutout, or a
channel repurposed for something else like a specular mask) can bleed through as an unwanted halo
or partial see-through - which is why it isn't applied to all `material_transparent` E3D content
by default, only where explicitly opted in above.

## Texture size and filtering

DDS textures larger than a limit drop their top mipmap levels when loaded, as the original's
`maxtexturesize`/`maxcabtexturesize` do (Globals.h:164-165):

- `maszyna/import/dds_max_texture_size` (default 1024) - scenery and vehicles,
- `maszyna/import/dds_max_cab_texture_size` (default 4096, the original's) - the cab, whose
  instruments need the full resolution (Train.cpp:660).

Anisotropic filtering (the original uses 8x, Texture.cpp:1191) is Godot's global
`rendering/textures/default_filters/anisotropic_filtering_level`. It acts only on samplers
declared `*_anisotropic`; most material shaders in `addons/libmaszyna/materials/types/`
use `filter_linear_mipmap`, so the setting does not reach them.

## Code quality

The code style is [CODE_STYLE.md](CODE_STYLE.md), the working agreement [AGENTS.md](AGENTS.md); how to
contribute is in [CONTRIBUTING.md](CONTRIBUTING.md).

Before opening a pull request, format the C++ files you touched and check them with clang-format
and clang-tidy:

```bash
make style-fix STYLE_FILE=<file>
make style-check STYLE_FILE=<file>
```

Without `STYLE_FILE` both take every C++ file. They run libreloaded's style scripts, with the same
`.clang-format` and `.clang-tidy` (the copies here are libreloaded's), and skip the vendored Mover
(`src/maszyna/maszyna-mover`).

## Documentation

The documentation is one site of libreloaded and libmaszyna, built with libreloaded's tooling and
this repository as its plugin: libreloaded's guides (the architecture among them), libmaszyna's
from `docs/` (wrapping the Mover, MaSzyna's Lua modules) and the class reference of both.

```bash
make docs-site      # assembles it into build-docs/site
make docs-server    # and serves it (Jekyll, or Ruby's image in Docker)
make docs-pdf       # the same as one PDF, build-docs/reloaded-docs.pdf
make docs           # refreshes doc_classes/ from the library built into demo/
```

`make docs` dumps the class reference of the whole library and keeps here only the XML of the
classes declared in `src/`; libreloaded's go back to libreloaded. The editor shows the same
reference under F1: the library carries the `doc_classes` of libreloaded and of every plugin built
into it.

If you have found any bug, have a suggestion or want to join us - feel free to open an
[issue](https://github.com/MaSzyna-Reloaded/libmaszyna/issues) or start a
[discussion](https://github.com/MaSzyna-Reloaded/libmaszyna/discussions)!
