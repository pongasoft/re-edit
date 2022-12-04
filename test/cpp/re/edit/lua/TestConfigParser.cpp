/*
 * Copyright (c) 2022 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include <gtest/gtest.h>
#include <re/edit/lua/ConfigParser.h>
#include <re/edit/PreferencesManager.h>

namespace re::edit::lua::Test {

TEST(ConfigParser, All) {
  auto configString = R"(format_version = "1.0"

global_config = {}
global_config["native_window_width"] = 1200
global_config["native_window_height"] = 800
global_config["font_size"] = 20
global_config["device_history"] = {}
global_config["device_history"][1] = {
  name = "CVA-7 CV Analyzer",
  path = [==[/Volumes/Development/github/org.pongasoft/re-cva-7]==],
  type = "helper",
  last_opened_time = 1670172939994405
}
global_config["device_history"][2] = {
  name = "Small",
  path = [==[/Volumes/Vault/tmp/com.acme.Small-plugin]==],
  type = "studio_fx",
  last_opened_time = 1670172947156375
}
)";

  auto config = GlobalConfigParser::fromString(configString);
  ASSERT_EQ(configString, PreferencesManager::getAsLua(config));
  
  ASSERT_EQ(1200, config.fNativeWindowWidth);
  ASSERT_EQ(800, config.fNativeWindowHeight);
  ASSERT_EQ(20, config.fFontSize);
  ASSERT_EQ(2, config.fDeviceHistory.size());

  {
    auto const &item = config.fDeviceHistory[0];
    ASSERT_EQ("CVA-7 CV Analyzer", item.fName);
    ASSERT_EQ("/Volumes/Development/github/org.pongasoft/re-cva-7", item.fPath);
    ASSERT_EQ("helper", item.fType);
    ASSERT_EQ(1670172939994405, item.fLastOpenedTime);
  }

  {
    auto const &item = config.fDeviceHistory[1];
    ASSERT_EQ("Small", item.fName);
    ASSERT_EQ("/Volumes/Vault/tmp/com.acme.Small-plugin", item.fPath);
    ASSERT_EQ("studio_fx", item.fType);
    ASSERT_EQ(1670172947156375, item.fLastOpenedTime);
  }

}

}