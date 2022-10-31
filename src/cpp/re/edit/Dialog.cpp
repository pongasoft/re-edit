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

#include "Dialog.h"
#include "ReGui.h"
#include "Errors.h"
#include <imgui.h>

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// Dialog::Dialog
//------------------------------------------------------------------------
Dialog::Dialog(std::string iTitle) : fTitle{std::move(iTitle)},
                                     fDialogID{fmt::printf("%s###Dialog", fTitle)}
{
  RE_EDIT_INTERNAL_ASSERT(!fTitle.empty());
}

//------------------------------------------------------------------------
// Dialog::render
//------------------------------------------------------------------------
Dialog::Result Dialog::render()
{
  auto res = fNoActionResult;

  auto title = fDialogID.c_str();

  if(!ImGui::IsPopupOpen(title))
    ImGui::OpenPopup(title);

  auto needsSeparator = false;

  if(ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar))
  {
    if(fPreContentMessage)
    {
      ImGui::TextUnformatted(fPreContentMessage->c_str());
      needsSeparator = true;
    }

    for(auto &content: fContent)
    {
      if(needsSeparator)
        ImGui::Separator();
      content->render();
      needsSeparator = true;
    }

    if(fPostContentMessage)
    {
      if(needsSeparator)
        ImGui::Separator();
      ImGui::TextUnformatted(fPostContentMessage->c_str());
      needsSeparator = true;
    }

    if(needsSeparator)
      ImGui::Separator();

    auto needsSameLine = false;
    ImVec2 buttonSize{computeButtonWidth(), 0};

    for(auto &button: fButtons)
    {
      if(needsSameLine)
        ImGui::SameLine();

      if(ImGui::Button(button.fLabel.c_str(), buttonSize))
      {
        if(button.fAction)
          res = button.fAction();
        ImGui::CloseCurrentPopup();
      }
      if(button.fDefaultFocus)
        ImGui::SetItemDefaultFocus();
      needsSameLine = true;
    }

    ImGui::EndPopup();
  }

  return res;
}

//------------------------------------------------------------------------
// Dialog::text
//------------------------------------------------------------------------
Dialog &Dialog::text(std::string iText, bool iCopyToClipboard)
{
  auto content = std::make_unique<TextContent>();
  content->fText = std::move(iText);
  content->fCopyToClipboard = iCopyToClipboard;
  fContent.emplace_back(std::move(content));
  return *this;
}

//------------------------------------------------------------------------
// Dialog::button
//------------------------------------------------------------------------
Dialog &Dialog::button(std::string iLabel, Dialog::Button::action_t iAction, bool iDefaultFocus)
{
  fButtons.emplace_back(Button{std::move(iLabel), std::move(iAction), iDefaultFocus});
  return *this;
}

//------------------------------------------------------------------------
// Dialog::computeButtonWidth
//------------------------------------------------------------------------
float Dialog::computeButtonWidth() const
{
  float w{120.0f};

  for(auto &button: fButtons)
  {
    w = std::max(w, ImGui::CalcTextSize(button.fLabel.c_str()).x);
  }
  return w;
}

//------------------------------------------------------------------------
// Dialog::isOpen
//------------------------------------------------------------------------
bool Dialog::isOpen() const
{
  return ImGui::IsPopupOpen(fDialogID.c_str());
}

//------------------------------------------------------------------------
// Dialog::TextContent::render
//------------------------------------------------------------------------
void Dialog::TextContent::render()
{
  ImGui::PushID(this);
  bool copy_to_clipboard = fCopyToClipboard ? ImGui::Button(ReGui_Prefix(ReGui_Icon_Copy, "Copy to clipboard")) : false;
  if(copy_to_clipboard)
  {
    ImGui::LogToClipboard();
  }
  std::istringstream stream(fText);
  std::string line;
  while(std::getline(stream, line, '\n'))
  {
    ImGui::TextUnformatted(line.c_str());
  }
  if(copy_to_clipboard)
  {
    ImGui::LogFinish();
  }
  ImGui::PopID();
}

}