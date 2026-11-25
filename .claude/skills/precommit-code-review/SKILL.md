---
name: precommit-code-review
description: Review a plan before it is shown to the operator and the staged diff before every commit, for the defects that already cost the project a regression - first of all every instance uniform it adds (an `instance uniform` in a shader, `set_instance_shader_parameter()`, `instance_geometry_set_shader_parameter()`, `ModelRenderingServer.instance_set_shader_parameter()`). Use after writing every plan and before every commit, and whenever a shader, a material type, a material factory or a model backend changes, because Godot reserves a block of its global shader buffer for every mesh drawn with a shader that declares an instance uniform, and a shader the scenery draws runs the buffer out.
---

## Rule zero: only what the operator orders

REQUIRED, ABSOLUTE: **do always and only what the operator orders.** Nothing is edited, reverted,
"restored", renamed, redesigned or posted (an issue, a comment, a PR) without the operator's
explicit order - not even one's own work after a complaint. A complaint is not an order: name the
facts and wait. Never touch an existing, reviewed mechanism outside the task.

REQUIRED, ABSOLUTE: **no separation-of-concerns breach, ever.** Before any new state, API, enum,
command or read from another layer, name to the operator which layer owns the value, where it is
read from and why that keeps every layer to its own concern - and write nothing until the operator
says yes. Data a scenery or a starter hands in is never read as live state; a value is read from
its owner, never from a plugin's implementation, a dump or a copy.

# Pre-commit code review

## 1. Instance uniforms

An `instance uniform` is not free. Godot reserves a block of its global shader buffer - 16 slots
of 16 bytes - for every geometry instance whose material declares one, whether anything sets it
or not. The buffer (`rendering/limits/global_shader_variables/buffer_size`, 65536 slots) holds
about 4000 blocks. Past them every new mesh logs "Too many instances using shader instance
variables", each later update of it logs
`Condition "global_shader_uniforms.instance_buffer_pos.has(p_instance)" is true` again, and the
mesh reads no value at all - the effect the uniform was for is gone on it.

#442: `instance uniform float reloaded_depth_behind` in every material type (#422) took a block
for every scenery submodel and `wrzosy_rpj69110.scn` ran out while its vehicles were built.

### When

* **After every plan**, before it is shown to the operator: the plan names each instance uniform it
  adds or uses, and the result of this review.
* **Before every commit**: the staged diff, `git diff --staged`.

### How

1. Search the plan or the diff for `instance uniform`, `set_instance_shader_parameter(`,
   `instance_geometry_set_shader_parameter(` and `instance_set_shader_parameter(`.
2. For each hit, name the shaders that declare the uniform - an `#include` counts: a
   `.gdshaderinc` declares it in every shader that includes it.
3. For each of those shaders, name the meshes drawn with it and how many a full scenery has.
4. It fails when the shader is one the scenery draws in numbers - a material type
   (`addons/libmaszyna/materials/types/*`), the terrain, a track, a road, anything a scenery is
   built of. It passes only when the meshes are few by construction and counted - a free spotlight's
   lamp (`free_spotlight*.gdshader`).
5. A failed review is reported to the operator with what was found, and the plan or the change
   is redone: a value some meshes of a material need is an option of the material
   (`MaszynaMaterialManager.MaterialOptions`), set as a plain `uniform` on a material of their own;
   the rest share theirs (`CODE_STYLE.md`, "An instance uniform costs every mesh drawn with its
   shader").

### What is no fix

* **A larger buffer.** Godot searches it from its start for every block it reserves, and every
  mesh still costs its 256 bytes.
* **Setting the uniform only on the meshes that need it.** The block is reserved for the
  declaration, not for the value.
