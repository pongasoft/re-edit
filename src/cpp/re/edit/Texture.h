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
#include "fx.h"

namespace re::edit {

class Texture : public std::enable_shared_from_this<Texture>
{
public:
  using key_t = FilmStrip::key_t;

public:
  struct RLTexture
  {
    explicit RLTexture(::Texture iTexture);
    RLTexture(RLTexture &&iOther) noexcept : fTexture{std::move(iOther.fTexture)} {}
    ~RLTexture();

    inline ImTextureID asImTextureID() const { return static_cast<ImTextureID>(fTexture.get()); }
    inline ::Texture asRLTexture() const { return *fTexture; }
    inline int height() const { return fTexture->height; }

    void draw(bool iUseRLDraw,
              ReGui::Rect const &iSource,
              ReGui::Rect const &iDestination,
              ImU32 iTextureColor,
              texture::FX const &iTextureFX) const;

  private:
    std::unique_ptr<::Texture> fTexture;
  };

  struct RLRenderTexture
  {
    RLRenderTexture() = default;
    RLRenderTexture(int iWidth, int iHeight);
    RLRenderTexture(RLRenderTexture &&iOther) noexcept = default;
    ~RLRenderTexture();
    RLRenderTexture &operator=(RLRenderTexture &&iOther) noexcept = default;

    inline ImTextureID asImTextureID() const { return static_cast<ImTextureID>(&fTexture->texture); }
    inline ::RenderTexture asRLRenderTexture() const { return *fTexture; }

    inline int width() const { return fTexture->texture.width; }
    inline int height() const { return fTexture->texture.height; }

    inline bool isValid() const { return fTexture != nullptr; }

  private:
    std::unique_ptr<::RenderTexture> fTexture;
  };

  struct RenderTexture
  {
    RenderTexture() = default;

    inline bool isValid() const { return fRLRenderTexture.isValid(); }
    constexpr ImVec2 const &size() const {  return fSize; }
    constexpr ImVec2 const &scale() const {  return fScale; }

    inline ImVec2 renderSize() const { return fSize * fScale; }

    inline ImVec2 rlTextureSize() const { return {static_cast<float>(rlTextureWidth()), static_cast<float>(rlTextureHeight())}; }

    void resize(ImVec2 const &iSize, ImVec2 const &iScale);

    inline ImTextureID asImTextureID() const { return fRLRenderTexture.asImTextureID(); }
    inline ::RenderTexture asRLRenderTexture() const { return fRLRenderTexture.asRLRenderTexture(); }

  private:
    inline int rlTextureWidth() const { return fRLRenderTexture.width(); }
    inline int rlTextureHeight() const { return fRLRenderTexture.height(); }

  private:
    RLRenderTexture fRLRenderTexture;
    ImVec2 fSize;
    ImVec2 fScale;
  };

public:
  Texture() = default;
  virtual ~Texture() = default;

  inline key_t const &key() const { return fFilmStrip->key(); };
  key_t computeKey(texture::FX const &iEffects) const { return fFilmStrip->computeKey(iEffects); }

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
    doDraw(true, ImGui::GetCursorScreenPos(), iSize, iFrameNumber, iBorderColor, iTextureColor, texture::kDefaultFX);
  }

  void ItemFit(ImVec2 const &iSize,
               int iFrameNumber = 0,
               ImU32 iBorderColor = ReGui::kTransparentColorU32,
               ImU32 iTextureColor = ReGui::kWhiteColorU32) const;

  inline void draw(ImVec2 const &iScreenPosition,
                   ImVec2 const &iSize,
                   int iFrameNumber,
                   ImU32 iBorderColor,
                   ImU32 iTextureColor,
                   texture::FX const &iTextureFX) const
  {
    doDraw(false, iScreenPosition, iSize, iFrameNumber, iBorderColor, iTextureColor, iTextureFX);
  }

  void loadOnGPU(const std::shared_ptr<FilmStrip>& iFilmStrip);

  void loadOnGPUFromUIThread(std::shared_ptr<FilmStrip> const &iFilmStrip);

  void unloadFromGPU() { fGPUTextures.clear(); }

  friend class TextureManager;

protected:
  void doDraw(bool iAddItem,
              ImVec2 const &iScreenPosition,
              ImVec2 const &iSize,
              int iFrameNumber,
              ImU32 iBorderColor,
              ImU32 iTextureColor,
              texture::FX const &iTextureFX) const;

//  void reloadOnGPU() const { doLoadOnGPU(fFilmStrip); }

protected:
  std::shared_ptr<FilmStrip> fFilmStrip{};
  mutable std::vector<std::unique_ptr<RLTexture>> fGPUTextures{};
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
