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
#include <nlohmann/json.hpp>

namespace re::edit {

using json = nlohmann::json;

constexpr auto kReEditCodebergURL = RE_EDIT_NATIVE_STRING("https://codeberg.org/api/v1/repos/pongasoft/re-edit/releases/latest");
const std::map<NetworkManager::native_string_t, NetworkManager::native_string_t> kReEditCodebergAPIHeaders{
  {RE_EDIT_NATIVE_STRING("Accept"), RE_EDIT_NATIVE_STRING("application/json")}
};

namespace impl {

std::optional<std::string> optional(json const &iObject, std::string const &iKey)
{
  auto i = iObject.find(iKey);
  if(i != iObject.end() && i->is_string())
    return *i;
  else
    return std::nullopt;
}

}

//------------------------------------------------------------------------
// NetworkManager::getLatestRelease
//------------------------------------------------------------------------
std::optional<Release> NetworkManager::getLatestRelease() const
{
  auto content = HttpGet(kReEditCodebergURL, kReEditCodebergAPIHeaders);
  if(content)
  {
    auto release = json::parse(*content);
    if(release.is_object())
    {
      auto version = impl::optional(release, "tag_name");
      if(version)
      {
        return Release{*version,
                       impl::optional(release, "html_url"),
                       impl::optional(release, "body")};
      }
    }
  }
  return std::nullopt;
}

}