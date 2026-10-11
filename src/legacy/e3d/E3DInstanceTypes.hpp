#pragma once

namespace godot {
    /* What an E3D instance is and how its submodels are drawn - the vocabulary
     * E3DRenderingServer's API and the backends that build its instances share. The server
     * inherits it, so the names are its own (E3DRenderingServer::INSTANCE_KIND_DYNAMIC) and a
     * backend needs no knowledge of the server. */
    struct E3DInstanceTypes {
            /// What a placement is: a static piece of the scenery (a "node ... model" of a .scn)
            /// against a dynamic one (its "dynamic", which is a vehicle). The original keeps the same distinction
            /// wherever it matters - a TAnimModel against a TDynamicObject, remembered by its
            /// particle emitters as owner_type { none, vehicle, node } (particles.h:135).
            /// Nothing about this is particular to smoke; smoke is only its first reader.
            enum InstanceKind {
                INSTANCE_KIND_STATIC,
                INSTANCE_KIND_DYNAMIC,
            };

            /// How a submodel is drawn, as the backends tell the material resolver. The original
            /// draws a submodel in one pass, the opaque or the alpha one, by its flags
            /// (opengl33renderer.cpp:3422, 4313).
            enum Translucency {
                /// As its material says: opaque, or cut out at the alpha threshold
                TRANSLUCENCY_CUTOUT,
                /// Alpha-blended - a forced submodel drawn as nodes (near the camera)
                TRANSLUCENCY_BLENDED,
                /// Opaque whatever its texture's alpha - a forced submodel of the optimized
                /// instancer, which never draws in the alpha pass
                TRANSLUCENCY_OPAQUE,
            };
    };
} // namespace godot
