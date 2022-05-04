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
std::shared_ptr<Texture> TextureManager::getTexture(std::string const &iPath) const
{
  // get the filmstrip associated to the path
  auto filmStrip = fFilmStripMgr.getFilmStrip(iPath);

  // do we already have a GPU texture associated to this path?
  auto iter = fTextures.find(iPath);
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
  fTextures[filmStrip->path()] = filmStrip->isValid() ? createTexture(filmStrip) : std::make_unique<Texture>(Texture{filmStrip, nullptr});
  return fTextures.at(filmStrip->path());
}

}
