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

#ifndef RE_EDIT_TEXTURE_H
#define RE_EDIT_TEXTURE_H

#include <imgui.h>
#include "FilmStrip.h"

namespace re::edit {

class Texture
{
public:
  Texture(std::shared_ptr<FilmStrip> iFilmStrip, ImTextureID iData) :
    fFilmStrip{std::move(iFilmStrip)},
    fData{iData}
  {
  }

  virtual ~Texture() = default;

  constexpr bool isValid() const { return fFilmStrip->isValid(); }

  constexpr float width() const { return fFilmStrip->width(); }
  constexpr float height() const { return fFilmStrip->height(); }

  constexpr int numFrames() const { return fFilmStrip->numFrames(); }
  constexpr float frameWidth() const { return fFilmStrip->frameWidth(); }
  constexpr float frameHeight() const { return fFilmStrip->frameHeight(); }

  constexpr ImTextureID data() const { return fData; }

  std::shared_ptr<FilmStrip> getFilmStrip() const { return fFilmStrip; }

protected:
  std::shared_ptr<FilmStrip> fFilmStrip{};
  ImTextureID fData{};
};

}

#endif //RE_EDIT_TEXTURE_H
