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

#include "TextureManager.h"

namespace re::edit {

//------------------------------------------------------------------------
// TextureManager::getTexture
//------------------------------------------------------------------------
TextureManager::Texture TextureManager::getTexture(std::string const &iPath, int iFrameNumber) const
{
  // get the filmstrip associated to the path
  auto filmStrip = fFilmStripMgr.getFilmStrip(iPath);

  if(!filmStrip->isValid())
  {
    return Texture{filmStrip, iFrameNumber, nullptr};
  }

  // do we already have a GPU texture associated to this path?
  auto iter = fTextures.find(iPath);
  if(iter != fTextures.end())
  {
    auto texture = iter->second;
    if(texture.fFilmStrip == filmStrip)
    {
      // same filmstrip
      if(texture.fFrameNumber == iFrameNumber)
        return texture; // nothing has changed
      else
      {
        // different frame => replace texture
        texture = replaceTexture(texture, filmStrip, iFrameNumber);
        fTextures[filmStrip->path()] = texture;
        return texture;
      }
    }

    if(texture.fFilmStrip->width() == filmStrip->width() &&
       texture.fFilmStrip->height() == filmStrip->height())
    {
      // different filmstrip but same size => no need to recreate the texture
      texture = replaceTexture(texture, filmStrip, iFrameNumber);
      fTextures[filmStrip->path()] = texture;
      return texture;
    }
    else
    {
      // need a new texture
      removeTexture(texture);
    }
  }

  // create a new texture
  auto texture = createTexture(filmStrip, iFrameNumber);
  fTextures[filmStrip->path()] = texture;
  return texture;
}

}
