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

#include "Color.h"
#include "WidgetAttribute.h"
#include "ReGui.h"

namespace re::edit::widget::attribute {

//------------------------------------------------------------------------
// Color3::getValueAsLua
//------------------------------------------------------------------------
std::string Color3::getValueAsLua() const
{
  return re::mock::fmt::printf("{%d,%d,%d}", fValue.fRed, fValue.fGreen, fValue.fBlue);
}

//------------------------------------------------------------------------
// Color3::editView
//------------------------------------------------------------------------
void Color3::editView(EditContext &iCtx)
{
  resetView();
  ImGui::SameLine();
  if(ReGui::ColorEdit(fName.c_str(), &fValue))
  {
    fProvided = true;
  }
}

}