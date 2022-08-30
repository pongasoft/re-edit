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
void Color3::editView(AppContext &iCtx)
{
  resetView();
  ImGui::SameLine();

  auto editedValue = fValue;

  ReGui::ColorEdit(fName, &editedValue);

  auto begin = ImGui::IsItemActivated();
  auto commit = ImGui::IsItemDeactivated();

  if(commit)
    fValueUndoTx.commit(iCtx, editedValue);

  if(begin)
    fValueUndoTx.beginCurrentWidgetAttribute(iCtx, fValue, this);

  if(fValue != editedValue)
  {
    fValue = editedValue;
    fProvided = true;
    fEdited = true;
  }
}

}