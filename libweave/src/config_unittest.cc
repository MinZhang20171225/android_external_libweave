// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libweave/src/config.h"

#include <set>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/mock_config_store.h>

#include "libweave/src/commands/unittest_utils.h"

using testing::_;
using testing::Invoke;
using testing::Return;

namespace weave {

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(*this, OnConfigChanged(_))
        .Times(1);  // Called from AddOnChangedCallback
    config_.reset(new Config{&config_store_});
    config_->AddOnChangedCallback(
        base::Bind(&ConfigTest::OnConfigChanged, base::Unretained(this)));
  }

  const Settings& GetSettings() const { return config_->GetSettings(); }

  const Settings& GetDefaultSettings() const { return default_.GetSettings(); }

  MOCK_METHOD1(OnConfigChanged, void(const Settings&));

  provider::test::MockConfigStore config_store_;
  std::unique_ptr<Config> config_;
  const Config default_{nullptr};
};

TEST_F(ConfigTest, NoStorage) {
  Config config{nullptr};
  Config::Transaction change{&config};
  change.Commit();
}

TEST_F(ConfigTest, Defaults) {
  EXPECT_EQ("58855907228.apps.googleusercontent.com", GetSettings().client_id);
  EXPECT_EQ("eHSAREAHrIqPsHBxCE9zPPBi", GetSettings().client_secret);
  EXPECT_EQ("AIzaSyDSq46gG-AxUnC3zoqD9COIPrjolFsMfMA", GetSettings().api_key);
  EXPECT_EQ("https://accounts.google.com/o/oauth2/", GetSettings().oauth_url);
  EXPECT_EQ("https://www.googleapis.com/clouddevices/v1/",
            GetSettings().service_url);
  EXPECT_EQ("Chromium", GetSettings().oem_name);
  EXPECT_EQ("Brillo", GetSettings().model_name);
  EXPECT_EQ("AAAAA", GetSettings().model_id);
  EXPECT_EQ("", GetSettings().firmware_version);
  EXPECT_EQ(base::TimeDelta::FromSeconds(7), GetSettings().polling_period);
  EXPECT_EQ(base::TimeDelta::FromMinutes(30),
            GetSettings().backup_polling_period);
  EXPECT_TRUE(GetSettings().wifi_auto_setup_enabled);
  EXPECT_FALSE(GetSettings().ble_setup_enabled);
  EXPECT_EQ(std::set<PairingType>{PairingType::kPinCode},
            GetSettings().pairing_modes);
  EXPECT_EQ("", GetSettings().embedded_code);
  EXPECT_EQ("Developer device", GetSettings().name);
  EXPECT_EQ("", GetSettings().description);
  EXPECT_EQ("", GetSettings().location);
  EXPECT_EQ("viewer", GetSettings().local_anonymous_access_role);
  EXPECT_TRUE(GetSettings().local_pairing_enabled);
  EXPECT_TRUE(GetSettings().local_discovery_enabled);
  EXPECT_EQ("", GetSettings().cloud_id);
  EXPECT_EQ("", GetSettings().refresh_token);
  EXPECT_EQ("", GetSettings().robot_account);
  EXPECT_EQ("", GetSettings().last_configured_ssid);
  EXPECT_EQ("", GetSettings().secret);
}

TEST_F(ConfigTest, LoadState) {
  auto state = R"({
    "api_key": "state_api_key",
    "client_id": "state_client_id",
    "client_secret": "state_client_secret",
    "cloud_id": "state_cloud_id",
    "description": "state_description",
    "last_configured_ssid": "state_last_configured_ssid",
    "local_anonymous_access_role": "user",
    "local_discovery_enabled": false,
    "local_pairing_enabled": false,
    "location": "state_location",
    "name": "state_name",
    "oauth_url": "state_oauth_url",
    "refresh_token": "state_refresh_token",
    "robot_account": "state_robot_account",
    "secret": "state_secret",
    "service_url": "state_service_url"
  })";
  EXPECT_CALL(config_store_, LoadSettings()).WillOnce(Return(state));

  EXPECT_CALL(*this, OnConfigChanged(_)).Times(1);
  config_->Load();

  EXPECT_EQ("state_client_id", GetSettings().client_id);
  EXPECT_EQ("state_client_secret", GetSettings().client_secret);
  EXPECT_EQ("state_api_key", GetSettings().api_key);
  EXPECT_EQ("state_oauth_url", GetSettings().oauth_url);
  EXPECT_EQ("state_service_url", GetSettings().service_url);
  EXPECT_EQ(GetDefaultSettings().oem_name, GetSettings().oem_name);
  EXPECT_EQ(GetDefaultSettings().model_name, GetSettings().model_name);
  EXPECT_EQ(GetDefaultSettings().model_id, GetSettings().model_id);
  EXPECT_EQ(GetDefaultSettings().polling_period, GetSettings().polling_period);
  EXPECT_EQ(GetDefaultSettings().backup_polling_period,
            GetSettings().backup_polling_period);
  EXPECT_EQ(GetDefaultSettings().wifi_auto_setup_enabled,
            GetSettings().wifi_auto_setup_enabled);
  EXPECT_EQ(GetDefaultSettings().ble_setup_enabled,
            GetSettings().ble_setup_enabled);
  EXPECT_EQ(GetDefaultSettings().pairing_modes, GetSettings().pairing_modes);
  EXPECT_EQ(GetDefaultSettings().embedded_code, GetSettings().embedded_code);
  EXPECT_EQ("state_name", GetSettings().name);
  EXPECT_EQ("state_description", GetSettings().description);
  EXPECT_EQ("state_location", GetSettings().location);
  EXPECT_EQ("user", GetSettings().local_anonymous_access_role);
  EXPECT_FALSE(GetSettings().local_pairing_enabled);
  EXPECT_FALSE(GetSettings().local_discovery_enabled);
  EXPECT_EQ("state_cloud_id", GetSettings().cloud_id);
  EXPECT_EQ("state_refresh_token", GetSettings().refresh_token);
  EXPECT_EQ("state_robot_account", GetSettings().robot_account);
  EXPECT_EQ("state_last_configured_ssid", GetSettings().last_configured_ssid);
  EXPECT_EQ("state_secret", GetSettings().secret);
}

