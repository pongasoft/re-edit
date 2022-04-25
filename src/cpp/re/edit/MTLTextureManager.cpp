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
#include <vector>

namespace re::edit {

std::vector<uint8_t> computeTexture(int iWidth, int iHeight)
{
  std::vector<uint8_t> res{};
  res.reserve(iWidth * iHeight * 4);

  for(int y = iHeight - 1; y >= 0; y--)
  {
    for(int x = 0; x < iWidth; x++)
    {
      res.emplace_back(static_cast<uint8_t>(255.99 * x / iWidth)); // R
      res.emplace_back(static_cast<uint8_t>(255.99 * y / iHeight)); // G
      res.emplace_back(static_cast<uint8_t>(255.99 * 0.2)); // B
      res.emplace_back(255); // alpha
    }
  }
  return res;
}

//------------------------------------------------------------------------
// MTLTextureManager::loadTexture
//------------------------------------------------------------------------
void *MTLTextureManager::loadTexture(char const *iPath)
{
  auto const width = 128;
  auto const height = 128;
  if(!fTexture)
  {
    auto desc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, false);
    auto texture = fDevice->newTexture(desc);

    auto pixels = computeTexture(width, height);

    auto region = MTL::Region::Make2D(0, 0, width, height);
    texture->replaceRegion(region, 0, pixels.data(), 4 * width);

    fTexture = texture;
  }
  return fTexture;
}

}