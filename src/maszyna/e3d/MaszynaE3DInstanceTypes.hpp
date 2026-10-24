#pragma once

namespace godot {
    /* How an E3D instance's submodels are drawn - the vocabulary MaszynaE3DRenderingServer's API and
     * the backends that build its instances share. The server inherits it, so the names are its own
     * (MaszynaE3DRenderingServer::TRANSLUCENCY_BLENDED) and a backend needs no knowledge of the
     * server. What an instance is - a static piece of the scenery (a "node ... model" of a .scn)
     * against a dynamic one (its "dynamic", a vehicle), the original's TAnimModel against a
     * TDynamicObject (particles.h:135) - is ModelRenderingServer::InstanceKind. */
    struct MaszynaE3DInstanceTypes {
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
