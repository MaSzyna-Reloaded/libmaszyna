# MaSzyna-API-wrapper

## About the project

This C++ GDExtension for Godot Engine offers a simplified interface to the MaSzyna train simulator API. Tailored for the MaSzyna Reloaded project, it enables developers to construct train components using Godot's Node-based system, providing a visual and intuitive approach to train creation and management. Key features include:

- Custom Node-based Classes: Create train elements using our premade and integrated classes based on Godot's familiar Node system.
- Parameter Customization: Fine-tune train parameters directly within the Godot editor.
- Simplified Integration: Seamlessly integrate the MaSzyna simulator physics into your Godot projects.
- Enhanced Debugging: Streamline the debugging process for your train simulations.

### Setup

1. Install Python 3
2. Install CMake 3.30 or newer. CMake will automatically install its dependencies.
3. Clone the repository and checkout submodules

```
git clone <url>
git submodule update --init --recursive
```

### Android development
#### Set up the build system   
For build system setup,
please take a look at [official Godot Engine documentation for Android development](https://docs.godotengine.org/en/4.3/tutorials/export/exporting_for_android.html)
### Compiling
> [!IMPORTANT]
> If you are using DEBUG macro, you should build CMake project with flag `-DLIBMASZYNA_DEBUG=ON` and then compile it.
> When using make ,add `LIBMASZYNA_DEBUG=ON` to a command line, i.e. `make compile-debug LIBMASZYNA_DEBUG=ON`. You can
> also turn off DEBUG using `LIBMASZYNA_DEBUG=OFF`.

```bash
	cmake -B build-<platform> \
          -DGODOTCPP_TARGET="template_release"
	cmake --build build-<platform>
```

Example:
```bash
	cmake -B build-linux64 \
          -DGODOTCPP_TARGET="template_release"
	cmake --build build-linux64
```

Cross-compiling (for Windows on Linux):
```bash
	cmake -B build-win64 \
          -DGODOTCPP_TARGET="template_release" \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	cmake --build build-win64
```

#### Makefile

You can use Makefile targets as shortcuts.

For debug compilation:

```
make compile-debug
```

or just simply

```
make
```

For release compilation:

```
make compile-release
```

For cross compilation linux/windows:

```
make cross-compile-debug
```

or 

```
make cross-compile-release
```

#### Parallel compilation

To enable parallel compilation add `--parallel <num_jobs>` argument to cmake calls, for example:

```bash
	cmake -B build-linux64 \
          -DGODOTCPP_TARGET="template_release"
          --parallel 4
	cmake --build build-linux64 --parallel 4
```

If you're using `Makefile`, number of parralel jobs will be set automatically to `<cores count> - 2` for system with at
least two cores.

### Compatibility

### Compiling
> [!IMPORTANT]
> Versions from 03.10.2026 onward will require Godot Engine's double precision build

| Plugin Version | Godot Engine version     | Windows | Linux | Mac OS | Android                  | iOS | C++ Standard | MaSzyna Version     |
|----------------|--------------------------|---------|-------|--------|--------------------------|-----|--------------|---------------------|
| 25.08.2024     | 4.3                      | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06               |
| 11.03.2025     | 4.4                      | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06               |
| 11.04.2025     | 4.4                      | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06               |
| 30.09.2025     | 4.5                      | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06               |
| 30.11.2025     | 4.5.x                    | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06, 25.11        |
| 27.01.2026     | 4.6                      | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06, 25.11, 26.01 |
| 06.07.2026     | 4.7.x                    | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06, 25.11, 26.01 |
| 03.10.2026     | 4.7.x (double precision) | ✅       | ✅     | ❌      | ✅ (Target API Level: 34) | ❌   | C++ 17       | 24.06, 25.11, 26.01 |
### Documentation 

Project documentation: https://maszyna-reloaded.github.io/MaSzyna-API-wrapper/

If you have found any bug, have a suggestion or want to join us - feel free to open an [issue](https://github.com/MaSzyna-Reloaded/MaSzyna-API-wrapper/issues) or start a [discussion](https://github.com/MaSzyna-Reloaded/MaSzyna-API-wrapper/discussions)!

### Simulation timing

Two rules decide when the vehicle simulation runs, and both matter enough to be stated here: the
scenario's events are driven by time, and multiplayer will be.

**The simulation is stepped before anything that reads it.** `SimulationServer` advances its
clock on `SceneTree`'s `process_frame`, which is emitted *before* every node's `_process`
(measured on Godot 4.7.2, `FINDINGS.md`), and `VehicleServer::stepping_advance()` hands the frame
to each vehicle implementation (`MaszynaMoverVehicleServer`). Drawing, the cabin, the HUD and the
cameras therefore see the position of *this* frame.

**No simulation time is ever dropped, and no sub-step is ever oversized.** A frame hands its whole
delta to `step_frame()`, which integrates as much of it as it honestly can and *owes* the rest to
the frames that follow:

* the sub-step stays at or below `PHYSICS_STEP` (10 ms), because that is what the coupler springs
  were tuned for - a stiff spring integrated with a much larger step kicks the trainset;
* one frame can therefore take at most `MAX_PHYSICS_ITERATIONS * PHYSICS_STEP` (0.2 s);
* every frame still integrates at least its own delta, so nothing is quantised and the motion is
  as smooth as the frame rate.

A stall of, say, half a second is not taken in one go: 0.2 s is integrated now and the remaining
0.3 s over the next frames. The clock and the simulation stay together, and nothing jumps.

**Past `maszyna/physics/catch_up_limit` (1 s by default) the debt is taken in one step instead.**
At that point the machine is not stalling, it is too slow to simulate in real time, and spreading
the debt would only add work to frames that are already late. The step is then larger than the
couplers can stand and the trainset visibly jumps - deliberately, because a jump that can be seen
beats a clock that silently lies to the scenario. It is written to `GameLog`, so it is not
mistaken for a physics bug.

### Rendering transparent elements

E3D submodels flagged `material_transparent` (the original engine's own translucent-pass flag)
render with a hard alpha-scissor cutout by default (`MaterialManager.Transparency.AlphaScissor`,
threshold 0.5, no real blending) - this matches how most such content actually looks (window
light masks, foliage) and avoids transparency sorting issues.

Some content needs real alpha blending instead - most commonly a self-contained cabin interior,
where the scissor cutout looks visibly wrong across the board (glass, instrument backlight glow).
Two opt-in overrides exist on `E3DModelInstance` (`addons/libmaszyna/legacy/e3d/e3d_model_instance.gd`),
both only affecting submodels already flagged `material_transparent` - opaque submodels are never
pulled into the alpha-blended pass:

- `force_alpha` (bool) - every `material_transparent` submodel in this model instance gets real
  alpha blending. Set by `MmdCabinInstancer.build_into()` for every cabin's `CabModel`.
- `force_alpha_submodel_paths` (`Array[NodePath]`) - a surgical alternative for forcing just one
  named submodel subtree (and its descendants) within an otherwise alpha-scissor model. Submodel
  names alone aren't unique across the tree, so entries are resolved via `E3DModel.get_node_or_null`.
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

### Texture size and filtering

DDS textures larger than a limit drop their top mipmap levels when loaded, as the original's
`maxtexturesize`/`maxcabtexturesize` do (Globals.h:164-165):

- `maszyna/import/dds_max_texture_size` (default 1024) - scenery and vehicles,
- `maszyna/import/dds_max_cab_texture_size` (default 4096, the original's) - the cab, whose
  instruments need the full resolution (Train.cpp:660).

Anisotropic filtering (the original uses 8x, Texture.cpp:1191) is Godot's global
`rendering/textures/default_filters/anisotropic_filtering_level`. It acts only on samplers
declared `*_anisotropic`; most material shaders in `addons/libmaszyna/legacy/materials/types/`
use `filter_linear_mipmap`, so the setting does not reach them.

### Code Quality

#### Formatting and style

Before opening a pull request, check formatting and clang-tidy issues:

```bash
make style-check
```

To apply automatic formatting and clang-tidy fixes:

```bash
make style-fix
```

Both targets use the repository `.clang-format` and `.clang-tidy` configuration. They skip legacy/generated code under `src/legacy/maszyna-mover` and `src/gen`.

To check or fix a single file, use `./scripts/style-check <path>` and `./scripts/style-fix <path>`.

#### CI
Clang-tidy checks are performed on CI. Those will fail automatically and publish results if any warning/error is found

CI uses no Docker Hub image. The double precision Godot comes from the release
`godot-<version>-double` (`ci/fetch-godot.sh`), and the Linux release library is built in the
Linux SDK image `ghcr.io/maszyna-reloaded/linux-sdk`, which CI pulls. The image is tagged by the
hash of `ci/docker/linux-sdk/Dockerfile`, so a changed Dockerfile is a new tag that is not on
ghcr.io yet - CI then builds the image on every run. After changing the Dockerfile, build and
publish the image yourself (a `gh` token with the `write:packages` scope:
`gh auth refresh -h github.com -s write:packages`):

```bash
gh auth token | docker login ghcr.io -u <github user> --password-stdin
make linux-sdk-image-push
```

### Testing

#### Testing locally

First, ensure you have checked out submodules:

```bash
git submodule update --init
```

Then run tests from the command line (if you have `make` installed, you can use shortcut `make run-tests`):

```bash
godot --path demo --headless -s addons/gut/gut_cmdln.gd -gdir=res://tests/ -gexit
```


Or use Godot Editor, but before that ensure you have configured test dirs.
GUT is storing the config in `user://` filesystem, so you have to do this manually.

![Godot GUT Configuration](docs/assets/gut-gui-tests-config.png)

Then run all tests:

![Godot GUT Running Tests](docs/assets/gut-gui-tests-running.png)


#### Testing using docker

From the command line, build the image first:

```bash
docker build-t godot-tests .
```

then run the image:

```bash
docker run --rm godot-tests
```

If you have `make`, you can simply use the shortcut `make docker-run-tests`.
