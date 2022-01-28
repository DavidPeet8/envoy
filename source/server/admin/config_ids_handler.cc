#include "source/server/admin/config_ids_handler.h"

#include "envoy/buffer/buffer.h"
#include "envoy/http/codes.h"
#include "envoy/http/header_map.h"

#include "source/common/http/headers.h"
#include "source/common/http/utility.h"
#include "source/common/network/utility.h"
#include "source/server/admin/utils.h"

#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Server {

using CfgMap = Extensions::Common::Tap::ConfigIdMap;

ConfigIdsHandler::ConfigIdsHandler(Server::Instance& server)
    : HandlerContextBase(server), config_id_map_(CfgMap::getSingleton(server.singletonManager())) {}

Http::Code ConfigIdsHandler::handlerConfigIds(absl::string_view,
                                              Http::ResponseHeaderMap& response_headers,
                                              Buffer::Instance& response, AdminStream&) const {
  response_headers.setReferenceContentType(Http::Headers::get().ContentTypeValues.Text);

  std::vector<std::string> keys = config_id_map_->keys();

  for (const auto& key : keys) {
    response.add(key);
    response.add(absl::string_view("\n"));
  }

  return Http::Code::OK;
}

} // namespace Server
} // namespace Envoy