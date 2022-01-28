#include "source/extensions/common/tap/config_id_map.h"

#include "envoy/admin/v3/tap.pb.h"
#include "envoy/singleton/manager.h"

#include "source/common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {

SINGLETON_MANAGER_REGISTRATION(admin_config_map);

using ConfigIdMapInternal = absl::node_hash_map<std::string, absl::node_hash_set<ExtensionConfig*>>;

ConfigIdMapSharedPtr ConfigIdMap::getSingleton(Singleton::Manager& singleton_manager) {
  return singleton_manager.getTyped<ConfigIdMap>(
      SINGLETON_MANAGER_REGISTERED_NAME(admin_config_map),
      [] { return std::make_shared<ConfigIdMap>(); });
}

void ConfigIdMap::registerExtensionConfig(ExtensionConfig& config, const std::string& config_id) {
  absl::WriterMutexLock lock(&mutex_);
  ASSERT(!config_id.empty());
  ASSERT(config_id_map_[config_id].count(&config) == 0);
  config_id_map_[config_id].insert(&config);
}

void ConfigIdMap::unregisterExtensionConfig(ExtensionConfig& config) {
  absl::WriterMutexLock lock(&mutex_);
  ASSERT(!config.adminId().empty());
  std::string admin_id(config.adminId());
  ASSERT(config_id_map_[admin_id].count(&config) == 1);

  config_id_map_[admin_id].erase(&config);
  if (config_id_map_[admin_id].empty()) {
    config_id_map_.erase(admin_id);
  }
}

void ConfigIdMap::registerTapConfig(const std::string& config_id,
                                    const envoy::config::tap::v3::TapConfig& tap_config,
                                    Sink* admin_sink) {
  absl::WriterMutexLock lock(&mutex_);
  for (ExtensionConfig* config : config_id_map_[config_id]) {
    config->newTapConfig(tap_config, admin_sink);
  }
}

void ConfigIdMap::unregisterTapConfig(const std::string& config_id) {
  absl::WriterMutexLock lock(&mutex_);
  for (auto config : config_id_map_[config_id]) {
    ENVOY_LOG(debug, "detach tap admin request for config_id={}", config_id);
    config->clearTapConfig();
  }
}

} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy