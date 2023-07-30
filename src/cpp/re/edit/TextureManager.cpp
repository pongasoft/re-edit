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
#include "UIContext.h"
#include <raylib.h>

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

  std::shared_ptr<Texture> texture = createTexture();
  texture->loadOnGPU(fFilmStripMgr->getFilmStrip(iKey));
  fTextures[iKey] = texture;

  return texture;
}

//------------------------------------------------------------------------
// TextureManager::loadTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> TextureManager::loadTexture(FilmStrip::key_t const &iKey, std::optional<int> iNumFrames)
{
  if(iKey.empty())
    return nullptr;

  auto texture = getTexture(iKey);
  if(iNumFrames)
  {
    auto previousNumFrames = texture->getFilmStrip()->overrideNumFrames(*iNumFrames);
    if(previousNumFrames != 0 && previousNumFrames != 1 && previousNumFrames != iNumFrames)
      RE_EDIT_LOG_WARNING("Inconsistent number of frames for %s : %d and %d", iKey, previousNumFrames, iNumFrames);
  }

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
    std::shared_ptr<Texture> texture = createTexture();
    texture->loadOnGPU(fFilmStripMgr->getFilmStrip(iKey));
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
    iter->second->loadOnGPU(fFilmStripMgr->getFilmStrip(iKey));
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
// TextureManager::createTexture
//------------------------------------------------------------------------
std::unique_ptr<Texture> TextureManager::createTexture() const
{
  // TODO remove/simplify
  return std::make_unique<Texture>();
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
// Texture::loadOnGPU
//------------------------------------------------------------------------
void Texture::loadOnGPU(std::shared_ptr<FilmStrip> const &iFilmStrip)
{
  fFilmStrip = iFilmStrip;

  if(UIContext::HasCurrent())
  {
    UIContext::GetCurrent().execute([texture = shared_from_this(), filmStrip = iFilmStrip] {
      texture->loadOnGPUFromUIThread(filmStrip);
    });
  }
}

//------------------------------------------------------------------------
// Texture::loadOnGPUFromUIThread
//------------------------------------------------------------------------
void Texture::loadOnGPUFromUIThread(std::shared_ptr<FilmStrip> const &iFilmStrip)
{
  auto const maxTextureSize = UIContext::GetCurrent().maxTextureSize();

  auto image = iFilmStrip->rlImage();
  RE_EDIT_ASSERT(image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  auto pixels = static_cast<unsigned char *>(image.data);
  auto height = image.height;

  fGPUTextures.clear();

  do
  {
    auto h = std::min(height, maxTextureSize);
    image.height = h;
    image.data = pixels;
    fGPUTextures.emplace_back(std::make_unique<RLTexture>(LoadTextureFromImage(image)));
    height -= h;
    pixels += 4 * image.width * h;
  }
  while(height != 0);
}

//------------------------------------------------------------------------
// Texture::doDraw
//------------------------------------------------------------------------
void Texture::doDraw(bool iAddItem,
                     ImVec2 const &iScreenPosition,
                     ImVec2 const &iSize,
                     int iFrameNumber,
                     ImU32 iBorderColor,
                     ImU32 iTextureColor,
                     texture::FX const &iTextureFX) const
{
  if(fGPUTextures.empty())
    return;

  auto const size = ImVec2{iSize.x == 0 ? frameWidth()  : iSize.x, iSize.y == 0 ? frameHeight() : iSize.y};

   const ReGui::Rect dest{iScreenPosition, iScreenPosition + size};

  // do we treat the texture as a ImGui item (clickable, etc...)
  if(iAddItem)
  {
    ImGui::SetCursorScreenPos(iScreenPosition);
    ImGui::ItemSize(ImRect{dest.Min, dest.Max});
    if(!ImGui::ItemAdd(ImRect{dest.Min, dest.Max}, 0))
      return;
  }

  ImDrawList* drawList = ImGui::GetWindowDrawList();

  const auto frameHeight = this->frameHeight();
  const auto frameY = frameHeight * static_cast<float>(iFrameNumber);

  auto data = fGPUTextures[0].get();

  if(fGPUTextures.size() == 1)
  {
    // most frequent use case
    ReGui::Rect source{0, frameY, 0 + frameWidth(), frameY + frameHeight};
    data->draw(!iAddItem, source, dest, iTextureColor, iTextureFX);
  }
  else
  {
    // used only if a filmstrip had to be split into multiple textures due to GPU limitations (ex: on macOS/metal, the
    // limit is 16384 pixels for a texture height)

    // find which texture the frame starts in
    int i = 1;
    auto startY = frameY;
    while(startY > static_cast<float>(data->height()))
    {
      startY -= static_cast<float>(data->height());
      data = fGPUTextures.at(i++).get();
    }
    auto height = static_cast<float>(data->height());
    auto endY = startY + frameHeight;
    if(endY <= height)
    {
      ReGui::Rect source{0, startY, 0 + frameWidth(), endY};
      data->draw(!iAddItem, source, dest, iTextureColor, iTextureFX);
    }
    else
    {
      // case when the frame crosses the boundary of a texture (rare)
      // Implementation note: this code is assuming that a frame will be split to at most 2 textures which seems
      // reasonable since a texture max height should be far bigger than a 9U device. But it could be revisited in
      // the event this assumption changes...
      auto heightInData1 = height - startY;
      auto heightInData2 = frameHeight - heightInData1;
      auto fraction = heightInData1 / frameHeight;
      auto dest1 = dest;
      dest1.Max.y = (dest.Max.y - dest.Min.y) * fraction + dest.Min.y;
      {
        ReGui::Rect source{0, startY, frameWidth(), height};
        data->draw(!iAddItem, source, dest1, iTextureColor, iTextureFX);
      }

      data = fGPUTextures.at(i++).get();
      {
        ReGui::Rect source{0, 0, frameWidth(), heightInData2};
        auto dest2 = dest;
        dest2.Min.y = dest1.Max.y;
        data->draw(!iAddItem, source, dest2, iTextureColor, iTextureFX);
      }
    }
  }

  // add a border?
  if(!ReGui::ColorIsTransparent(iBorderColor))
    drawList->AddRect(dest.Min, dest.Max, iBorderColor, 0.0f);
}

//------------------------------------------------------------------------
// Texture::RLTexture::RLTexture
//------------------------------------------------------------------------
Texture::RLTexture::RLTexture(::Texture iTexture) : fTexture(std::make_unique<::Texture>(iTexture))
{
  SetTextureFilter(*fTexture, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(*fTexture, TEXTURE_WRAP_CLAMP);
}

//------------------------------------------------------------------------
// Texture::RLTexture::~RLTexture
//------------------------------------------------------------------------
Texture::RLTexture::~RLTexture()
{
  if(fTexture)
    UnloadTexture(*fTexture);
}

//------------------------------------------------------------------------
// Texture::RLTexture::draw
//------------------------------------------------------------------------
void Texture::RLTexture::draw(bool iUseRLDraw,
                              ReGui::Rect const &iSource,
                              ReGui::Rect const &iDestination,
                              ImU32 iTextureColor,
                              texture::FX const &iTextureFX) const
{

  if(iUseRLDraw)
  {
    Rectangle source {
      iSource.Min.x,
      iSource.Min.y,
      iSource.GetWidth(),
      iSource.GetHeight()
    };
    Rectangle destination {
      iDestination.Min.x,
      iDestination.Min.y,
      iDestination.GetWidth(),
      iDestination.GetHeight()
    };
    bool useFXShader = false;

    if(iTextureFX.isFlippedX())
      source.width = -source.width;

    if(iTextureFX.isFlippedY())
      source.height = -source.height;

    if(iTextureFX.hasShaderFX())
    {
      auto contrast = static_cast<float>(iTextureFX.fContrast);
      if(contrast != 0)
      {
        if(contrast == -100.f)
          contrast = -99.f; // otherwise the formular returns 0...
        contrast = (100.0f + contrast) / 100.0f;
        contrast *= contrast;
      }
      UIContext::GetCurrent().beginFXShader(ReGui::GetColorImVec4(iTextureFX.fTint),
                                            static_cast<float>(iTextureFX.fBrightness) / 255.0f,
                                            contrast);
      useFXShader = true;
    }

    DrawTexturePro(asRLTexture(), source, destination, {}, 0, ReGui::GetRLColor(iTextureColor));

    if(useFXShader)
      UIContext::GetCurrent().endFXShader();
  }
  else
  {
    auto uv0 = ImVec2(0, iSource.Min.y / static_cast<float>(height()));
    auto uv1 = ImVec2(1, iSource.Max.y / static_cast<float>(height()));
    ImGui::GetWindowDrawList()->AddImage(asImTextureID(), iDestination.Min, iDestination.Max, uv0, uv1, iTextureColor);
  }

}

}
