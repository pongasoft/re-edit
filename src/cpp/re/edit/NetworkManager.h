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

#ifndef RE_EDIT_NETWORK_MANAGER_H
#define RE_EDIT_NETWORK_MANAGER_H

#include <string>
#include <optional>
#include <map>
#include <memory>

namespace re::edit {

struct Release
{
  std::string fVersion{};
  std::optional<std::string> fURL{};
  std::optional<std::string> fReleaseNotes{};
};

class NetworkManager
{
public:
#if __APPLE__
  using native_string_t = std::string;
#define RE_EDIT_NATIVE_STRING(s) s
#else
  using native_string_t = std::wstring;
#define RE_EDIT_NATIVE_STRING(s) L ## s
#endif

public:
  virtual ~NetworkManager() = default;

  std::optional<Release> getLatestRelease() const;

protected:
  virtual std::optional<std::string> HttpGet(native_string_t const &iUrl,
                                             std::map<native_string_t, native_string_t> const &iHeaders) const = 0;
};

}

#endif //RE_EDIT_NETWORK_MANAGER_H
