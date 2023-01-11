/*
 * Copyright (c) 2023 pongasoft
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

#include "String.h"
#include "imgui.h"

extern IMGUI_API ImGuiID ImHashStr(const char* data, size_t data_size = 0, ImU32 seed = 0);

namespace re::edit {

//------------------------------------------------------------------------
// StringWithHash::StringWithHash
//------------------------------------------------------------------------
StringWithHash::StringWithHash(std::string s) :
  fValue{std::move(s)},
  fHash{ImHashStr(fValue.c_str())}
{
  // empty
}

//------------------------------------------------------------------------
// StringWithHash::StringWithHash
//------------------------------------------------------------------------
StringWithHash::StringWithHash(char const *s) :
  fValue(s),
  fHash{ImHashStr(fValue.c_str())}
{
  // empty
}
}