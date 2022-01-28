#pragma once

#include <memory>
#include <string>

#include "envoy/admin/v3/tap.pb.h"
#include "envoy/singleton/manager.h"

#include "source/common/common/logger.h"
#include "source/extensions/common/tap/tap.h"

#include "absl/container/node_hash_set.h"
#include "absl/synchronization/mutex.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {

class ConfigIdMap;
using ConfigIdMapSharedPtr = std::shared_ptr<ConfigIdMap>;

/**
 * ConfigIdMap is a singleton class used by the adminhandler to maintain mappings
 * between config_id strings and sets of tap configurations.
 * This functionality was pulled into a singleton to enable the /tap/config_ids
 * endpoint which requires knowledge of valid config_ids.
 *
 * ConfigIdMap is thread safe, this functionality is provided by an internal mutex lock -
 * contention for this lock should be very low as members are called rarely.
 */
class ConfigIdMap : public Singleton::Instance, Logger::Loggable<Logger::Id::tap> {
  absl::node_hash_map<std::string, absl::node_hash_set<ExtensionConfig*>> config_id_map_;
  mutable absl::Mutex mutex_; // Mutable to allow const accessor methods

public:
  ConfigIdMap() {}
  ~ConfigIdMap() override {}

  /**
   * Get the singleton Config ID Map. The map will be created if it doesn't already exist,
   * otherwise the existing map will be returned.
   * @param singleton_manager Manager used obtain the ConfigIdMap singleton
   */
  static ConfigIdMapSharedPtr getSingleton(Singleton::Manager& singleton_manager);

  /**
   * Registers a new new extension configuration config under key config_id,
   * creating a new entry in the map if needed.
   * Requires that config has not been registered under config_id.
   * @param config_id the key to insert config into the map
   * @param config the extension configuration to register under config_id
   */
  void registerExtensionConfig(ExtensionConfig& config, const std::string& config_id);

  /**
   * Removes a registered admin extension configuration config from the map.
   * If there are no longer any ExtensionConfigs registered with the admin ID,
   * this function removes the admin id key from the map.
   * Requires that config has already been registered under the admin config_id
   * @param config the Admin ExtensionConfiguration to remove
   */
  void unregisterExtensionConfig(ExtensionConfig& config);

  /**
   * Registers a new tap configuration with all extension configurations in the
   * map at the config_id provided inside tap_request
   * Note - at most one tap configuration can be registered with a given Extension
   * configuration at one time. In other words, concurrent taps are not allowed.
   * @param config_id the config id to register tap_config under
   * @param tap_config the protobuf representation of the tap config received
   * @param admin_sink supplies the singleton admin sink to use for output if the tap_request
   * configuration specifies that output type. May not be used if the configuration does not specify
   *        admin output. May be nullptr if admin is not used to supply the config.
   */
  void registerTapConfig(const std::string& config_id,
                         const envoy::config::tap::v3::TapConfig& tap_config, Sink* admin_sink);

  /**
   * Removes the tap configuration registered via registerTapConfig from all
   * ExtensionConfigs in the map at the given config_id.
   * @param config_id specifies the set of ExtensionConfig objects to clear the tap config for
   */
  void unregisterTapConfig(const std::string& config_id);

  /**
   * hasConfigId returns true if the internal map contains an ExtensionConfig registered
   * under config_id as a key, false otherwise
   * @param config_id The config_id to search for in the map
   */
  bool hasConfigId(const std::string& config_id) const {
    absl::ReaderMutexLock lock(&mutex_);
    return config_id_map_.count(config_id) != 0;
  }

  /**
   * keys returns a vector of all config_ids in the map
   */
  std::vector<std::string> keys() const {
    absl::ReaderMutexLock lock(&mutex_);
    std::vector<std::string> keyList;
    keyList.reserve(config_id_map_.size());

    for (const auto& p : config_id_map_) {
      keyList.emplace_back(p.first);
    }

    return keyList;
  }
};

} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy