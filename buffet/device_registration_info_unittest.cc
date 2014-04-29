// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <base/json/json_reader.h>
#include <base/values.h>
#include <gtest/gtest.h>

#include "buffet/bind_lambda.h"
#include "buffet/device_registration_info.h"
#include "buffet/device_registration_storage_keys.h"
#include "buffet/http_request.h"
#include "buffet/http_transport_fake.h"
#include "buffet/mime_utils.h"

using namespace buffet;
using namespace chromeos;
using namespace chromeos::http;

namespace {
// StorageInterface for testing. Just stores the values in memory.
class MemStorage : public DeviceRegistrationInfo::StorageInterface {
 public:
  virtual std::unique_ptr<base::Value> Load() override {
    return std::unique_ptr<base::Value>(cache_->DeepCopy());
  }

  virtual bool Save(const base::Value* config) {
    cache_.reset(config->DeepCopy());
    ++save_count_;
    return true;
  }

  int save_count_ = 0;

private:
  std::unique_ptr<base::Value> cache_;
};

namespace test_data {

const char kServiceURL[]           = "http://gcd.server.com/";
const char kOAuthURL[]             = "http://oauth.server.com/";
const char kApiKey[]               = "GOadRdTf9FERf0k4w6EFOof56fUJ3kFDdFL3d7f";
const char kClientId[]             = "123543821385-sfjkjshdkjhfk234sdfsdfkskd"
                                     "fkjh7f.apps.googleusercontent.com";
const char kClientSecret[]         = "5sdGdGlfolGlrFKfdFlgP6FG";
const char kDeviceId[]             = "4a7ea2d1-b331-1e1f-b206-e863c7635196";
const char kClaimTicketId[]        = "RTcUE";
const char kAccessToken[]          = "ya29.1.AADtN_V-dLUM-sVZ0qVjG9Dxm5NgdS9J"
                                     "Mx_JLUqhC9bED_YFjzHZtYt65ZzXCS35NMAeaVZ"
                                     "Dei530-w0yE2urpQ";
const char kRefreshToken[]         = "1/zQmxR6PKNvhcxf9SjXUrCjcmCrcqRKXctc6cp"
                                     "1nI-GQ";
const char kRobotAccountAuthCode[] = "4/Mf_ujEhPejVhOq-OxW9F5cSOnWzx."
                                     "YgciVjTYGscRshQV0ieZDAqiTIjMigI";
const char kRobotAccountEmail[]    = "6ed0b3f54f9bd619b942f4ad2441c252@"
                                     "clouddevices.gserviceaccount.com";
const char kUserAccountAuthCode[]  = "2/sd_GD1TGFKpJOLJ34-0g5fK0fflp.GlT"
                                     "I0F5g7hNtFgj5HFGOf8FlGK9eflO";
const char kUserAccessToken[]      = "sd56.4.FGDjG_F-gFGF-dFG6gGOG9Dxm5NgdS9"
                                     "JMx_JLUqhC9bED_YFjLKjlkjLKJlkjLKjlKJea"
                                     "VZDei530-w0yE2urpQ";
const char kUserRefreshToken[]     = "1/zQLKjlKJlkLkLKjLkjLKjLkjLjLkjl0ftc6"
                                     "cp1nI-GQ";

}  // namespace test_data

// Fill in the storage with default environment information (URLs, etc).
void InitDefaultStorage(base::DictionaryValue* data) {
  data->SetString(storage_keys::kClientId, test_data::kClientId);
  data->SetString(storage_keys::kClientSecret, test_data::kClientSecret);
  data->SetString(storage_keys::kApiKey, test_data::kApiKey);
  data->SetString(storage_keys::kRefreshToken, "");
  data->SetString(storage_keys::kDeviceId, "");
  data->SetString(storage_keys::kOAuthURL, test_data::kOAuthURL);
  data->SetString(storage_keys::kServiceURL, test_data::kServiceURL);
  data->SetString(storage_keys::kRobotAccount, "");
}

// Add the test device registration information.
void SetDefaultDeviceRegistration(base::DictionaryValue* data) {
  data->SetString(storage_keys::kRefreshToken, test_data::kRefreshToken);
  data->SetString(storage_keys::kDeviceId, test_data::kDeviceId);
  data->SetString(storage_keys::kRobotAccount, test_data::kRobotAccountEmail);
}

void OAuth2Handler(const fake::ServerRequest& request,
                  fake::ServerResponse* response) {
  base::DictionaryValue json;
  if (request.GetFormField("grant_type") == "refresh_token") {
    // Refresh device access token.
    EXPECT_EQ(test_data::kRefreshToken, request.GetFormField("refresh_token"));
    EXPECT_EQ(test_data::kClientId, request.GetFormField("client_id"));
    EXPECT_EQ(test_data::kClientSecret, request.GetFormField("client_secret"));
    json.SetString("access_token", test_data::kAccessToken);
  } else if (request.GetFormField("grant_type") == "authorization_code") {
    // Obtain access token.
    std::string code = request.GetFormField("code");
    if (code == test_data::kUserAccountAuthCode) {
      // Get user access token.
      EXPECT_EQ(test_data::kClientId, request.GetFormField("client_id"));
      EXPECT_EQ(test_data::kClientSecret,
                request.GetFormField("client_secret"));
      EXPECT_EQ("urn:ietf:wg:oauth:2.0:oob",
                request.GetFormField("redirect_uri"));
      json.SetString("access_token", test_data::kUserAccessToken);
      json.SetString("token_type", "Bearer");
      json.SetString("refresh_token", test_data::kUserRefreshToken);
    } else if (code == test_data::kRobotAccountAuthCode) {
      // Get device access token.
      EXPECT_EQ(test_data::kClientId, request.GetFormField("client_id"));
      EXPECT_EQ(test_data::kClientSecret,
                request.GetFormField("client_secret"));
      EXPECT_EQ("oob", request.GetFormField("redirect_uri"));
      EXPECT_EQ("https://www.googleapis.com/auth/clouddevices",
                request.GetFormField("scope"));
      json.SetString("access_token", test_data::kAccessToken);
      json.SetString("token_type", "Bearer");
      json.SetString("refresh_token", test_data::kRefreshToken);
    } else {
      ASSERT_TRUE(false); // Unexpected authorization code.
    }
  } else {
    ASSERT_TRUE(false); // Unexpected grant type.
  }
  json.SetInteger("expires_in", 3600);
  response->ReplyJson(status_code::Ok, &json);
}

void DeviceInfoHandler(const fake::ServerRequest& request,
                       fake::ServerResponse* response) {
  std::string auth = "Bearer ";
  auth += test_data::kAccessToken;
  EXPECT_EQ(auth, request.GetHeader(http::request_header::kAuthorization));
  response->ReplyJson(status_code::Ok, {
    {"channel.supportedType", "xmpp"},
    {"deviceKind", "vendor"},
    {"id", test_data::kDeviceId},
    {"kind", "clouddevices#device"},
  });
}

void FinalizeTicketHandler(const fake::ServerRequest& request,
                           fake::ServerResponse* response) {
  EXPECT_EQ(test_data::kApiKey, request.GetFormField("key"));
  EXPECT_TRUE(request.GetData().empty());

  response->ReplyJson(status_code::Ok, {
    {"id", test_data::kClaimTicketId},
    {"kind", "clouddevices#registrationTicket"},
    {"oauthClientId", test_data::kClientId},
    {"userEmail", "user@email.com"},
    {"deviceDraft.id", test_data::kDeviceId},
    {"deviceDraft.kind", "clouddevices#device"},
    {"deviceDraft.channel.supportedType", "xmpp"},
    {"robotAccountEmail", test_data::kRobotAccountEmail},
    {"robotAccountAuthorizationCode", test_data::kRobotAccountAuthCode},
  });
}

}  // anonymous namespace

