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

#include "plugin-factory-proxy.h"

#include <pluginterfaces/vst/ivstcomponent.h>

#include "../vst3.h"
#include "plugin-proxy.h"

Vst3PluginFactoryProxyImpl::Vst3PluginFactoryProxyImpl(
    Vst3PluginBridge& bridge,
    Vst3PluginFactoryProxy::ConstructArgs&& args) noexcept
    : Vst3PluginFactoryProxy(std::move(args)), bridge_(bridge) {}

tresult PLUGIN_API
Vst3PluginFactoryProxyImpl::queryInterface(const Steinberg::TUID _iid,
                                           void** obj) {
    const tresult result = Vst3PluginFactoryProxy::queryInterface(_iid, obj);
    bridge_.logger_.log_query_interface("In IPluginFactory::queryInterface()",
                                        result,
                                        Steinberg::FUID::fromTUID(_iid));

    return result;
}

tresult PLUGIN_API
Vst3PluginFactoryProxyImpl::createInstance(Steinberg::FIDString cid,
                                           Steinberg::FIDString _iid,
                                           void** obj) {
    // Class IDs may be padded with null bytes
    constexpr size_t uid_size = sizeof(Steinberg::TUID);
    if (!cid || !_iid || !obj || strnlen(_iid, uid_size) < uid_size) {
        return Steinberg::kInvalidArgument;
    }

    Steinberg::TUID cid_array;
    std::copy(cid, cid + std::extent_v<Steinberg::TUID>, cid_array);

    // I don't think they include a safe way to convert a `FIDString/char*` into
    // a `FUID`, so this will have to do
    const Steinberg::FUID requested_iid = Steinberg::FUID::fromTUID(
        *reinterpret_cast<const Steinberg::TUID*>(&*_iid));

    Vst3PluginProxy::Construct::Interface requested_interface;
    if (requested_iid == Steinberg::Vst::IComponent::iid) {
        requested_interface = Vst3PluginProxy::Construct::Interface::IComponent;
    } else if (requested_iid == Steinberg::Vst::IEditController::iid) {
        requested_interface =
            Vst3PluginProxy::Construct::Interface::IEditController;
    } else {
        // When the host requests an interface we do not (yet) implement, we'll
        // print a recognizable log message
        bridge_.logger_.log_query_interface(
            "In IPluginFactory::createInstance()", Steinberg::kNotImplemented,
            requested_iid);

        *obj = nullptr;
        return Steinberg::kNotImplemented;
    }

    std::variant<Vst3PluginProxy::ConstructArgs, UniversalTResult> result =
        bridge_.send_mutually_recursive_message(Vst3PluginProxy::Construct{
            .cid = cid_array, .requested_interface = requested_interface});

    return std::visit(
        overload{
            [&](Vst3PluginProxy::ConstructArgs&& args) -> tresult {
                // These pointers are scary. The idea here is that we return a
                // newly initialized object (that initializes itself with a
                // reference count of 1), and then the receiving side will use
                // `Steinberg::owned()` to adopt it to an `IPtr<T>`.
                Vst3PluginProxyImpl* proxy_object =
                    new Vst3PluginProxyImpl(bridge_, std::move(args));

                // We return a properly downcasted version of the proxy object
                // we just created
                switch (requested_interface) {
                    case Vst3PluginProxy::Construct::Interface::IComponent:
                        *obj = static_cast<Steinberg::Vst::IComponent*>(
                            proxy_object);
                        break;
                    case Vst3PluginProxy::Construct::Interface::IEditController:
                        *obj = static_cast<Steinberg::Vst::IEditController*>(
                            proxy_object);
                        break;
                }

                return Steinberg::kResultOk;
            },
            [&](const UniversalTResult& code) -> tresult { return code; }},
        std::move(result));
}

tresult PLUGIN_API
Vst3PluginFactoryProxyImpl::setHostContext(Steinberg::FUnknown* context) {
    if (context) {
        // We will create a proxy object that that supports all the same
        // interfaces as `context`, and then we'll store `context` in this
        // object. We can then use it to handle callbacks made by the Windows
        // VST3 plugin to this context.
        host_context_ = context;

        // Automatically converted smart pointers for when the plugin performs a
        // callback later
        host_application_ = host_context_;
        plug_interface_support_ = host_context_;

        return bridge_.send_message(YaPluginFactory3::SetHostContext{
            .host_context_args = Vst3HostContextProxy::ConstructArgs(
                host_context_, std::nullopt)});
    } else {
        bridge_.logger_.log(
            "WARNING: Null pointer passed to "
            "'IPluginFactory3::setHostContext()'");
        return Steinberg::kInvalidArgument;
    }
}
