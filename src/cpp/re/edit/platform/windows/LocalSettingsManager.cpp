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

#include "LocalSettingsManager.h"
#include "../../Application.h"
#include "../../Errors.h"
#include "../../fs.h"

namespace re::edit {

namespace impl {

//------------------------------------------------------------------------
// impl::getOrCreateSettingsFolder
//------------------------------------------------------------------------
std::optional<fs::path> getOrCreateSettingsFolder()
{
  char *pLocalAppData;
  size_t len;
  errno_t err = _dupenv_s(&pLocalAppData, &len, "localappdata");
  if(err) return std::nullopt; // this would be weird: no %LOCALAPPDATA% variable
  std::string sLocalAppData{pLocalAppData};
  free(pLocalAppData); // it is the responsibility of the caller to delete this!

  fs::path localAppData{sLocalAppData};

  if(!fs::exists(localAppData))
    return std::nullopt; // this would be weird: the local app data folder does not exist

  auto settingsFolder = localAppData / "pongasoft" / "re-edit";
  if(!fs::exists(settingsFolder))
    fs::create_directories(settingsFolder);

  return settingsFolder;
}

}

//------------------------------------------------------------------------
// LocalSettingsManager::load
//------------------------------------------------------------------------
std::optional<std::string> LocalSettingsManager::load() const
{
  auto settingsFolder = impl::getOrCreateSettingsFolder();
  if(settingsFolder)
    return Application::readFile(*settingsFolder / "preferences.lua");
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// LocalSettingsManager::save
//------------------------------------------------------------------------
void LocalSettingsManager::save(std::string const &iPreferences) const
{
  auto settingsFolder = impl::getOrCreateSettingsFolder();
  if(settingsFolder)
    Application::saveFile(*settingsFolder / "preferences.lua", iPreferences);
}

}