// This is a helper class that allows the unit tests to set the private
// member DeviceRegistrationInfo::ticket_id_, since TestHelper is declared
// as a friend to DeviceRegistrationInfo.
class DeviceRegistrationInfo::TestHelper {
 public:
  static void SetTestTicketId(DeviceRegistrationInfo* info) {
    info->ticket_id_ = test_data::kClaimTicketId;
  }
};

class DeviceRegistrationInfoTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    InitDefaultStorage(&data);
    storage = std::make_shared<MemStorage>();
    storage->Save(&data);
    transport = std::make_shared<fake::Transport>();
    dev_reg = std::unique_ptr<DeviceRegistrationInfo>(
        new DeviceRegistrationInfo(transport, storage));
  }

  base::DictionaryValue data;
  std::shared_ptr<MemStorage> storage;
  std::shared_ptr<fake::Transport> transport;
  std::unique_ptr<DeviceRegistrationInfo> dev_reg;
};

////////////////////////////////////////////////////////////////////////////////
TEST_F(DeviceRegistrationInfoTest, GetServiceURL) {
  EXPECT_TRUE(dev_reg->Load());
  EXPECT_EQ(test_data::kServiceURL, dev_reg->GetServiceURL());
  std::string url = test_data::kServiceURL;
  url += "registrationTickets";
  EXPECT_EQ(url, dev_reg->GetServiceURL("registrationTickets"));
  url += "?key=";
  url += test_data::kApiKey;
  EXPECT_EQ(url, dev_reg->GetServiceURL("registrationTickets", {
    {"key", test_data::kApiKey}
  }));
  url += "&restart=true";
  EXPECT_EQ(url, dev_reg->GetServiceURL("registrationTickets", {
    {"key", test_data::kApiKey},
    {"restart", "true"},
  }));
}

