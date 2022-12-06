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
global_config["font_size"] = 20
global_config["device_history"] = {}
global_config["device_history"][1] = {
  name = "CVA-7 CV Analyzer",
  path = [==[/Volumes/Development/github/org.pongasoft/re-cva-7]==],
  type = "helper",
  show_properties = false,
  show_panel = true,
  show_panel_widgets = true,
  show_widgets = true,
  grid = { 10, 10 },
  ["imgui.ini"] = [==[<snipped> / CVA-7 CV Analyzer]==],
  native_window_pos = { 351, 151 },
  native_window_size = { 1217, 687 },
  last_access_time = 1670339790319718
}
global_config["device_history"][2] = {
  name = "Small",
  path = [==[/Volumes/Vault/tmp/com.acme.Small-plugin]==],
  type = "studio_fx",
  show_properties = false,
  show_panel = true,
  show_panel_widgets = true,
  show_widgets = true,
  grid = { 10, 10 },
  ["imgui.ini"] = [==[<snipped> / Small]==],
  native_window_pos = { 222, 171 },
  native_window_size = { 1280, 720 },
  last_access_time = 1670269282284453
}
)";

  auto config = GlobalConfigParser::fromString(configString);
  ASSERT_EQ(configString, PreferencesManager::getAsLua(config));
  
  ASSERT_EQ(20, config.fFontSize);
  ASSERT_EQ(2, config.fDeviceHistory.size());

  {
    auto const &item = config.fDeviceHistory[0];
    ASSERT_EQ("CVA-7 CV Analyzer", item.fName);
    ASSERT_EQ("/Volumes/Development/github/org.pongasoft/re-cva-7", item.fPath);
    ASSERT_EQ("helper", item.fType);
    ASSERT_FALSE(item.fShowProperties);
    ASSERT_TRUE(item.fShowPanel);
    ASSERT_TRUE(item.fShowPanelWidgets);
    ASSERT_TRUE(item.fShowWidgets);
    ASSERT_FLOAT_EQ(10.0f, item.fGrid.x); ASSERT_FLOAT_EQ(10.0f, item.fGrid.y);
    ASSERT_EQ(ImVec2(10.0f, 10.0f), item.fGrid);
    ASSERT_EQ("<snipped> / CVA-7 CV Analyzer", item.fImGuiIni);
    ASSERT_FLOAT_EQ(351.0f, item.fNativeWindowPos->x); ASSERT_FLOAT_EQ(151.0f, item.fNativeWindowPos->y);
    ASSERT_FLOAT_EQ(1217.0f, item.fNativeWindowSize.x); ASSERT_FLOAT_EQ(687.0f, item.fNativeWindowSize.y);
    ASSERT_EQ(1670339790319718, item.fLastAccessTime);
  }

  {
    auto const &item = config.fDeviceHistory[1];
    ASSERT_EQ("Small", item.fName);
    ASSERT_EQ("/Volumes/Vault/tmp/com.acme.Small-plugin", item.fPath);
    ASSERT_EQ("studio_fx", item.fType);
    ASSERT_FALSE(item.fShowProperties);
    ASSERT_TRUE(item.fShowPanel);
    ASSERT_TRUE(item.fShowPanelWidgets);
    ASSERT_TRUE(item.fShowWidgets);
    ASSERT_FLOAT_EQ(10.0f, item.fGrid.x); ASSERT_FLOAT_EQ(10.0f, item.fGrid.y);
    ASSERT_EQ(ImVec2(10.0f, 10.0f), item.fGrid);
    ASSERT_EQ("<snipped> / Small", item.fImGuiIni);
    ASSERT_FLOAT_EQ(222.0f, item.fNativeWindowPos->x); ASSERT_FLOAT_EQ(171.0f, item.fNativeWindowPos->y);
    ASSERT_FLOAT_EQ(1280.0f, item.fNativeWindowSize.x); ASSERT_FLOAT_EQ(720.0f, item.fNativeWindowSize.y);
    ASSERT_EQ(1670269282284453, item.fLastAccessTime);
  }

}

}