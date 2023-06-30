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
#include "ReGui.h"
#include "Errors.h"

namespace re::edit {

class Texture
{
public:
  using key_t = FilmStrip::key_t;

public:
  class GPUData
  {
  public:
    GPUData(ImTextureID iImTextureID, float iHeight) : fImTextureID{iImTextureID}, fHeight{iHeight} {}
    virtual ~GPUData() = default;
    GPUData(const GPUData &) = delete;
    GPUData &operator=(const GPUData &) = delete;
    constexpr ImTextureID data() const { return fImTextureID; }

    friend class Texture;
    friend class TextureManager;

  protected:
    ImTextureID fImTextureID;
    float fHeight;
  };

public:
  Texture() = default;
  virtual ~Texture() = default;

  inline key_t const &key() const { return fFilmStrip->key(); };

  inline bool isValid() const { return fFilmStrip->isValid(); }

  constexpr float width() const { return static_cast<float>(fFilmStrip->width()); }
  constexpr float height() const { return static_cast<float>(fFilmStrip->height()); }

  constexpr int numFrames() const { return fFilmStrip->numFrames(); }
  constexpr float frameWidth() const { return static_cast<float>(fFilmStrip->frameWidth()); }
  constexpr float frameHeight() const { return static_cast<float>(fFilmStrip->frameHeight()); }
  constexpr ImVec2 frameSize() const { return isValid() ? ImVec2{frameWidth(), frameHeight()} : kNoGraphics; }

  std::shared_ptr<FilmStrip> getFilmStrip() const { return fFilmStrip; }

  inline void Item(ImVec2 const &iSize = {},
                   int iFrameNumber = 0,
                   ImU32 iBorderColor = ReGui::kTransparentColorU32,
                   ImU32 iTextureColor = ReGui::kWhiteColorU32) const
  {
    doDraw(true, ImGui::GetCursorScreenPos(), iSize, iFrameNumber, iBorderColor, iTextureColor);
  }

  void ItemFit(ImVec2 const &iSize,
               int iFrameNumber = 0,
               ImU32 iBorderColor = ReGui::kTransparentColorU32,
               ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  inline void draw(ImVec2 const &iScreenPosition,
                   ImVec2 const &iSize = {},
                   int iFrameNumber = 0,
                   ImU32 iBorderColor = ReGui::kTransparentColorU32,
                   ImU32 iTextureColor = ReGui::kWhiteColorU32) const
  {
    doDraw(false, iScreenPosition, iSize, iFrameNumber, iBorderColor, iTextureColor);
  }

  void loadOnGPU(std::shared_ptr<FilmStrip> iFilmStrip);

  friend class TextureManager;

protected:
  virtual void doLoadOnGPU(std::shared_ptr<FilmStrip> const &iFilmStrip) const = 0;

  void doDraw(bool iAddItem,
              ImVec2 const &iScreenPosition,
              ImVec2 const &iSize,
              int iFrameNumber,
              ImU32 iBorderColor,
              ImU32 iTextureColor) const;

//  void reloadOnGPU() const { doLoadOnGPU(fFilmStrip); }

protected:
  std::shared_ptr<FilmStrip> fFilmStrip{};
  mutable std::vector<std::unique_ptr<GPUData>> fGPUData{};
};

struct Icon
{
  std::shared_ptr<Texture> fTexture{};
  int fFrameNumber{};

  inline void Item(ImVec2 const &iSize = {},
                   ImU32 iBorderColor = ReGui::kTransparentColorU32,
                   ImU32 iTextureColor = ReGui::kWhiteColorU32) const
  {
    fTexture->Item(iSize, fFrameNumber, iBorderColor, iTextureColor);
  }
};

}

#endif //RE_EDIT_TEXTURE_H