TEST_F(DeviceRegistrationInfoTest, GetOAuthURL) {
  EXPECT_TRUE(dev_reg->Load());
  EXPECT_EQ(test_data::kOAuthURL, dev_reg->GetOAuthURL());
  std::string url = test_data::kOAuthURL;
  url += "auth?scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fclouddevices&";
  url += "redirect_uri=urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob&";
  url += "response_type=code&";
  url += "client_id=";
  url += test_data::kClientId;
  EXPECT_EQ(url, dev_reg->GetOAuthURL("auth", {
    {"scope", "https://www.googleapis.com/auth/clouddevices"},
    {"redirect_uri", "urn:ietf:wg:oauth:2.0:oob"},
    {"response_type", "code"},
    {"client_id", test_data::kClientId}
  }));
}

TEST_F(DeviceRegistrationInfoTest, CheckRegistration) {
  EXPECT_TRUE(dev_reg->Load());
  EXPECT_FALSE(dev_reg->CheckRegistration());
  EXPECT_EQ(0, transport->GetRequestCount());

  SetDefaultDeviceRegistration(&data);
  storage->Save(&data);
  EXPECT_TRUE(dev_reg->Load());

  transport->AddHandler(dev_reg->GetOAuthURL("token"), request_type::kPost,
                        base::Bind(OAuth2Handler));
  transport->ResetRequestCount();
  EXPECT_TRUE(dev_reg->CheckRegistration());
  EXPECT_EQ(1, transport->GetRequestCount());
}

TEST_F(DeviceRegistrationInfoTest, GetDeviceInfo) {
  SetDefaultDeviceRegistration(&data);
  storage->Save(&data);
  EXPECT_TRUE(dev_reg->Load());

  transport->AddHandler(dev_reg->GetOAuthURL("token"), request_type::kPost,
                        base::Bind(OAuth2Handler));
  transport->AddHandler(dev_reg->GetDeviceURL(), request_type::kGet,
                        base::Bind(DeviceInfoHandler));
  transport->ResetRequestCount();
  auto device_info = dev_reg->GetDeviceInfo();
  EXPECT_EQ(2, transport->GetRequestCount());
  EXPECT_NE(nullptr, device_info.get());
  base::DictionaryValue* dict = nullptr;
  EXPECT_TRUE(device_info->GetAsDictionary(&dict));
  std::string id;
  EXPECT_TRUE(dict->GetString("id", &id));
  EXPECT_EQ(test_data::kDeviceId, id);
}

TEST_F(DeviceRegistrationInfoTest, GetDeviceId) {
  SetDefaultDeviceRegistration(&data);
  storage->Save(&data);
  EXPECT_TRUE(dev_reg->Load());

  transport->AddHandler(dev_reg->GetOAuthURL("token"), request_type::kPost,
                        base::Bind(OAuth2Handler));
  transport->AddHandler(dev_reg->GetDeviceURL(), request_type::kGet,
                        base::Bind(DeviceInfoHandler));
  std::string id = dev_reg->GetDeviceId();
  EXPECT_EQ(test_data::kDeviceId, id);
}

TEST_F(DeviceRegistrationInfoTest, StartRegistration) {
  EXPECT_TRUE(dev_reg->Load());

  auto create_ticket = [](const fake::ServerRequest& request,
                          fake::ServerResponse* response) {
    EXPECT_EQ(test_data::kApiKey, request.GetFormField("key"));
    auto json = request.GetDataAsJson();
    EXPECT_NE(nullptr, json.get());
    std::string value;
    EXPECT_TRUE(json->GetString("deviceDraft.channel.supportedType", &value));
    EXPECT_EQ("xmpp", value);
    EXPECT_TRUE(json->GetString("oauthClientId", &value));
    EXPECT_EQ(test_data::kClientId, value);
    EXPECT_TRUE(json->GetString("deviceDraft.deviceKind", &value));
    EXPECT_EQ("vendor", value);

    base::DictionaryValue json_resp;
    json_resp.SetString("id", test_data::kClaimTicketId);
    json_resp.SetString("kind", "clouddevices#registrationTicket");
    json_resp.SetString("oauthClientId", test_data::kClientId);
    base::DictionaryValue* device_draft = nullptr;
    EXPECT_TRUE(json->GetDictionary("deviceDraft", &device_draft));
    device_draft = device_draft->DeepCopy();
    device_draft->SetString("id", test_data::kDeviceId);
    device_draft->SetString("kind", "clouddevices#device");
    json_resp.Set("deviceDraft", device_draft);

    response->ReplyJson(status_code::Ok, &json_resp);
  };

  transport->AddHandler(dev_reg->GetServiceURL("registrationTickets"),
                        request_type::kPost,
                        base::Bind(create_ticket));
  std::map<std::string, std::shared_ptr<base::Value>> params;
  std::string json_resp = dev_reg->StartRegistration(params, nullptr);
  auto json = std::unique_ptr<base::Value>(base::JSONReader::Read(json_resp));
  EXPECT_NE(nullptr, json.get());
  base::DictionaryValue* dict = nullptr;
  EXPECT_TRUE(json->GetAsDictionary(&dict));
  std::string value;
  EXPECT_TRUE(dict->GetString("ticket_id", &value));
  EXPECT_EQ(test_data::kClaimTicketId, value);
}

