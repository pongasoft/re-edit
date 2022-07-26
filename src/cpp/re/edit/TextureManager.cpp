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

#include "Errors.h"
#include "TextureManager.h"
#include "ReGui.h"
#include "imgui_internal.h"

namespace re::edit {

//------------------------------------------------------------------------
// TextureManager::init
//------------------------------------------------------------------------
void TextureManager::init(std::string iDirectory)
{
  fFilmStripMgr = std::make_unique<FilmStripMgr>(std::move(iDirectory));
}

//------------------------------------------------------------------------
// TextureManager::getTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::getTexture(std::string const &iKey) const
{
  auto texture = findTexture(iKey);
  RE_EDIT_ASSERT(texture != nullptr, "No texture with key [%s]", iKey);
  return texture;
}

//------------------------------------------------------------------------
// TextureManager::findTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::findTexture(std::string const &iKey) const
{
  RE_EDIT_INTERNAL_ASSERT(fFilmStripMgr != nullptr);

  // get the filmstrip associated to the key
  auto filmStrip = fFilmStripMgr->findFilmStrip(iKey);

  if(!filmStrip)
    return nullptr;

  // do we already have a GPU texture associated to this path?
  auto iter = fTextures.find(iKey);
  if(iter != fTextures.end())
  {
    auto texture = iter->second;

    // same filmstrip
    if(texture->getFilmStrip() == filmStrip)
    {
      return texture; // nothing has changed
    }
  }

  // create a new texture
  fTextures[filmStrip->key()] = filmStrip->isValid() ? createTexture(filmStrip) : std::make_unique<Texture>(filmStrip);
  return fTextures.at(filmStrip->key());
}

//------------------------------------------------------------------------
// TextureManager::findHDTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::findHDTexture(std::string const &iKey) const
{
  return findTexture(iKey + "-HD");
}

//------------------------------------------------------------------------
// Texture::doDraw
//------------------------------------------------------------------------
void Texture::doDraw(bool iAddItem,
                     ImVec2 const &iPosition,
                     float iPositionZoom,
                     float iTextureZoom,
                     int iFrameNumber,
                     ImVec4 const &iBorderCol) const
{
  if(fData.empty())
    return;

  auto const cp = ImGui::GetCursorScreenPos() + iPosition * iPositionZoom;

  auto size = ImVec2{frameWidth() * iTextureZoom, frameHeight() * iTextureZoom};
  const ImRect rect{cp, cp + size};

  // do we treat the texture as a ImGui item (clickable, etc...)
  if(iAddItem)
  {
    ImGui::SetCursorScreenPos(cp);
    ImGui::ItemSize(rect);
    if(!ImGui::ItemAdd(rect, 0))
      return;
  }

  ImDrawList* drawList = ImGui::GetWindowDrawList();

  const auto frameHeight = this->frameHeight();
  const auto frameY = frameHeight * static_cast<float>(iFrameNumber);

  auto data = fData[0].get();

  if(fData.size() == 1)
  {
    // most frequent use case
    auto height = data->fHeight;
    auto uv0 = ImVec2(0, (frameY) / height);
    auto uv1 = ImVec2(1, (frameY + frameHeight) / height);
    drawList->AddImage(data->fImTextureID, rect.Min, rect.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));
  }
  else
  {
    // used only if a filmstrip had to be split into multiple textures due to GPU limitations (ex: on macOS/metal, the
    // limit is 16384 pixels for a texture height)

    // find which texture the frame starts in
    int i = 1;
    auto startY = frameY;
    while(startY > data->fHeight)
    {
      startY -= data->fHeight;
      data = fData[i++].get();
    }
    auto height = data->fHeight;
    auto endY = startY + frameHeight;
    if(endY <= height)
    {
      // case when it starts/ends in the same texture
      auto uv0 = ImVec2(0, startY / height);
      auto uv1 = ImVec2(1, endY / height);
      drawList->AddImage(data->fImTextureID, rect.Min, rect.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));
    }
    else
    {
      // case when the frame crosses the boundary of a texture (rare)
      // Implementation note: this code is assuming that a frame will be split to at most 2 textures which seems
      // reasonable since a texture max height should be far bigger than a 9U device. But it could be revisited in
      // the event this assumption changes...
      auto uv0 = ImVec2(0, startY / height);
      auto uv1 = ImVec2(1, 1);
      auto heightInData1 = height - startY;
      auto heightInData2 = frameHeight - heightInData1;
      auto fraction = heightInData1 / frameHeight;
      auto rect1 = rect;
      rect1.Max.y = (rect.Max.y - rect.Min.y) * fraction + rect.Min.y;
      drawList->AddImage(data->fImTextureID, rect1.Min, rect1.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));
      data = fData[i++].get();
      height = data->fHeight;
      uv0 = ImVec2(0, 0);
      uv1 = ImVec2(1, heightInData2 / height);
      auto rect2 = rect;
      rect2.Min.y = rect1.Max.y;
      drawList->AddImage(data->fImTextureID, rect2.Min, rect2.Max, uv0, uv1, ImGui::GetColorU32(ImVec4{1, 1, 1, 1}));
    }
  }

  // add a border?
  if(iBorderCol.w > 0.0f)
    drawList->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(iBorderCol), 0.0f);
}

}
