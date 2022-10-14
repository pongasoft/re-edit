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

#include "ReGui.h"
#include "Errors.h"

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// Window::Lifecycle::~Lifecycle
//------------------------------------------------------------------------
Window::Lifecycle::~Lifecycle()
{
  if(fEndRequired)
  {
    try
    {
      ImGui::End();
    }
    catch(ImGuiException &e)
    {
      RE_EDIT_LOG_ERROR("%s", e.what());
    }
  }
}

//------------------------------------------------------------------------
// Window::begin
//------------------------------------------------------------------------
Window::Lifecycle Window::begin(ImGuiWindowFlags flags)
{
  if(fVisible)
  {
    if(fSizeToFitRequested > 0)
    {
      flags |= ImGuiWindowFlags_AlwaysAutoResize;
      fSizeToFitRequested--;
    }
    return Lifecycle{ImGui::Begin(fName, fDisableClosingWidget ? nullptr : &fVisible, fFlags | flags)};
  }
  else
    return {};
}
}