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
#include "logging/logging.h"
#include <algorithm>

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
TextureManager::Texture MTLTextureManager::createTexture(std::shared_ptr<FilmStrip> const &iFilmStrip, int iFrameNumber) const
{
  DCHECK_F(iFilmStrip->isValid());

  auto const width = iFilmStrip->frameWidth();
  auto const height = iFilmStrip->frameHeight();

  auto desc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm,
                                                          width,
                                                          height,
                                                          false);
  auto mtlTexture = fDevice->newTexture(desc);
  DLOG_F(INFO, "createTexture(%s) : %lu", iFilmStrip->path().c_str(), mtlTexture->retainCount());
  mtlTexture->retain();
  DLOG_F(INFO, "createTexture(%s) : %lu (after)", iFilmStrip->path().c_str(), mtlTexture->retainCount());

  auto region = MTL::Region::Make2D(0, 0, width, height);
  mtlTexture->replaceRegion(region, 0, iFilmStrip->data(iFrameNumber), 4 * width);

  return Texture{iFilmStrip, iFrameNumber, mtlTexture};
}

//------------------------------------------------------------------------
// MTLTextureManager::replaceTexture
//------------------------------------------------------------------------
TextureManager::Texture MTLTextureManager::replaceTexture(Texture const &iTexture,
                                                          std::shared_ptr<FilmStrip> const &iFilmStrip,
                                                          int iFrameNumber) const
{
  DCHECK_F(iFilmStrip->isValid());
  DCHECK_F(iTexture.fFilmStrip->frameWidth() == iFilmStrip->frameWidth());
  DCHECK_F(iTexture.fFilmStrip->frameHeight() == iFilmStrip->frameHeight());

  auto mtlTexture = reinterpret_cast<MTL::Texture *>(iTexture.fData);
  auto const width = iFilmStrip->frameWidth();
  auto const height = iFilmStrip->frameHeight();
  auto region = MTL::Region::Make2D(0, 0, width, height);
  mtlTexture->replaceRegion(region, 0, iFilmStrip->data(iFrameNumber), 4 * width);

  return Texture{iFilmStrip, iFrameNumber, mtlTexture};
}

//------------------------------------------------------------------------
// MTLTextureManager::removeTexture
//------------------------------------------------------------------------
void MTLTextureManager::removeTexture(Texture const &iTexture) const
{
  auto mtlTexture = reinterpret_cast<MTL::Texture *>(iTexture.fData);
  DLOG_F(INFO, "removeTexture(%s) : %lu", iTexture.fFilmStrip->path().c_str(), mtlTexture->retainCount());
  mtlTexture->release();
  DLOG_F(INFO, "removeTexture(%s) : %lu (after)", iTexture.fFilmStrip->path().c_str(), mtlTexture->retainCount());
}


}