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

#ifndef RE_EDIT_TEXTURE_MANAGER_H
#define RE_EDIT_TEXTURE_MANAGER_H

#include "FilmStrip.h"
#include <map>
#include <memory>

namespace re::edit {

class TextureManager
{
public:
  struct Texture
  {
    std::shared_ptr<FilmStrip> fFilmStrip{};
    int fFrameNumber{};
    void *fData{};
  };

public:

  TextureManager() = default;
  virtual ~TextureManager() = default;

  Texture getTexture(std::string const &iPath, int iFrameNumber = 0) const;

  bool loadFilmStrip(char const *iPath, int iNumFrames = 1) { return fFilmStripMgr.maybeAddFilmStrip(iPath, iNumFrames); }

protected:
  virtual Texture createTexture(std::shared_ptr<FilmStrip> const &iFilmStrip, int iFrameNumber) const = 0;
  virtual Texture replaceTexture(Texture const &iTexture, std::shared_ptr<FilmStrip> const &iFilmStrip, int iFrameNumber) const = 0;
  virtual void removeTexture(Texture const &iTexture) const = 0;

private:
  FilmStripMgr fFilmStripMgr{};

  mutable std::map<std::string, Texture> fTextures{};
};

}

#endif //RE_EDIT_TEXTURE_MANAGER_H