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

#include "Notification.h"
#include "Constants.h"
#include "ReGui.h"

#include <utility>

namespace re::edit::ReGui {

//------------------------------------------------------------------------
// Notification::Notification
//------------------------------------------------------------------------
Notification::Notification(Key const &iKey) :
  fKey{iKey},
  fWindowName{fmt::printf("Notification_%d", kIota++)}
{
}

//------------------------------------------------------------------------
// Notification::render
//------------------------------------------------------------------------
bool Notification::render()
{
  static ImVec2 kButtonSize{};
  static ImGuiOnceUponAFrame kOaf{};

  if(fDismissTime && std::chrono::system_clock::now() >= *fDismissTime)
  {
    dismiss();
  }

  if(!isActive())
    return false;

  if(kOaf)
    kButtonSize = ImGui::CalcTextSize(ReGui::kResetIcon);

  bool rendered = false;

  if(ImGui::Begin(fWindowName.c_str(), nullptr,   ImGuiWindowFlags_NoTitleBar
                                                | ImGuiWindowFlags_NoResize
                                                | ImGuiWindowFlags_NoCollapse
                                                | ImGuiWindowFlags_NoDocking
                                                | ImGuiWindowFlags_NoSavedSettings))
  {
    auto available = ImGui::GetContentRegionAvail() - ImGui::GetStyle().WindowPadding;
    auto cp = ImGui::GetCursorPos();
    auto resetButtonX = available.x - kButtonSize.x;
    ImGui::PushTextWrapPos(resetButtonX);
    for(auto &content: fContent)
    {
      if(!content->render())
        dismiss();
    }
    ImGui::PopTextWrapPos();
    ImGui::SetCursorPos({resetButtonX, cp.y});
    if(ImGui::Button(ReGui::kResetIcon))
    {
      dismiss();
    }
    rendered = true;
  }
  ImGui::End();

  return rendered;
}

//------------------------------------------------------------------------
// Notification::lambda
//------------------------------------------------------------------------
Notification &Notification::lambda(std::function<bool()> iLambda)
{
  if(iLambda)
  {
    auto content = std::make_unique<LamdaContent>();
    content->fLambda = std::move(iLambda);
    fContent.emplace_back(std::move(content));
  }
  return *this;
}

//------------------------------------------------------------------------
// Notification::text
//------------------------------------------------------------------------
Notification &Notification::text(std::string iText)
{
  return lambda([text = std::move(iText)]() { ReGui::MultiLineText(text); return true; });
}

//------------------------------------------------------------------------
// Notification::LamdaContent::render
//------------------------------------------------------------------------
bool Notification::LamdaContent::render()
{
  return fLambda();
}

}