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

#include "MTLTextureManager.h"
#include "Errors.h"
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
std::unique_ptr<Texture> MTLTextureManager::createTexture(std::shared_ptr<FilmStrip> const &iFilmStrip) const
{
  RE_EDIT_ASSERT(iFilmStrip->isValid());

  auto const width = iFilmStrip->width();
  auto const height = iFilmStrip->height();

  auto desc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm,
                                                          width,
                                                          height,
                                                          false);
  auto mtlTexture = fDevice->newTexture(desc);

  RE_EDIT_LOG_INFO("createTexture(%s) : %lu", iFilmStrip->key(), mtlTexture->retainCount());

  auto res = std::make_unique<MTLTexture>(iFilmStrip, mtlTexture);

  // copy the texture from memory (filmstrip) to GPU
  auto region = MTL::Region::Make2D(0, 0, width, height);
  mtlTexture->replaceRegion(region, 0, iFilmStrip->data(), 4 * width);

  return res;
}

//------------------------------------------------------------------------
// MTLTexture::MTLTexture
//------------------------------------------------------------------------
MTLTexture::MTLTexture(std::shared_ptr<FilmStrip> iFilmStrip, ImTextureID iData) : Texture{std::move(iFilmStrip), iData}
{
  auto mtlTexture = reinterpret_cast<MTL::Texture *>(fData);
  mtlTexture->retain();
  RE_EDIT_LOG_INFO("createTexture(%s) : %lu (after)", fFilmStrip->key(), mtlTexture->retainCount());
}

//------------------------------------------------------------------------
// MTLTexture::~MTLTexture
//------------------------------------------------------------------------
MTLTexture::~MTLTexture()
{
  auto mtlTexture = reinterpret_cast<MTL::Texture *>(fData);
  RE_EDIT_LOG_INFO("removeTexture(%s) : %lu", fFilmStrip->key(), mtlTexture->retainCount());
  mtlTexture->release();
  RE_EDIT_LOG_INFO("removeTexture(%s) : %lu (after)", fFilmStrip->key(), mtlTexture->retainCount());
}


}