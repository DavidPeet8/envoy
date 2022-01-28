#include <array>
#include <string>
#include <vector>

#include "envoy/config/tap/v3/common.pb.h"

#include "source/extensions/common/tap/config_id_map.h"
#include "source/extensions/common/tap/tap.h"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace Common {
namespace Tap {
namespace {

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Between;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;

class MockAdminSink : public Sink {
public:
  MOCK_METHOD(PerTapSinkHandlePtr, createPerTapSinkHandle,
              (uint64_t, envoy::config::tap::v3::OutputSink::OutputSinkTypeCase));
};

class MockExtensionConfig : public ExtensionConfig {
public:
  MOCK_METHOD(const absl::string_view, adminId, ());
  MOCK_METHOD(void, clearTapConfig, ());
  MOCK_METHOD(void, newTapConfig,
              (const envoy::config::tap::v3::TapConfig& proto_config, Sink* admin_streamer));
};

class ConfigIdMapTest : public testing::Test {
  const std::string base_cfg_id = "test_config";
  size_t id_count = 0;

public:
  ConfigIdMapTest() = default;

  std::string newConfigId() {
    // For brevity - id_count++ increments id_count returning the old value
    return base_cfg_id + std::to_string(id_count++);
  }
};

// Try registering a config_id multiple times
TEST_F(ConfigIdMapTest, ExtCfgSameConfigId) {
  ConfigIdMap map;
  MockExtensionConfig cfg;
  const std::string config_id = newConfigId();

  map.registerExtensionConfig(cfg, config_id);
  EXPECT_DEBUG_DEATH(map.registerExtensionConfig(cfg, config_id), "");
}

// Try registering an empty config id
TEST_F(ConfigIdMapTest, ExtCfgEmptyConfigId) {
  ConfigIdMap map;
  MockExtensionConfig cfg;

  // Verify config_id must be valid
  EXPECT_DEBUG_DEATH(map.registerExtensionConfig(cfg, ""), "");
}

// Try deleting an ExtensionConfig without an admin id
TEST_F(ConfigIdMapTest, ExtCfgEmptyAdminId) {
  ConfigIdMap map;
  MockExtensionConfig cfg;

  ON_CALL(cfg, adminId()).WillByDefault(Return(""));
  EXPECT_DEBUG_DEATH(
      {
        EXPECT_CALL(cfg, adminId()).Times(Between(1, 2));
        map.unregisterExtensionConfig(cfg);
      },
      "");
}

// Try deleting a non-existent config
TEST_F(ConfigIdMapTest, ExtCfgNonexistentConfig) {
  ConfigIdMap map;
  MockExtensionConfig cfg;

  ON_CALL(cfg, adminId()).WillByDefault(Return("admin_id"));
  EXPECT_DEBUG_DEATH(
      {
        EXPECT_CALL(cfg, adminId()).Times(Between(1, 2));
        map.unregisterExtensionConfig(cfg);
      },
      "");
}

// Validate a normal register / unregister sequence for Extension configs
TEST_F(ConfigIdMapTest, ValidExtCfgRegisterUnregister) {
  ConfigIdMap map;
  MockExtensionConfig cfg;
  const std::string config_id = newConfigId();

  ON_CALL(cfg, adminId()).WillByDefault(Return(config_id));
  EXPECT_CALL(cfg, adminId()).Times(Between(1, 2));

  map.registerExtensionConfig(cfg, config_id);
  EXPECT_TRUE(map.hasConfigId(config_id));
  map.unregisterExtensionConfig(cfg);
  EXPECT_FALSE(map.hasConfigId(config_id));
  map.registerExtensionConfig(cfg, config_id);
  EXPECT_TRUE(map.hasConfigId(config_id));
}

// Validate a normal creation / deletion sequence for Tap configs
TEST_F(ConfigIdMapTest, ValidTapCfgCreationDeletion) {
  ConfigIdMap map;
  MockAdminSink sink;
  NiceMock<MockExtensionConfig> cfg;
  NiceMock<MockExtensionConfig> cfg2;
  envoy::config::tap::v3::TapConfig tap;
  envoy::config::tap::v3::TapConfig tap2;

  const std::string config_id = newConfigId();
  const std::string config_id_2 = newConfigId();

  map.registerExtensionConfig(cfg, config_id);
  map.registerExtensionConfig(cfg2, config_id_2);

  ON_CALL(sink, createPerTapSinkHandle(_, _)).WillByDefault(Return(ByMove(nullptr)));
  EXPECT_CALL(sink, createPerTapSinkHandle(_, _)).Times(AnyNumber());

  map.registerTapConfig(config_id, tap, &sink);
  map.registerTapConfig(config_id_2, tap, &sink);

  map.unregisterTapConfig(config_id);
  map.unregisterTapConfig(config_id_2);

  map.registerTapConfig(config_id, tap, &sink);
}

// Validate accessor method behavior
TEST_F(ConfigIdMapTest, Accessors) {
  ConfigIdMap map;
  MockExtensionConfig cfg;
  std::array<std::string, 3> config_ids;

  EXPECT_EQ(map.keys().size(), 0);

  for (size_t i = 0; i < config_ids.size(); i++) {
    config_ids[i] = newConfigId();
    EXPECT_FALSE(map.hasConfigId(config_ids[i]));
    map.registerExtensionConfig(cfg, config_ids[i]);
    EXPECT_TRUE(map.hasConfigId(config_ids[i]));
  }

  std::vector<std::string> keys = map.keys();
  absl::flat_hash_set<std::string> keyset(keys.begin(), keys.end());

  for (const auto& id : config_ids) {
    EXPECT_FALSE(keyset.find(id) == keyset.end());
    keyset.erase(id);
  }
  EXPECT_TRUE(keyset.empty());
}

} // namespace
} // namespace Tap
} // namespace Common
} // namespace Extensions
} // namespace Envoy