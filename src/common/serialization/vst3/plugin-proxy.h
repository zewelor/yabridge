// yabridge: a Wine plugin bridge
// Copyright (C) 2020-2022 Robbert van der Helm
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../bitsery/ext/in-place-variant.h"

#include "../common.h"
#include "plugin/audio-presentation-latency.h"
#include "plugin/audio-processor.h"
#include "plugin/automation-state.h"
#include "plugin/component.h"
#include "plugin/connection-point.h"
#include "plugin/edit-controller-2.h"
#include "plugin/edit-controller-host-editing.h"
#include "plugin/edit-controller.h"
#include "plugin/info-listener.h"
#include "plugin/keyswitch-controller.h"
#include "plugin/midi-learn.h"
#include "plugin/midi-mapping.h"
#include "plugin/note-expression-controller.h"
#include "plugin/note-expression-physical-ui-mapping.h"
#include "plugin/parameter-function-name.h"
#include "plugin/plugin-base.h"
#include "plugin/prefetchable-support.h"
#include "plugin/process-context-requirements.h"
#include "plugin/program-list-data.h"
#include "plugin/unit-data.h"
#include "plugin/unit-info.h"
#include "plugin/xml-representation-controller.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

/**
 * An abstract class that optionally implements all VST3 interfaces a plugin
 * object could implement. A more in depth explanation can be found in
 * `docs/architecture.md`, but the way this works is that we begin with an
 * `FUnknown` pointer from the Windows VST3 plugin obtained by a call to
 * `IPluginFactory::createInstance()` (with an interface decided by the host).
 * We then go through all the plugin interfaces and check whether that object
 * supports them one by one. For each supported interface we remember that the
 * plugin supports it, and we'll optionally write down some static data (such as
 * the edit controller cid) that can't change over the lifetime of the
 * application. On the plugin side we then return a `Vst3PluginProxyImpl` object
 * that contains all of this information about interfaces the object we're
 * proxying might support. This way we can allow casts to all of those object
 * types in `queryInterface()`, essentially perfectly mimicing the original
 * object.
 *
 * This monolith approach is also important when it comes to `IConnectionPoint`.
 * The host should be able to connect arbitrary objects together, and the plugin
 * can then use the query interface smart pointer casting system to cast those
 * objects to the types they want. By having a huge monolithic class that
 * implements any interface such an object might also implement, we can allow
 * perfect proxying behaviour for connecting components.
 */
