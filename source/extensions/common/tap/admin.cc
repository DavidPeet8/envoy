#include "source/extensions/common/tap/admin.h"

#include "envoy/admin/v3/tap.pb.h"
#include "envoy/admin/v3/tap.pb.validate.h"
#include "envoy/config/tap/v3/common.pb.h"
#include "envoy/data/tap/v3/wrapper.pb.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/protobuf/message_validator_impl.h"
#include "source/common/protobuf/utility.h"
#include "source/extensions/common/tap/config_id_map.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {

// Singleton registration via macro defined in envoy/singleton/manager.h
SINGLETON_MANAGER_REGISTRATION(tap_admin_handler);

AdminHandlerSharedPtr AdminHandler::getSingleton(Server::Admin& admin,
                                                 Singleton::Manager& singleton_manager,
                                                 Event::Dispatcher& main_thread_dispatcher) {
  return singleton_manager.getTyped<AdminHandler>(
      SINGLETON_MANAGER_REGISTERED_NAME(tap_admin_handler),
      [&admin, &singleton_manager, &main_thread_dispatcher] {
        AdminHandlerSharedPtr admin_handler = std::make_shared<AdminHandler>(
            admin, main_thread_dispatcher, ConfigIdMap::getSingleton(singleton_manager));
        return admin_handler;
      });
}

AdminHandler::AdminHandler(Server::Admin& admin, Event::Dispatcher& main_thread_dispatcher,
                           ConfigIdMapSharedPtr config_id_map)
    : admin_(admin), main_thread_dispatcher_(main_thread_dispatcher),
      config_id_map_(config_id_map) {
  const bool rc =
      admin_.addHandler("/tap", "tap filter control", MAKE_ADMIN_HANDLER(handler), true, true);
  RELEASE_ASSERT(rc, "/tap admin endpoint is taken");
  if (admin_.socket().addressType() == Network::Address::Type::Pipe) {
    ENVOY_LOG(warn, "Admin tapping (via /tap) is unreliable when the admin endpoint is a pipe and "
                    "the connection is HTTP/1. Either use an IP address or connect using HTTP/2.");
  }
}

AdminHandler::~AdminHandler() {
  const bool rc = admin_.removeHandler("/tap");
  ASSERT(rc);
}

Http::Code AdminHandler::handler(absl::string_view, Http::HeaderMap&, Buffer::Instance& response,
                                 Server::AdminStream& admin_stream) {
  if (attached_request_.has_value()) {
    // TODO(mattlklein123): Consider supporting concurrent admin /tap streams. Right now we support
    // a single stream as a simplification.
    return badRequest(response, "An attached /tap admin stream already exists. Detach it.");
  }

  if (admin_stream.getRequestBody() == nullptr) {
    return badRequest(response, "/tap requires a JSON/YAML body");
  }

  envoy::admin::v3::TapRequest tap_request;
  try {
    MessageUtil::loadFromYamlAndValidate(admin_stream.getRequestBody()->toString(), tap_request,
                                         ProtobufMessage::getStrictValidationVisitor());
  } catch (EnvoyException& e) {
    return badRequest(response, e.what());
  }

  ENVOY_LOG(debug, "tap admin request for config_id={}", tap_request.config_id());
  if (!config_id_map_->hasConfigId(tap_request.config_id())) {
    return badRequest(
        response, fmt::format("Unknown config id '{}'. No extension has registered with this id.",
                              tap_request.config_id()));
  }
  config_id_map_->registerTapConfig(tap_request.config_id(), tap_request.tap_config(), this);

  admin_stream.setEndStreamOnComplete(false);
  admin_stream.addOnDestroyCallback([this] {
    config_id_map_->unregisterTapConfig(attached_request_->id());
    attached_request_.reset(); // remove ref to attached_request_
  });
  attached_request_.emplace(tap_request.config_id(), tap_request.tap_config(), &admin_stream);
  return Http::Code::OK;
}

Http::Code AdminHandler::badRequest(Buffer::Instance& response, absl::string_view error) {
  ENVOY_LOG(debug, "handler bad request: {}", error);
  response.add(error);
  return Http::Code::BadRequest;
}

void AdminHandler::registerConfig(ExtensionConfig& config, const std::string& config_id) {
  config_id_map_->registerExtensionConfig(config, config_id);

  // Attached request only exists if an active tap is running
  if (attached_request_ && attached_request_->id() == config_id) {
    // Just register TapConfig with the new extension config
    config.newTapConfig(attached_request_->config(), this);
  }
}

void AdminHandler::unregisterConfig(ExtensionConfig& config) {
  config_id_map_->unregisterExtensionConfig(config);
}

void AdminHandler::AdminPerTapSinkHandle::submitTrace(
    TraceWrapperPtr&& trace, envoy::config::tap::v3::OutputSink::Format format) {
  ENVOY_LOG(debug, "admin submitting buffered trace to main thread");
  // Convert to a shared_ptr, so we can send it to the main thread.
  std::shared_ptr<envoy::data::tap::v3::TraceWrapper> shared_trace{std::move(trace)};
  // The handle can be destroyed before the cross thread post is complete. Thus, we capture a
  // reference to our parent.
  parent_.main_thread_dispatcher_.post([&parent = parent_, trace = shared_trace, format]() {
    if (!parent.attached_request_.has_value()) {
      return;
    }

    std::string output_string;
    switch (format) {
      PANIC_ON_PROTO_ENUM_SENTINEL_VALUES;
    case envoy::config::tap::v3::OutputSink::JSON_BODY_AS_STRING:
    case envoy::config::tap::v3::OutputSink::JSON_BODY_AS_BYTES:
      output_string = MessageUtil::getJsonStringFromMessageOrError(*trace, true, true);
      break;
    case envoy::config::tap::v3::OutputSink::PROTO_BINARY:
      PANIC("not implemented");
    case envoy::config::tap::v3::OutputSink::PROTO_BINARY_LENGTH_DELIMITED:
      PANIC("not implemented");
    case envoy::config::tap::v3::OutputSink::PROTO_TEXT:
      PANIC("not implemented");
    }

    ENVOY_LOG(debug, "admin writing buffered trace to response");
    Buffer::OwnedImpl output_buffer{output_string};
    parent.attached_request_.value().admin_stream_->getDecoderFilterCallbacks().encodeData(
        output_buffer, false);
  });
}

} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy
