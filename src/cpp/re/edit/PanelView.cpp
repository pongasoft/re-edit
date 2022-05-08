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

#include "PanelView.h"

namespace re::edit {

//------------------------------------------------------------------------
// PanelView::draw
//------------------------------------------------------------------------
void PanelView::draw(DrawContext &iCtx)
{
  auto top = ImGui::GetCursorPos();
  bool clearAllWidgets = false;
  if(fTexture)
  {
    iCtx.drawTexture(fTexture.get());
    if(ImGui::IsItemClicked()) // true when a widget is clicked as well
      clearAllWidgets = true;
  }
  for(auto &widget: fWidgets)
  {
    auto &w = widget.second;
    w->draw(iCtx);
    if(ImGui::IsItemClicked())
    {
      auto &io = ImGui::GetIO();
      if(!io.KeyShift)
      {
        clearSelectedWidgets();
      }
      w->setSelected(true);
      fSelectedWidgets.emplace(w.get());
      clearAllWidgets = false;
    }
  }

  if(clearAllWidgets)
    clearSelectedWidgets();

  ImGui::SetCursorPos(top + ImVec2{0, fTexture->height() * iCtx.getZoom()});

  ImGui::Text("Selected widgets = %lu", fSelectedWidgets.size());
  auto &io = ImGui::GetIO();
  ImGui::Text("Shift=%s", io.KeyShift ? "true" : "false");
}

//------------------------------------------------------------------------
// PanelView::addWidget
//------------------------------------------------------------------------
int PanelView::addWidget(std::unique_ptr<Widget> iWidget)
{
  return fWidgets.add(std::move(iWidget));
}

//------------------------------------------------------------------------
// PanelView::clearSelectedWidgets
//------------------------------------------------------------------------
void PanelView::clearSelectedWidgets()
{
  for(auto w: fSelectedWidgets)
    w->setSelected(false);
  fSelectedWidgets.clear();
}

}