TEST_F(ConfigTest, Setters) {
  Config::Transaction change{config_.get()};

  change.set_client_id("set_client_id");
  EXPECT_EQ("set_client_id", GetSettings().client_id);

  change.set_client_secret("set_client_secret");
  EXPECT_EQ("set_client_secret", GetSettings().client_secret);

  change.set_api_key("set_api_key");
  EXPECT_EQ("set_api_key", GetSettings().api_key);

  change.set_oauth_url("set_oauth_url");
  EXPECT_EQ("set_oauth_url", GetSettings().oauth_url);

  change.set_service_url("set_service_url");
  EXPECT_EQ("set_service_url", GetSettings().service_url);

  change.set_name("set_name");
  EXPECT_EQ("set_name", GetSettings().name);

  change.set_description("set_description");
  EXPECT_EQ("set_description", GetSettings().description);

  change.set_location("set_location");
  EXPECT_EQ("set_location", GetSettings().location);

  change.set_local_anonymous_access_role("viewer");
  EXPECT_EQ("viewer", GetSettings().local_anonymous_access_role);

  change.set_local_anonymous_access_role("none");
  EXPECT_EQ("none", GetSettings().local_anonymous_access_role);

  change.set_local_anonymous_access_role("user");
  EXPECT_EQ("user", GetSettings().local_anonymous_access_role);

  change.set_local_discovery_enabled(false);
  EXPECT_FALSE(GetSettings().local_discovery_enabled);

  change.set_local_pairing_enabled(false);
  EXPECT_FALSE(GetSettings().local_pairing_enabled);

  change.set_local_discovery_enabled(true);
  EXPECT_TRUE(GetSettings().local_discovery_enabled);

  change.set_local_pairing_enabled(true);
  EXPECT_TRUE(GetSettings().local_pairing_enabled);

  change.set_cloud_id("set_cloud_id");
  EXPECT_EQ("set_cloud_id", GetSettings().cloud_id);

  change.set_refresh_token("set_token");
  EXPECT_EQ("set_token", GetSettings().refresh_token);

  change.set_robot_account("set_account");
  EXPECT_EQ("set_account", GetSettings().robot_account);

  change.set_last_configured_ssid("set_last_configured_ssid");
  EXPECT_EQ("set_last_configured_ssid", GetSettings().last_configured_ssid);

  change.set_secret("set_secret");
  EXPECT_EQ("set_secret", GetSettings().secret);

  EXPECT_CALL(*this, OnConfigChanged(_)).Times(1);

  EXPECT_CALL(config_store_, SaveSettings(_))
      .WillOnce(Invoke([](const std::string& json) {
        auto expected = R"({
          'api_key': 'set_api_key',
          'client_id': 'set_client_id',
          'client_secret': 'set_client_secret',
          'cloud_id': 'set_cloud_id',
          'description': 'set_description',
          'last_configured_ssid': 'set_last_configured_ssid',
          'local_anonymous_access_role': 'user',
          'local_discovery_enabled': true,
          'local_pairing_enabled': true,
          'location': 'set_location',
          'name': 'set_name',
          'oauth_url': 'set_oauth_url',
          'refresh_token': 'set_token',
          'robot_account': 'set_account',
          'secret': 'set_secret',
          'service_url': 'set_service_url'
        })";
        EXPECT_JSON_EQ(expected, *test::CreateValue(json));
      }));
  EXPECT_CALL(config_store_, OnSettingsChanged(_)).Times(1);

  change.Commit();
}

}  // namespace weave
