#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `updatevalues`, `addvalues` and `copyvalues` events (Event.cpp:469-571,
    /// 876-961): writes the masked fields of its memories, from its own values or from a source
    /// memory. The `*` of the scenery - "leave this field as it is" - is a field left out of the
    /// mask.
    class MaszynaLegacyMemoryAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyMemoryAction, ScenarioEventAction)

        public:
            enum Mode {
                /// `updatevalues`, `copyvalues`
                MODE_SET,
                /// `addvalues`: the text is appended, the values added (MemCell.cpp:30-37)
                MODE_ADD,
                /// An isolated section's own memory when the section is taken: value 2 made odd
                /// (TIsolated::Modify(), Track.cpp:155-156)
                MODE_ISOLATED_BUSY,
                /// ...and when it is free again: the low byte of value 2 cleared (Track.cpp:128-130)
                MODE_ISOLATED_FREE,
            };

            /// The part of value 2 a freed isolated section clears
            static constexpr int ISOLATED_FREE_MASK = 0xFF;

        private:
            TypedArray<RID> memories;
            RID source;
            String text;
            double value1 = 0.0;
            double value2 = 0.0;
            BitField<ScenarioEventServer::MemoryField> mask = 0;
            Mode mode = MODE_SET;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_memories(const TypedArray<RID> &p_memories);
            TypedArray<RID> get_memories() const;
            /// `copyvalues`: the memory the values are read from when the event runs; empty for
            /// the action's own values
            void set_source(const RID &p_source);
            RID get_source() const;
            void set_text(const String &p_text);
            String get_text() const;
            void set_value1(double p_value1);
            double get_value1() const;
            void set_value2(double p_value2);
            double get_value2() const;
            void set_mask(BitField<ScenarioEventServer::MemoryField> p_mask);
            BitField<ScenarioEventServer::MemoryField> get_mask() const;
            void set_mode(Mode p_mode);
            Mode get_mode() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaLegacyMemoryAction::Mode)
