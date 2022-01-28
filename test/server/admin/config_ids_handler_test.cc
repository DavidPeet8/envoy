#include <memory>

#include "envoy/config/tap/v3/common.pb.h"

#include "source/extensions/common/tap/tap.h"

#include "test/server/admin/admin_instance.h"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_split.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Server {

namespace Tap = Envoy::Extensions::Common::Tap;
namespace ProtoConfig = envoy::config::tap::v3;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

class MockExtensionConfig : public Tap::ExtensionConfig {
public:
  MOCK_METHOD(const absl::string_view, adminId, ());
  MOCK_METHOD(void, clearTapConfig, ());
  MOCK_METHOD(void, newTapConfig,
              (const ProtoConfig::TapConfig& proto_config, Tap::Sink* admin_streamer));
};

class ConfigIdsTest : public AdminInstanceTest {
public:
  ConfigIdsTest() : map(Tap::ConfigIdMap::getSingleton(admin_.server().singletonManager())) {}

  // Add a new entry to the config id map
  void addConfigMapEntry(const std::string& config_id) {
    auto extension_config = MockExtensionConfig();
    map->registerExtensionConfig(extension_config, config_id);
  }

  Tap::ConfigIdMapSharedPtr map;
};

INSTANTIATE_TEST_SUITE_P(IpVersions, ConfigIdsTest,
                         testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                         TestUtility::ipTestParamsToString);

// Test configuring map content & dumping config_ids
TEST_P(ConfigIdsTest, ConfigIds) {
  absl::flat_hash_set<std::string> config_ids{"test_cfg_id_1", "test_cfg_id_2", "test_cfg_id_3"};
  Buffer::OwnedImpl response;
  Http::TestResponseHeaderMapImpl header_map;

  // Add entries to the config_ids map
  for (const auto& id : config_ids) {
    addConfigMapEntry(id);
  }

  EXPECT_EQ(Http::Code::OK, getCallback("/tap/config_ids", header_map, response));
  std::vector<std::string> output_ids = absl::StrSplit(response.toString(), "\n");
  output_ids.pop_back(); // last element is empty due to trailing newline

  // match response with expected ids - note that response may not be in
  // order due to hash map internals
  for (const auto& id : output_ids) {
    EXPECT_NE(config_ids.find(id), config_ids.end());
    config_ids.erase(id);
  }

  EXPECT_TRUE(config_ids.empty());
}

// Test dumping config_ids on an empty map
TEST_P(ConfigIdsTest, ConfigIdsEmpty) {
  Buffer::OwnedImpl response;
  Http::TestResponseHeaderMapImpl header_map;

  EXPECT_EQ(Http::Code::OK, getCallback("/tap/config_ids", header_map, response));
  EXPECT_EQ("", response.toString());
}

} // namespace Server
} // namespace Envoy