class Vst3PluginProxy : public YaAudioPresentationLatency,
                        public YaAudioProcessor,
                        public YaAutomationState,
                        public YaComponent,
                        public YaConnectionPoint,
                        public YaEditController,
                        public YaEditController2,
                        public YaEditControllerHostEditing,
                        public YaInfoListener,
                        public YaKeyswitchController,
                        public YaMidiLearn,
                        public YaMidiMapping,
                        public YaNoteExpressionController,
                        public YaNoteExpressionPhysicalUIMapping,
                        public YaParameterFunctionName,
                        public YaPluginBase,
                        public YaPrefetchableSupport,
                        public YaProcessContextRequirements,
                        public YaProgramListData,
                        public YaUnitData,
                        public YaUnitInfo,
                        public YaXmlRepresentationController {
   public:
    /**
     * These are the arguments for constructing a `Vst3PluginProxyImpl`.
     */
    struct ConstructArgs {
        ConstructArgs() noexcept;

        /**
         * Read from an existing object. We will try to mimic this object, so
         * we'll support any interfaces this object also supports.
         */
        ConstructArgs(Steinberg::IPtr<FUnknown> object,
                      size_t instance_id) noexcept;

        /**
         * The unique identifier for this specific object instance.
         */
        native_size_t instance_id;

        YaAudioPresentationLatency::ConstructArgs
            audio_presentation_latency_args;
        YaAudioProcessor::ConstructArgs audio_processor_args;
        YaAutomationState::ConstructArgs automation_state_args;
        YaComponent::ConstructArgs component_args;
        YaConnectionPoint::ConstructArgs connection_point_args;
        YaEditController::ConstructArgs edit_controller_args;
        YaEditController2::ConstructArgs edit_controller_2_args;
        YaEditControllerHostEditing::ConstructArgs
            edit_controller_host_editing_args;
        YaInfoListener::ConstructArgs info_listener_args;
        YaKeyswitchController::ConstructArgs keyswitch_controller_args;
        YaMidiLearn::ConstructArgs midi_learn_args;
        YaMidiMapping::ConstructArgs midi_mapping_args;
        YaNoteExpressionController::ConstructArgs
            note_expression_controller_args;
        YaNoteExpressionPhysicalUIMapping::ConstructArgs
            note_expression_physical_ui_mapping_args;
        YaParameterFunctionName::ConstructArgs parameter_function_name_args;
        YaPluginBase::ConstructArgs plugin_base_args;
        YaPrefetchableSupport::ConstructArgs prefetchable_support_args;
        YaProcessContextRequirements::ConstructArgs
            process_context_requirements_args;
        YaProgramListData::ConstructArgs program_list_data_args;
        YaUnitData::ConstructArgs unit_data_args;
        YaUnitInfo::ConstructArgs unit_info_args;
        YaXmlRepresentationController::ConstructArgs
            xml_representation_controller_args;

        template <typename S>
        void serialize(S& s) {
            s.value8b(instance_id);
            s.object(audio_presentation_latency_args);
            s.object(audio_processor_args);
            s.object(automation_state_args);
            s.object(component_args);
            s.object(connection_point_args);
            s.object(edit_controller_args);
            s.object(edit_controller_2_args);
            s.object(edit_controller_host_editing_args);
            s.object(info_listener_args);
            s.object(keyswitch_controller_args);
            s.object(midi_learn_args);
            s.object(midi_mapping_args);
            s.object(note_expression_controller_args);
            s.object(note_expression_physical_ui_mapping_args);
            s.object(parameter_function_name_args);
            s.object(plugin_base_args);
            s.object(prefetchable_support_args);
            s.object(process_context_requirements_args);
            s.object(program_list_data_args);
            s.object(unit_data_args);
            s.object(unit_info_args);
            s.object(xml_representation_controller_args);
        }
    };

    /**
     * Message to request the Wine plugin host to instantiate a new IComponent
     * to pass through a call to `IComponent::createInstance(cid,
     * <requested_interface>::iid, ...)`.
     */
    struct Construct {
        using Response = std::variant<ConstructArgs, UniversalTResult>;

        NativeUID cid;

        /**
         * The interface the host was trying to instantiate an object for.
         * Technically the host can create any kind of object, but these are the
         * objects that are actually used.
         */
        enum class Interface {
            IComponent,
            IEditController,
        };

        Interface requested_interface;

        template <typename S>
        void serialize(S& s) {
            s.object(cid);
            s.value4b(requested_interface);
        }
    };

    /**
     * Instantiate this object instance with arguments read from another
     * interface implementation.
     */
    Vst3PluginProxy(ConstructArgs&& args) noexcept;

    /**
     * Message to request the Wine plugin host to destroy this object instance
     * with the given instance ID. Sent from the destructor of
     * `Vst3PluginProxyImpl`. This will cause all smart pointers to the actual
     * object in the Wine plugin host to be dropped.
     */
    struct Destruct {
        using Response = Ack;

        native_size_t instance_id;

        template <typename S>
        void serialize(S& s) {
            s.value8b(instance_id);
        }
    };

    /**
     * @remark The plugin side implementation should send a control message to
     *   clean up the instance on the Wine side in its destructor.
     */
    virtual ~Vst3PluginProxy() noexcept = 0;

    DECLARE_FUNKNOWN_METHODS

    /**
     * Get this object's instance ID. Used in `IConnectionPoint` to identify and
     * connect specific objects.
     */
    inline size_t instance_id() const noexcept {
        return arguments_.instance_id;
    }

    // These have to be defined here instead of in `YaPluginBase` because we
    // need to reference the `ConstructArgs`

    /**
     * The response code and updated supported interface list after a call to
     * `IPluginBase::initialize()`.
     *
     * HACK: This is needed to support Waves VST3 plugins because they only
     *       expose the edit controller interface after this point
     */
    struct InitializeResponse {
        UniversalTResult result;

        // This is a very ugly hack, but we'll just have to requery all
        // supported interfaces and replace the original constructargs in the
        // plugin-side proxy object
        Vst3PluginProxy::ConstructArgs updated_plugin_interfaces;

        template <typename S>
        void serialize(S& s) {
            s.object(result);
            s.object(updated_plugin_interfaces);
        }
    };

    /**
     * Message to pass through a call to `IPluginBase::initialize()` to the Wine
     * plugin host. We will read what interfaces the passed context object
     * implements so we can then create a proxy object on the Wine side that the
     * plugin can use to make callbacks with. The lifetime of this
     * `Vst3HostContextProxy` object should be bound to the `IComponent` we are
     * proxying.
     */
    struct Initialize {
        using Response = InitializeResponse;

        native_size_t instance_id;

        Vst3HostContextProxy::ConstructArgs host_context_args;

        template <typename S>
        void serialize(S& s) {
            s.value8b(instance_id);
            s.object(host_context_args);
        }
    };

    // We'll define messages for functions that have identical definitions in
    // multiple interfaces below. When the Wine plugin host process handles
    // these it should check which of the interfaces is supported on the host.

    /**
     * Message to pass through a call to
     * `{IComponent,IEditController}::setState(state)` to the Wine plugin host.
     */
    struct SetState {
        using Response = UniversalTResult;

        native_size_t instance_id;

        YaBStream state;

        template <typename S>
        void serialize(S& s) {
            s.value8b(instance_id);
            s.object(state);
        }
    };

    /**
     * The response code and written state for a call to
     * `{IComponent,IEditController}::getState(&state)`.
     */
    struct GetStateResponse {
        UniversalTResult result;
        YaBStream state;

        template <typename S>
        void serialize(S& s) {
            s.object(result);
            s.object(state);
        }
    };

    /**
     * Message to pass through a call to
     * `{IComponent,IEditController}::getState(&state)` to the Wine plugin host.
     */
    struct GetState {
        using Response = GetStateResponse;

        native_size_t instance_id;

        YaBStream state;

        template <typename S>
        void serialize(S& s) {
            s.value8b(instance_id);
            s.object(state);
        }
    };

   protected:
    /**
     * Update the supported status for all interfaces. This is needed because
     * Waves changes its query interface after `IPluginBase::initialize()` has
     * been called.
     */
    void update_supported_interfaces(ConstructArgs&& updated_interfaces);

   private:
    ConstructArgs arguments_;
};

#pragma GCC diagnostic pop

template <typename S>
void serialize(
    S& s,
    std::variant<Vst3PluginProxy::ConstructArgs, UniversalTResult>& result) {
    s.ext(result, bitsery::ext::InPlaceVariant{});
}
