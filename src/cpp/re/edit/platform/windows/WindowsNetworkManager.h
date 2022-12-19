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

#ifndef RE_EDIT_WINDOWS_NETWORK_MANAGER_H
#define RE_EDIT_WINDOWS_NETWORK_MANAGER_H

#include "../../NetworkManager.h"

namespace re::edit {

class WindowsNetworkManager : public NetworkManager
{
protected:
  std::optional<std::string> HttpGet(std::wstring const &iUrl,
                                     std::map<std::wstring, std::wstring> const &iHeaders) const override;
};

}

#endif //RE_EDIT_WINDOWS_NETWORK_MANAGER_H
