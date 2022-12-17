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

#include "NetworkManager.h"

namespace re::edit {

// using Jamba for now as re-edit is NOT public yet!
constexpr auto kReEditGithubURL = RE_EDIT_NATIVE_STRING("https://api.github.com/repos/pongasoft/jamba/releases/latest");
const std::map<NetworkManager::native_string_t, NetworkManager::native_string_t> kReEditGithubAPIHeaders{
  {RE_EDIT_NATIVE_STRING("X-GitHub-Api-Version"), RE_EDIT_NATIVE_STRING("2022-11-28")},
  {RE_EDIT_NATIVE_STRING("Accept"),               RE_EDIT_NATIVE_STRING("application/vnd.github+json")}
};

//------------------------------------------------------------------------
// NetworkManager::getLatestRelease
//------------------------------------------------------------------------
std::optional<Release> NetworkManager::getLatestRelease() const
{
  auto content = HttpGet(kReEditGithubURL, kReEditGithubAPIHeaders);
  if(content)
    return Release{"TBD", *content};
  else
    return std::nullopt;
}

}