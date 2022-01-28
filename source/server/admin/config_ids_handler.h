#pragma once

#include <memory>

#include "envoy/http/codes.h"

#include "source/extensions/common/tap/config_id_map.h"
#include "source/server/admin/handler_ctx.h"

#include "absl/strings/string_view.h"

namespace Envoy {

namespace Buffer {
class Instance;
}

namespace Http {
class ResponseHeaderMap;
}

namespace Server {

class Instance;
class AdminStream;

/**
 * ConfigIdsHandler provides an implementation for the /tap/config_ids admin endpoint.
 * This endpoint should return a newline separated list of valid config_id strings
 * obtained from the singleton ConfigIdsMap
 */
class ConfigIdsHandler : public HandlerContextBase {
  std::shared_ptr<const Extensions::Common::Tap::ConfigIdMap> config_id_map_;

public:
  ConfigIdsHandler(Server::Instance& server);

  // Handler method called when /tap/config_ids is hit
  Http::Code handlerConfigIds(absl::string_view path_and_query,
                              Http::ResponseHeaderMap& response_headers, Buffer::Instance& response,
                              AdminStream&) const;
};

} // namespace Server
} // namespace Envoy