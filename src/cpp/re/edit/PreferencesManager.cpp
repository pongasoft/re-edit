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

#include "PreferencesManager.h"
#include "lua/ConfigParser.h"
#include "Constants.h"
#include <iosfwd>

namespace re::edit {

//------------------------------------------------------------------------
// PreferencesManager::load
//------------------------------------------------------------------------
config::Global PreferencesManager::load(NativePreferencesManager const *iPreferencesManager)
{
  if(iPreferencesManager)
  {
    auto luaString = iPreferencesManager->load();
    if(luaString)
      return lua::GlobalConfigParser::fromString(*luaString);
  }

  return {};
}

//------------------------------------------------------------------------
// PreferencesManager::save
//------------------------------------------------------------------------
void PreferencesManager::save(NativePreferencesManager const *iPreferencesManager, config::Global const &iConfig)
{
  if(!iPreferencesManager)
    return;

  iPreferencesManager->save(getAsLua(iConfig));
}

//------------------------------------------------------------------------
// PreferencesManager::getAsLua
//------------------------------------------------------------------------
std::string PreferencesManager::getAsLua(config::Global const &iConfig)
{
  std::stringstream s{};

  s << "format_version = \"1.0\"\n\n";
  s << "global_config = {}\n";

  s << fmt::printf("global_config[\"native_window_width\"] = %d\n", iConfig.fNativeWindowWidth);
  s << fmt::printf("global_config[\"native_window_height\"] = %d\n", iConfig.fNativeWindowHeight);
  s << fmt::printf("global_config[\"font_size\"] = %d\n", static_cast<int>(iConfig.fFontSize));

  auto const &history = iConfig.fDeviceHistory;
  if(!history.empty())
  {
    s << "global_config[\"device_history\"] = {}\n";
    int index = 0;
    for(auto const &item: history)
    {
      index++;
      s << fmt::printf(R"(global_config["device_history"][%d] = {
  name = "%s",
  path = [==[%s]==],
  type = "%s",
  last_opened_time = %ld
}
)",
                       index, item.fName, item.fPath, item.fType, item.fLastOpenedTime);
    }
  }

  return s.str();
}

}