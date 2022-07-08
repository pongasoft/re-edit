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
// TextureManager::getTexture
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
  fTextures[filmStrip->key()] = filmStrip->isValid() ? createTexture(filmStrip) : std::make_unique<Texture>(Texture{filmStrip, nullptr});
  return fTextures.at(filmStrip->key());
}

}
