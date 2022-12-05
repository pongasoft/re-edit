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

#include "MTLManagers.h"
#include "../../Errors.h"
#include "backends/imgui_impl_metal.h"
#include <algorithm>
#include <memory>

namespace re::edit {

//std::vector<uint8_t> computeTexture(int iWidth, int iHeight)
//{
//  std::vector<uint8_t> res{};
//  res.reserve(iWidth * iHeight * 4);
//
//  for(int y = iHeight - 1; y >= 0; y--)
//  {
//    for(int x = 0; x < iWidth; x++)
//    {
//      res.emplace_back(static_cast<uint8_t>(255.99 * x / iWidth)); // R
//      res.emplace_back(static_cast<uint8_t>(255.99 * y / iHeight)); // G
//      res.emplace_back(static_cast<uint8_t>(255.99 * 0.2)); // B
//      res.emplace_back(255); // alpha
//    }
//  }
//  return res;
//}

//------------------------------------------------------------------------
// MTLTextureManager::MTLTextureManager
//------------------------------------------------------------------------
MTLTextureManager::MTLTextureManager(MTL::Device *iDevice) :
  TextureManager(), fDevice{iDevice} {}

//------------------------------------------------------------------------
// MTLTextureManager::createTexture
//------------------------------------------------------------------------
std::unique_ptr<Texture> MTLTextureManager::createTexture() const
{
  return std::make_unique<MTLTexture>();
}

//------------------------------------------------------------------------
// MTLTextureManager::populateTexture
//------------------------------------------------------------------------
void MTLTextureManager::populateTexture(std::shared_ptr<Texture> const &iTexture) const
{
  RE_EDIT_ASSERT(iTexture->isValid());

  auto filmStrip = iTexture->getFilmStrip();

  auto const width = filmStrip->width();
  auto height = filmStrip->height();

  auto pixels = filmStrip->data();

  do
  {
    auto h = std::min(height, MTLTexture::kMaxTextureHeight);

    auto desc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm,
                                                            width,
                                                            h,
                                                            false);

    auto mtlData = std::make_unique<MTLTexture::MTLData>(fDevice->newTexture(desc), h);

    // copy the texture from memory (filmstrip) to GPU
    auto region = MTL::Region::Make2D(0, 0, width, h);
    mtlData->getMTLTexture()->replaceRegion(region, 0, pixels, 4 * width);

    iTexture->addData(std::move(mtlData));

    height -= h;
    pixels += 4 * width * h;
  }
  while(height != 0);

}

//------------------------------------------------------------------------
// MTLTexture::MTLData::MTLData
//------------------------------------------------------------------------
MTLTexture::MTLData::MTLData(ImTextureID iImTextureID, float iHeight) : Data(iImTextureID, iHeight)
{
  getMTLTexture()->retain();
}

//------------------------------------------------------------------------
// MTLTexture::MTLData::MTLData
//------------------------------------------------------------------------
MTLTexture::MTLData::~MTLData()
{
  getMTLTexture()->release();
}

//------------------------------------------------------------------------
// MTLFontManager::createFontsTexture
//------------------------------------------------------------------------
void MTLFontManager::createFontsTexture()
{
  ImGui_ImplMetal_CreateFontsTexture(fDevice);
}

//------------------------------------------------------------------------
// MTLFontManager::destroyFontsTexture
//------------------------------------------------------------------------
void MTLFontManager::destroyFontsTexture()
{
  ImGui_ImplMetal_DestroyFontsTexture();
}

}