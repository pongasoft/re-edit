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
void TextureManager::init(std::vector<BuiltIns::Def> const &iBuiltIns, std::optional<fs::path> iDirectory)
{
  fFilmStripMgr = std::make_unique<FilmStripMgr>(iBuiltIns, std::move(iDirectory));
}

//------------------------------------------------------------------------
// TextureManager::getTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::getTexture(std::string const &iKey) const
{
  if(iKey.empty())
    return nullptr;

  auto iter = fTextures.find(iKey);
  if(iter != fTextures.end())
    return iter->second;

  std::shared_ptr<Texture> texture = updateTexture(createTexture(), fFilmStripMgr->getFilmStrip(iKey));
  fTextures[iKey] = texture;
  return texture;
}

//------------------------------------------------------------------------
// TextureManager::getTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::findTexture(std::string const &iKey) const
{
  auto iter = fTextures.find(iKey);
  if(iter != fTextures.end())
    return iter->second;

  auto filmStrip = fFilmStripMgr->findFilmStrip(iKey);
  if(filmStrip)
  {
    std::shared_ptr<Texture> texture = updateTexture(createTexture(), filmStrip);
    fTextures[iKey] = texture;
    return texture;
  }
  else
    return nullptr;
}

//------------------------------------------------------------------------
// TextureManager::findHDTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::findHDTexture(std::string const &iKey) const
{
  return findTexture(iKey + "-HD");
}

//------------------------------------------------------------------------
// TextureManager::scanDirectory
//------------------------------------------------------------------------
void TextureManager::scanDirectory()
{
  auto keys = fFilmStripMgr->scanDirectory();
  std::for_each(keys.begin(), keys.end(), [this](auto const &k) { updateTexture(k); });
}

//------------------------------------------------------------------------
// TextureManager::importTexture
//------------------------------------------------------------------------
std::optional<FilmStrip::key_t> TextureManager::importTexture(fs::path const &iTexturePath)
{
  auto key = fFilmStripMgr->importTexture(iTexturePath);
  if(key)
  {
    updateTexture(*key);
    return key;
  }
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// TextureManager::importBuiltIns
//------------------------------------------------------------------------
void TextureManager::importBuiltIns(std::set<FilmStrip::key_t> const &iKeys, UserError *oErrors)
{
  auto keys = fFilmStripMgr->importBuiltIns(iKeys, oErrors);
  std::for_each(keys.begin(), keys.end(), [this](auto const &k) { updateTexture(k); });
}


//------------------------------------------------------------------------
// TextureManager::updateTexture
//------------------------------------------------------------------------
void TextureManager::updateTexture(FilmStrip::key_t const &iKey)
{
  auto iter = fTextures.find(iKey);
  if(iter != fTextures.end())
  {
    updateTexture(iter->second, fFilmStripMgr->getFilmStrip(iKey));
  }
}

//------------------------------------------------------------------------
// TextureManager::updateTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::updateTexture(std::shared_ptr<Texture> const &iTexture,
                                                       std::shared_ptr<FilmStrip> const &iFilmStrip) const
{
  iTexture->fFilmStrip = iFilmStrip;
  iTexture->clearData();

  if(iFilmStrip->isValid())
    populateTexture(iTexture);

  return iTexture;
}

//------------------------------------------------------------------------
// TextureManager::overrideNumFrames
//------------------------------------------------------------------------
void TextureManager::overrideNumFrames(std::map<std::string, int> const &iNumFrames) const
{
  for(auto &[k, numFrames]: iNumFrames)
  {
    getTexture(k)->fFilmStrip->overrideNumFrames(numFrames);
  }
}

//------------------------------------------------------------------------
// TextureManager::overrideNumFrames
//------------------------------------------------------------------------
int TextureManager::overrideNumFrames(std::string const &iKey, int iNumFrames) const
{
  return getTexture(iKey)->fFilmStrip->overrideNumFrames(iNumFrames);
}

//------------------------------------------------------------------------
// Texture::ItemFit
//------------------------------------------------------------------------
void Texture::ItemFit(ImVec2 const &iSize, int iFrameNumber, ImU32 iBorderColor, ImU32 iTextureColor) const
{
  auto size = frameSize();
  auto scaleX = iSize.x == 0 ? 1.0f: std::min(iSize.x, size.x) / size.x;
  auto scaleY = iSize.y == 0 ? 1.0f: std::min(iSize.y, size.y) / size.y;

  auto scale = std::min(scaleX, scaleY);

  Item(size * scale, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// Texture::doDraw
//------------------------------------------------------------------------
void Texture::doDraw(bool iAddItem,
                     ImVec2 const &iScreenPosition,
                     ImVec2 const &iSize,
                     int iFrameNumber,
                     ImU32 iBorderColor,
                     ImU32 iTextureColor) const
{
  if(fData.empty())
    return;

  auto const size = ImVec2{iSize.x == 0 ? frameWidth()  : iSize.x, iSize.y == 0 ? frameHeight() : iSize.y};

  const ImRect rect{iScreenPosition, iScreenPosition + size};

  // do we treat the texture as a ImGui item (clickable, etc...)
  if(iAddItem)
  {
    ImGui::SetCursorScreenPos(iScreenPosition);
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
    drawList->AddImage(data->fImTextureID, rect.Min, rect.Max, uv0, uv1, iTextureColor);
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
      drawList->AddImage(data->fImTextureID, rect.Min, rect.Max, uv0, uv1, iTextureColor);
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
      drawList->AddImage(data->fImTextureID, rect1.Min, rect1.Max, uv0, uv1, iTextureColor);
      data = fData[i++].get();
      height = data->fHeight;
      uv0 = ImVec2(0, 0);
      uv1 = ImVec2(1, heightInData2 / height);
      auto rect2 = rect;
      rect2.Min.y = rect1.Max.y;
      drawList->AddImage(data->fImTextureID, rect2.Min, rect2.Max, uv0, uv1, iTextureColor);
    }
  }

  // add a border?
  if(!ReGui::ColorIsTransparent(iBorderColor))
    drawList->AddRect(rect.Min, rect.Max, iBorderColor, 0.0f);
}

}
