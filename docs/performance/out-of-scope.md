---
layout: page
title: "Performance - out of scope"
---

What would save frame time in libmaszyna but changes a setting or the picture, so it is left out
of the work. The full list, with the game's own items, is in the game's
`docs/performance/out-of-scope.md`; these are the ones whose code is here.

| Item | Where |
|---|---|
| Directional shadow atlas 8192 also outside the cab | `maszyna_sky_environment.gd:72, 368-370` |
| Mirrors render to the main camera's far plane, with sun shafts | `PlanarMirror3D.cpp` |
| Merged street lamps become omni lights (6 shadow passes each) | `E3DLightFactory.cpp:162` |
| No mesh LODs, no vertex compression | `E3DMesh.cpp` |
| DDS textures without a full mip chain | `dds_texture_loader.gd:88-101` |
| Anisotropic filtering not used by most material shaders | `types/*.gdshader` (`filter_linear_mipmap`) |
