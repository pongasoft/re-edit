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

#include "DrawContext.h"
#include "imgui_internal.h"

namespace re::edit {

//------------------------------------------------------------------------
// DrawContext::TextureItem
//------------------------------------------------------------------------
void DrawContext::TextureItem(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  if(iTexture->isValid())
  {
    auto const cp = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(cp + iPosition * fZoom);

    auto frameWidth = iTexture->frameWidth();
    auto frameHeight = iTexture->frameHeight();
    auto frameY = frameHeight * static_cast<float>(iFrameNumber);
    auto width = iTexture->width();
    auto height = iTexture->height();

    Image(iTexture->data(),
          { frameWidth * fZoom, frameHeight * fZoom},
          ImVec2(0, (frameY) / height),
          ImVec2((0 + frameWidth) / width, (frameY + frameHeight) / height),
          iBorderCol
    );
  }
}

//------------------------------------------------------------------------
// DrawContext::drawTexture
//------------------------------------------------------------------------
void DrawContext::drawTexture(Texture const *iTexture, ImVec2 const &iPosition, int iFrameNumber, const ImVec4& iBorderCol) const
{
  if(iTexture->isValid())
  {
    auto frameWidth = iTexture->frameWidth();
    auto frameHeight = iTexture->frameHeight();
    auto frameY = frameHeight * static_cast<float>(iFrameNumber);
    auto width = iTexture->width();
    auto height = iTexture->height();

    auto position = iPosition * fZoom;
    auto size = ImVec2{frameWidth * fZoom, frameHeight * fZoom};

    drawImage(iTexture->data(),
              position,
              size,
              ImVec2(0, (frameY) / height),
              ImVec2((0 + frameWidth) / width, (frameY + frameHeight) / height),
              iBorderCol
    );
  }
}

//------------------------------------------------------------------------
// DrawContext::drawRect
//------------------------------------------------------------------------
void DrawContext::drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImVec4 const &iColor) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, ImGui::GetColorU32(iColor));
}

//------------------------------------------------------------------------
// DrawContext::Image
//------------------------------------------------------------------------
void DrawContext::Image(ImTextureID user_texture_id,
                        ImVec2 const &size,
                        ImVec2 const &uv0,
                        ImVec2 const &uv1,
                        ImVec4 const &border_col)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if(window->SkipItems)
    return;

  ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
  ImGui::ItemSize(bb);
  if(!ImGui::ItemAdd(bb, 0))
    return;

  window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));

  if(border_col.w > 0.0f)
    window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
}

//------------------------------------------------------------------------
// DrawContext::drawImage
//------------------------------------------------------------------------
void DrawContext::drawImage(ImTextureID user_texture_id,
                            ImVec2 const &iPosition,
                            ImVec2 const &size,
                            ImVec2 const &uv0,
                            ImVec2 const &uv1,
                            ImVec4 const &border_col)
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  ImRect rect{cp + iPosition, cp + iPosition + size};
  drawList->AddImage(user_texture_id, rect.Min, rect.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));
  if(border_col.w > 0.0f)
    drawList->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(border_col), 0.0f);
}

}