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
  class Data
  {
  public:
    Data(ImTextureID iImTextureID, float iHeight) : fImTextureID{iImTextureID}, fHeight{iHeight} {}
    virtual ~Data() = default;
    Data(const Data &) = delete;
    Data &operator=(const Data &) = delete;
    constexpr ImTextureID data() const { return fImTextureID; }

    friend class Texture;
    friend class TextureManager;

  protected:
    ImTextureID fImTextureID;
    float fHeight;
  };

public:
  explicit Texture(std::shared_ptr<FilmStrip> iFilmStrip) :
    fFilmStrip{std::move(iFilmStrip)}
  {
  }

  virtual ~Texture() = default;

  constexpr std::string const &key() const { return fFilmStrip->key(); };

  constexpr bool isValid() const { return fFilmStrip->isValid(); }

  constexpr float width() const { return fFilmStrip->width(); }
  constexpr float height() const { return fFilmStrip->height(); }

  constexpr int numFrames() const { return fFilmStrip->numFrames(); }
  constexpr float frameWidth() const { return fFilmStrip->frameWidth(); }
  constexpr float frameHeight() const { return fFilmStrip->frameHeight(); }
  constexpr ImVec2 frameSize() const { return {frameWidth(), frameHeight()}; }

  std::shared_ptr<FilmStrip> getFilmStrip() const { return fFilmStrip; }

  inline void Item(ImVec2 const &iPosition, float iZoom, int iFrameNumber, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const
  {
    doDraw(true, iPosition, iZoom, iFrameNumber, iBorderCol);
  }
  inline void draw(ImVec2 const &iPosition, float iZoom, int iFrameNumber, const ImVec4& iBorderCol = ImVec4(0,0,0,0)) const
  {
    doDraw(false, iPosition, iZoom, iFrameNumber, iBorderCol);
  }

  void addData(std::unique_ptr<Data> iData) { fData.emplace_back(std::move(iData)); }

protected:
  void doDraw(bool iAddItem, ImVec2 const &iPosition, float iZoom, int iFrameNumber, const ImVec4& iBorderCol) const;

protected:
  std::shared_ptr<FilmStrip> fFilmStrip{};
  std::vector<std::unique_ptr<Data>> fData{};
};

}

#endif //RE_EDIT_TEXTURE_H