TEST_F(DeviceRegistrationInfoTest, FinishRegistration_NoAuth) {
  // Test finalizing ticket with no user authorization token.
  // This assumes that a client would patch in their email separately.
  EXPECT_TRUE(dev_reg->Load());

  // General ticket finalization handler.
  std::string ticket_url =
      dev_reg->GetServiceURL("registrationTickets/" +
                             std::string(test_data::kClaimTicketId));
  transport->AddHandler(ticket_url + "/finalize", request_type::kPost,
                        base::Bind(FinalizeTicketHandler));

  transport->AddHandler(dev_reg->GetOAuthURL("token"), request_type::kPost,
                        base::Bind(OAuth2Handler));

  storage->save_count_ = 0;
  DeviceRegistrationInfo::TestHelper::SetTestTicketId(dev_reg.get());
  EXPECT_TRUE(dev_reg->FinishRegistration(""));
  EXPECT_EQ(1, storage->save_count_); // The device info must have been saved.
  EXPECT_EQ(2, transport->GetRequestCount());

  // Validate the device info saved to storage...
  auto storage_data = storage->Load();
  base::DictionaryValue* dict = nullptr;
  EXPECT_TRUE(storage_data->GetAsDictionary(&dict));
  std::string value;
  EXPECT_TRUE(dict->GetString(storage_keys::kApiKey, &value));
  EXPECT_EQ(test_data::kApiKey, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kClientId, &value));
  EXPECT_EQ(test_data::kClientId, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kClientSecret, &value));
  EXPECT_EQ(test_data::kClientSecret, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kDeviceId, &value));
  EXPECT_EQ(test_data::kDeviceId, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kOAuthURL, &value));
  EXPECT_EQ(test_data::kOAuthURL, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kRefreshToken, &value));
  EXPECT_EQ(test_data::kRefreshToken, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kRobotAccount, &value));
  EXPECT_EQ(test_data::kRobotAccountEmail, value);
  EXPECT_TRUE(dict->GetString(storage_keys::kServiceURL, &value));
  EXPECT_EQ(test_data::kServiceURL, value);
}

TEST_F(DeviceRegistrationInfoTest, FinishRegistration_Auth) {
  // Test finalizing ticket with user authorization token.
  EXPECT_TRUE(dev_reg->Load());

  // General ticket finalization handler.
  std::string ticket_url =
      dev_reg->GetServiceURL("registrationTickets/" +
                             std::string(test_data::kClaimTicketId));
  transport->AddHandler(ticket_url + "/finalize", request_type::kPost,
                        base::Bind(FinalizeTicketHandler));

  transport->AddHandler(dev_reg->GetOAuthURL("token"), request_type::kPost,
                        base::Bind(OAuth2Handler));

  // Handle patching in the user email onto the device record.
  auto email_patch_handler = [](const fake::ServerRequest& request,
                                fake::ServerResponse* response) {
    std::string auth_header = "Bearer ";
    auth_header += test_data::kUserAccessToken;
    EXPECT_EQ(auth_header,
              request.GetHeader(http::request_header::kAuthorization));
    auto json = request.GetDataAsJson();
    EXPECT_NE(nullptr, json.get());
    std::string value;
    EXPECT_TRUE(json->GetString("userEmail", &value));
    EXPECT_EQ("me", value);

    response->ReplyJson(status_code::Ok, {
      {"id", test_data::kClaimTicketId},
      {"kind", "clouddevices#registrationTicket"},
      {"oauthClientId", test_data::kClientId},
      {"userEmail", "user@email.com"},
      {"deviceDraft.id", test_data::kDeviceId},
      {"deviceDraft.kind", "clouddevices#device"},
      {"deviceDraft.channel.supportedType", "xmpp"},
    });
  };
  transport->AddHandler(ticket_url, request_type::kPatch,
                        base::Bind(email_patch_handler));

  storage->save_count_ = 0;
  DeviceRegistrationInfo::TestHelper::SetTestTicketId(dev_reg.get());
  EXPECT_TRUE(dev_reg->FinishRegistration(test_data::kUserAccountAuthCode));
  EXPECT_EQ(1, storage->save_count_); // The device info must have been saved.
  EXPECT_EQ(4, transport->GetRequestCount());
}