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

#include "Graphics.h"
#include "WidgetAttribute.hpp"
#include "Errors.h"
#include "AppContext.hpp"
#include "stl.h"

namespace re::edit {

//------------------------------------------------------------------------
// HitBoundaries::operator==
//------------------------------------------------------------------------
bool operator==(HitBoundaries const &lhs, HitBoundaries const &rhs)
{
  return lhs.fLeftInset == rhs.fLeftInset &&
         lhs.fTopInset == rhs.fTopInset &&
         lhs.fRightInset == rhs.fRightInset &&
         lhs.fBottomInset == rhs.fBottomInset;
}

//------------------------------------------------------------------------
// HitBoundaries::operator!=
//------------------------------------------------------------------------
bool operator!=(HitBoundaries const &lhs, HitBoundaries const &rhs)
{
  return !(rhs == lhs);
}

namespace impl {

//------------------------------------------------------------------------
// graphics_device2D_path
//------------------------------------------------------------------------
std::string graphics_device2D_path(Texture const *iTexture, texture::FX const &iEffects)
{
  std::string path{};

  if(iTexture)
  {
    if(iEffects.hasAny())
    {
      auto tint = ReGui::GetJboxColor3(iEffects.fTint);
      auto sizeOverride =
        iEffects.hasSizeOverride() && iEffects.fSizeOverride != iTexture->frameSize() ?
        re::mock::fmt::printf(", re_edit_size = { %d, %d }",
                              stl::roundToInt(iEffects.fSizeOverride->x),
                              stl::roundToInt(iEffects.fSizeOverride->y)) :
        "";

      path = re::mock::fmt::printf(R"(path = "%s"%s, re_edit_path = "%s"%s%s%s%s%s)",
                                   iTexture->computeKey(iEffects),
                                   iTexture->numFrames() > 1 ?  re::mock::fmt::printf(", frames = %d", iTexture->numFrames()) : "",
                                   iTexture->key(),
                                   iEffects.hasTint() ? re::mock::fmt::printf(", re_edit_tint = { %d, %d, %d }", tint.fRed, tint.fGreen, tint.fBlue) : "",
                                   iEffects.hasBrightness() ? re::mock::fmt::printf(", re_edit_brightness = %d", iEffects.fBrightness) : "",
                                   iEffects.isFlippedX() ? ", re_edit_flip_x = true" : "",
                                   iEffects.isFlippedY() ? ", re_edit_flip_y = true" : "",
                                   sizeOverride
      );
    }
    else
    {
      path = re::mock::fmt::printf(R"(path = "%s"%s)",
                                   iTexture->key(),
                                   iTexture->numFrames() > 1 ?  re::mock::fmt::printf(", frames = %d", iTexture->numFrames()) : "");
    }
  }

  return path;
}

//------------------------------------------------------------------------
// collectUsedTexturePaths
//------------------------------------------------------------------------
static void collectUsedTexturePaths(Texture const *iTexture, texture::FX const &iEffects, std::set<fs::path> &oPaths)
{
  // Implementation note: this API is used to generate cmake includes. When there is an effect
  // applied, we do not include the original image
  if(iTexture)
  {
    auto filmStrip = iTexture->getFilmStrip();
    if(filmStrip)
    {
      if(iEffects.hasAny())
      {
        auto texture = AppContext::GetCurrent().findTexture(filmStrip->computeKey(iEffects));
        if(texture)
          filmStrip = texture->getFilmStrip();
      }

      if(filmStrip && filmStrip->isValid() && filmStrip->hasPath())
        oPaths.emplace(filmStrip->path());
    }
  }
}

//------------------------------------------------------------------------
// collectAllUsedTextureKeys
//------------------------------------------------------------------------
static void collectAllUsedTextureKeys(Texture const *iTexture, texture::FX const &iEffects, std::set<FilmStrip::key_t> &oKeys)
{
  // Implementation note: this API is used to determine which textures are unused. As a result we include both
  // the image with effect and the original image (where there is an effect applied)
  if(iTexture)
  {
    auto filmStrip = iTexture->getFilmStrip();
    if(filmStrip)
    {
      if(filmStrip->isValid() && filmStrip->hasPath())
        oKeys.emplace(filmStrip->key());

      if(iEffects.hasAny())
      {
        auto texture = AppContext::GetCurrent().findTexture(filmStrip->computeKey(iEffects));
        if(texture)
        {
          filmStrip = texture->getFilmStrip();
          if(filmStrip && filmStrip->isValid() && filmStrip->hasPath())
            oKeys.emplace(filmStrip->key());
        }
      }
    }
  }
}

//------------------------------------------------------------------------
// editEffects
//------------------------------------------------------------------------
static void editEffects(texture::FX &iEffects,
                        float iItemWidth,
                        float iOffset,
                        std::function<void(char const *iName, texture::FX const &fx, MergeKey const &iMergeKey)> const &iOnFXUpdate)
{
  // tint
  ImGui::PushID("tint");
  {
    if(ReGui::ResetButton())
    {
      auto fx = iEffects;
      fx.fTint = kDefaultTintColor;
      iOnFXUpdate("tint", fx, MergeKey::from(&iEffects.fTint));
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(iItemWidth - (ImGui::GetCursorPosX() - iOffset));
    ImVec4 tint = ReGui::GetColorImVec4(iEffects.fTint);
    if(ImGui::ColorEdit3("tint", &tint.x))
    {
      auto fx = iEffects;
      fx.fTint = ReGui::GetColorU32(tint);
      iOnFXUpdate("tint", fx, MergeKey::from(&iEffects.fTint));
    }
    ImGui::PopItemWidth();
  }
  ImGui::PopID();

  // brightness
  ImGui::PushID("brightness");
  {
    if(ReGui::ResetButton())
    {
      auto fx = iEffects;
      fx.fBrightness = kDefaultBrightness;
      iOnFXUpdate("brightness", fx, MergeKey::from(&iEffects.fBrightness));
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(iItemWidth - (ImGui::GetCursorPosX() - iOffset));
    auto brightness = iEffects.fBrightness;
    if(ImGui::SliderInt("brightness", &brightness, -255, 255))
    {
      auto fx = iEffects;
      fx.fBrightness = std::clamp(brightness, -255, 255);
      iOnFXUpdate("brightness", fx, MergeKey::from(&iEffects.fBrightness));
    }
    ImGui::PopItemWidth();
  }
  ImGui::PopID();

  // contrast
  ImGui::PushID("contrast");
  {
    if(ReGui::ResetButton())
    {
      auto fx = iEffects;
      fx.fContrast = kDefaultContrast;
      iOnFXUpdate("contrast", fx, MergeKey::from(&iEffects.fContrast));
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(iItemWidth - (ImGui::GetCursorPosX() - iOffset));
    auto contrast = iEffects.fContrast;
    if(ImGui::SliderInt("contrast", &contrast, -100, 100))
    {
      auto fx = iEffects;
      fx.fContrast = std::clamp(contrast, -100, 100);
      iOnFXUpdate("contrast", fx, MergeKey::from(&iEffects.fContrast));
    }
    ImGui::PopItemWidth();
  }
  ImGui::PopID();

  // flip
  ImGui::PushID("flip");
  {
    if(ReGui::ResetButton())
    {
      auto fx = iEffects;
      fx.fFlipX = false;
      fx.fFlipY = false;
      iOnFXUpdate("flip", fx, MergeKey::from(&iEffects.fFlipX));
    }
    ImGui::SameLine();
    auto flipX = iEffects.fFlipX;
    if(ImGui::Checkbox("horizontal flip", &flipX))
    {
      auto fx = iEffects;
      fx.fFlipX = flipX;
      iOnFXUpdate("horizontal flip", fx, MergeKey::from(&iEffects.fFlipX));
    }
    ImGui::SameLine();
    auto flipY = iEffects.fFlipY;
    if(ImGui::Checkbox("vertical flip", &flipY))
    {
      auto fx = iEffects;
      fx.fFlipY = flipY;
      iOnFXUpdate("vertical flip", fx, MergeKey::from(&iEffects.fFlipY));
    }
  }
  ImGui::PopID();
}

}

}

namespace re::edit::panel {

//------------------------------------------------------------------------
// Graphics::device2D
//------------------------------------------------------------------------
std::string Graphics::device2D() const
{
  std::string path{};

  if(hasTexture())
  {
    path = impl::graphics_device2D_path(fDNZTexture.get(), fEffects);
  }

  return fmt::printf("{ { %s } }", path);
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fTextureKey = "";
  fDNZTexture = nullptr;
  fEffects = {};
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(AppContext &iCtx)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  if(ImGui::BeginPopup("Menu"))
  {
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Reset, "Reset")))
    {
      fParent->setBackgroundKey("");
    }

    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ImportImages, "Import")))
    {
      auto textureKey = iCtx.importTextureBlocking();
      if(textureKey)
        fParent->setBackgroundKey(*textureKey);
    }

    ImGui::SeparatorText("Effects");

    ImGui::BeginDisabled(!fEffects.hasAny());
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ResetAllEffects, "Reset All Effects")))
      fParent->setBackgroundEffect("all effects (reset)", texture::kDefaultFX, MergeKey::from(&fEffects));
    if(ImGui::MenuItem(ReGui_Prefix(ICON_FAC_SparklesCircleCheck, "Commit All Effects")))
      fParent->commitBackgroundEffects(iCtx);
    ImGui::EndDisabled();

    ImGui::EndPopup();
  }

  ImGui::SameLine();

  auto offset = ImGui::GetCursorPosX();

  auto key = getTextureKey();
  if(ImGui::BeginCombo("graphics", key.c_str()))
  {
    auto textureKeys = (fFilter && ReGui::IsFilterEnabled()) ? iCtx.findTextureKeys(fFilter) : iCtx.getTextureKeys();
    for(auto &p: textureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
        fParent->setBackgroundKey(p);
      if(ReGui::ShowQuickView())
        iCtx.textureTooltip(p);
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(hasTexture() && ReGui::ShowQuickView())
  {
    ReGui::ToolTip([this] {
      auto texture = getTexture();
      if(texture->isValid())
      {
        ImGui::TextUnformatted(fmt::printf("%dx%d | %d frames",
                                           static_cast<int>(texture->frameWidth()),
                                           static_cast<int>(texture->frameHeight()),
                                           texture->numFrames()).c_str());
        texture->ItemFit({static_cast<float>(k1UPixelSize), static_cast<float>(k1UPixelSize)});
      }
      else
        ImGui::TextUnformatted(fmt::printf("%s", texture->getFilmStrip()->errorMessage()).c_str());
    });
  }

  if(hasTexture())
  {
    ImGui::SetCursorPosX(offset);
    auto const itemWidth = AppContext::GetCurrent().fItemWidth;

    ImGui::BeginGroup();

    if(!isSizeValid())
    {
      if(ImGui::Button("Resize to fit panel", ImVec2{itemWidth, 0}))
      {
        auto fx = fEffects;
        fx.fSizeOverride = fParent->getSize();
        fParent->setBackgroundEffect("size", fx, MergeKey::from(&fEffects.fSizeOverride));
      }
    }

    re::edit::impl::editEffects(fEffects, itemWidth, offset,
                                [parent = fParent](char const *iName, texture::FX const &fx, MergeKey const &iMergeKey) {
                                  parent->setBackgroundEffect(iName, fx, iMergeKey);
                                });

    ImGui::EndGroup();
  }
}

//------------------------------------------------------------------------
// Graphics::initTextureKey
//------------------------------------------------------------------------
void Graphics::initTextureKey(Texture::key_t const &iTextureKey,
                              std::optional<Texture::key_t> const &iOriginalTextureKey,
                              texture::FX const &iEffects)
{
  if(iOriginalTextureKey)
  {
    fDNZTexture = AppContext::GetCurrent().findTexture(*iOriginalTextureKey);
    if(!fDNZTexture || !fDNZTexture->isValid())
    {
      if(iEffects.hasAny())
        RE_EDIT_LOG_WARNING("Could not locate texture %s to apply effects.", *iOriginalTextureKey);

      fTextureKey = iTextureKey;
      fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey);
    }
    else
    {
      fTextureKey = *iOriginalTextureKey;
      fEffects = iEffects;
    }
  }
  else
  {
    fTextureKey = iTextureKey;
    fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey);
  }
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::isSizeValid
//------------------------------------------------------------------------
bool Graphics::isSizeValid() const
{
  if(hasValidTexture())
  {
    auto size = fEffects.hasSizeOverride() ? *fEffects.fSizeOverride : getTexture()->frameSize();
    return size == fParent->getSize();
  }
  else
    return false;
}

//------------------------------------------------------------------------
// Graphics::findErrors
//------------------------------------------------------------------------
void Graphics::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(hasTexture())
  {
    auto texture = getTexture();
    if(!texture->isValid())
      oErrors.add(texture->getFilmStrip()->errorMessage());
    else
    {
      auto size = fEffects.hasSizeOverride() ? *fEffects.fSizeOverride : texture->frameSize();
      auto expectedSize = fParent->getSize();
      if(size != expectedSize)
      {
        oErrors.add("Size must be %dx%d", static_cast<int>(expectedSize.x), static_cast<int>(expectedSize.y));
      }
    }
  }
  else
    oErrors.add("Required");
}

//------------------------------------------------------------------------
// Graphics::collectUsedTexturePaths
//------------------------------------------------------------------------
void Graphics::collectUsedTexturePaths(std::set<fs::path> &oPaths) const
{
  impl::collectUsedTexturePaths(fDNZTexture.get(), fEffects, oPaths);
}

//------------------------------------------------------------------------
// Graphics::collectAllUsedTextureKeys
//------------------------------------------------------------------------
void Graphics::collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const
{
  impl::collectAllUsedTextureKeys(fDNZTexture.get(), fEffects, oKeys);
}

//------------------------------------------------------------------------
// Graphics::collectFilmStripEffects
//------------------------------------------------------------------------
void Graphics::collectFilmStripEffects(std::vector<FilmStripFX> &oEffects) const
{
  if(hasValidTexture() && fEffects.hasAny())
  {
    oEffects.emplace_back(FilmStripFX{getTexture()->key(), fEffects});
  }
}


}

namespace re::edit::widget::attribute {

namespace impl {

//------------------------------------------------------------------------
// impl::computeTextureColor
//------------------------------------------------------------------------
constexpr ImU32 computeTextureColor(bool iXRay)
{
  return iXRay ?
         ReGui::GetColorU32(kXRayColor) :
         ReGui::GetColorU32(kWhiteColor);
}

}

//------------------------------------------------------------------------
// Graphics::draw
//------------------------------------------------------------------------
void Graphics::draw(AppContext &iCtx, ReGui::Canvas &iCanvas, ImU32 iBorderColor, bool iXRay) const
{
  auto texture = hasTexture() ? getTexture() : nullptr;
  if(texture && texture->isValid())
  {
    iCanvas.addTexture(texture, fPosition, fFrameNumber, iBorderColor, impl::computeTextureColor(iXRay), fEffects);
  }
  else
  {
    auto color = iXRay ?
                 iCtx.getUserPreferences().fWidgetNoGraphicsColor :
                 iCtx.getUserPreferences().fWidgetNoGraphicsXRayColor;
    switch(iCtx.fNoGraphicsRendering)
    {
      case AppContext::ENoGraphicsRendering::kFill:
        iCanvas.addRectFilled(fPosition, getSize(), color);
        break;
      case AppContext::ENoGraphicsRendering::kBorder:
        iCanvas.addRect(fPosition, getSize(), color);
        break;
      case AppContext::ENoGraphicsRendering::kNone:
        // do nothing
        break;
    }
    drawBorder(iCanvas, iBorderColor);
  }
}

//------------------------------------------------------------------------
// Graphics::drawBorder
//------------------------------------------------------------------------
void Graphics::drawBorder(ReGui::Canvas &iCanvas, ImU32 iBorderColor) const
{
  if(!ReGui::ColorIsTransparent(iBorderColor))
    iCanvas.addRect(fPosition, getSize(), iBorderColor);
}

//------------------------------------------------------------------------
// Graphics::drawHitBoundaries
//------------------------------------------------------------------------
void Graphics::drawHitBoundaries(ReGui::Canvas &iCanvas, ImU32 iColor) const
{
  if(fHitBoundariesEnabled)
    iCanvas.addRect(fPosition + ImVec2{fHitBoundaries.fLeftInset, fHitBoundaries.fTopInset},
                    getSize() - ImVec2{fHitBoundaries.fLeftInset + fHitBoundaries.fRightInset,
                                       fHitBoundaries.fTopInset + fHitBoundaries.fBottomInset},
                    iColor);
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(AppContext &iCtx,
                        FilmStrip::Filter const &iFilter,
                        std::function<void(std::string const &)> const &iOnTextureUpdate,
                        std::function<void(ImVec2 const &)> const &iOnSizeUpdate,
                        std::function<void(char const *iName, texture::FX const &fx, MergeKey const &iMergeKey)> const &iOnFXUpdate)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  auto numFramesPopup = ImGui::GetID("NumFrames_popup");

  if(ImGui::BeginPopup("Menu"))
  {
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Reset, "Reset")))
    {
      resetAttribute();
    }

    // Copy
    copyToClipboardMenuItem(iCtx);

    ImGui::BeginDisabled(hasSize());
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Frames_Edit, "Change number of frames")))
      ImGui::OpenPopup(numFramesPopup);
    ImGui::EndDisabled();

    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ImportImages, "Import")))
    {
      auto textureKey = iCtx.importTextureBlocking();
      if(textureKey)
        iOnTextureUpdate(*textureKey);
    }

    ImGui::SeparatorText("Effects");

    ImGui::BeginDisabled(hasSize() || !fEffects.hasAny());
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ResetAllEffects, "Reset All Effects")))
      iOnFXUpdate("all effects (reset)", texture::kDefaultFX, MergeKey::from(&fEffects));
    if(ImGui::MenuItem(ReGui_Prefix(ICON_FAC_SparklesCircleCheck, "Commit All Effects")))
    {
      if(hasTexture())
      {
        auto newKey = iCtx.applyTextureEffects(getTextureKey(), fEffects);
        if(newKey)
          iOnTextureUpdate(*newKey);
      }
    }
    ImGui::EndDisabled();

    ImGui::EndPopup();
  }

  if(ImGui::BeginPopup("NumFrames_popup"))
  {
    auto texture = getTexture();
    auto numFrames = texture->numFrames();
    if(ImGui::InputInt("frames", &numFrames, 1, 10))
    {
      iCtx.overrideTextureNumFrames(texture->key(), numFrames);
      fEdited = true;
    }
    if(ImGui::Button("Ok"))
    {
      iCtx.resetUndoMergeKey();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::SameLine();

  auto offset = ImGui::GetCursorPosX();

  auto key = hasTexture() ? getTextureKey() : "";
  if(ImGui::BeginCombo(fName, key.c_str()))
  {
    auto textureKeys = (iFilter && ReGui::IsFilterEnabled()) ? iCtx.findTextureKeys(iFilter) : iCtx.getTextureKeys();
    for(auto &p: textureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        iOnTextureUpdate(p);
      }

      if(ReGui::ShowQuickView())
        iCtx.textureTooltip(p);

      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(hasTexture() && ReGui::ShowQuickView())
  {
    iCtx.textureTooltip(getTextureKey());
  }

  ImGui::SetCursorPosX(offset);
  auto const itemWidth = AppContext::GetCurrent().fItemWidth;

  ImGui::BeginGroup();

  static bool kLinkWidthAndHeight{true};

  auto size = getSize();
  ImGui::PushID("width");
  {
    if(ReGui::ResetButton())
    {
      if(kLinkWidthAndHeight)
        iOnSizeUpdate(getOriginalSize());
      else
      {
        size.x = getOriginalSize().x;
        iOnSizeUpdate(size);
      }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));
    if(ReGui::InputInt("w", &size.x, 1, static_cast<int>(iCtx.fGrid.width())))
    {
      size.x = std::max(1.0f, size.x);
      if(kLinkWidthAndHeight)
      {
        auto originalSize = getOriginalSize();
        size.y = std::max(1.0f, originalSize.y * (size.x / std::max(1.0f, originalSize.x)));
      }
      iOnSizeUpdate(size);
    }
    ImGui::PopItemWidth();
  }
  ImGui::PopID();

  ImGui::PushID("height");
  {
    if(ReGui::ResetButton())
    {
      if(kLinkWidthAndHeight)
        iOnSizeUpdate(getOriginalSize());
      else
      {
        size.y = getOriginalSize().y;
        iOnSizeUpdate(size);
      }
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(itemWidth - (ImGui::GetCursorPosX() - offset));
    if(ReGui::InputInt("h", &size.y, 1, static_cast<int>(iCtx.fGrid.height())))
    {
      size.y = std::max(1.0f, size.y);
      if(kLinkWidthAndHeight)
      {
        auto originalSize = getOriginalSize();
        size.x = std::max(1.0f, originalSize.x * (size.y / std::max(1.0f, originalSize.y)));
      }
      iOnSizeUpdate(size);
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    ImGui::Checkbox("Link", &kLinkWidthAndHeight);
  }
  ImGui::PopID();

  if(hasTexture())
  {
    re::edit::impl::editEffects(fEffects, itemWidth, offset, iOnFXUpdate);
  }

  ImGui::EndGroup();
}

//------------------------------------------------------------------------
// Graphics::editPositionView
//------------------------------------------------------------------------
void Graphics::editPositionView(AppContext &iCtx)
{
  auto editedPosition = fPosition;

  ImGui::PushID("ResetX");
  if(ReGui::ResetButton())
  {
    editedPosition.x = 0;
    getParent()->setPosition(editedPosition);
  }
  ImGui::PopID();

  ImGui::SameLine();

  if(ReGui::InputInt("x", &editedPosition.x, 1, static_cast<int>(iCtx.fGrid.width())))
  {
    getParent()->setPosition(editedPosition);
  }

  ImGui::PushID("ResetY");
  if(ReGui::ResetButton())
  {
    editedPosition.y = 0;
    getParent()->setPosition(editedPosition);
  }
  ImGui::PopID();

  ImGui::SameLine();

  if(ReGui::InputInt("y", &editedPosition.y, 1, static_cast<int>(iCtx.fGrid.height())))
  {
    getParent()->setPosition(editedPosition);
  }

  if(hasTexture())
  {
    auto texture = getTexture();
    if(texture->isValid())
    {
      auto numFrames = texture->numFrames();
      if(numFrames > 1)
      {
        if(ReGui::ResetButton())
          fFrameNumber = 0;
        ImGui::SameLine();
        ImGui::SliderInt("frame", &fFrameNumber, 0, numFrames - 1);
      }
    }
  }
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(AppContext &iCtx)
{
  editView(iCtx,
           fFilter,
           [this](auto &k) { updateTextureKey(k); },
           [this](auto &s) {
             update([this, &s] {
                      if(hasTexture())
                      {
                        fEffects.fSizeOverride = s != getOriginalSize() ? std::optional<ImVec2>(s) : std::nullopt;
                      }
                      else
                        fTexture = s;
                    },
                    fmt::printf("Change %s size", getParent()->getName()),
                    MergeKey::from(&fEffects.fSizeOverride));
           },
           [this](char const *iName, texture::FX const &fx, MergeKey const &iMergeKey) {
             update([this, &fx] {
                      fEffects = fx;
                    },
                    fmt::printf("Change %s %s", getParent()->getName(), iName),
                    iMergeKey);
           }
  );
  ImGui::Indent();
  editHitBoundariesView(iCtx);
  ImGui::Unindent();
}

//------------------------------------------------------------------------
// Graphics::editHitBoundariesView
//------------------------------------------------------------------------
void Graphics::editHitBoundariesView(AppContext &iCtx)
{
  if(fHitBoundariesEnabled && iCtx.fBorderRendering == AppContext::EBorderRendering::kHitBoundaries)
  {
    auto editedHB = fHitBoundaries;

    float *tb[] = { &editedHB.fTopInset, &editedHB.fBottomInset };
    if(ReGui::SliderInt2("hit_boundaries - Top | Bottom", tb, 0, static_cast<int>(getSize().y), "inset: %d", ImGuiSliderFlags_AlwaysClamp))
    {
      update([this, &editedHB] {
               fHitBoundaries = editedHB;
             },
             fmt::printf("Change %s Hit Boundaries", getParent()->getName()),
             MergeKey::from(&fHitBoundaries.fTopInset));
    }

    float *lr[] = { &editedHB.fLeftInset, &editedHB.fRightInset };
    if(ReGui::SliderInt2("hit_boundaries - Left | Right", lr, 0, static_cast<int>(getSize().x), "inset: %d", ImGuiSliderFlags_AlwaysClamp))
    {
      update([this, &editedHB] {
               fHitBoundaries = editedHB;
             },
             fmt::printf("Change %s Hit Boundaries", getParent()->getName()),
             MergeKey::from(&fHitBoundaries.fLeftInset));
    }
  }
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fTexture = kNoGraphics;
  if(fEffects.hasSizeOverride())
    fTexture = *fEffects.fSizeOverride;
  else if(fDNZTexture && fDNZTexture->isValid())
  {
    fTexture = fDNZTexture->frameSize();
  }
  fDNZTexture = nullptr;
  fHitBoundaries = {};
  fEdited = true;
  fEffects = texture::kDefaultFX;
}

//------------------------------------------------------------------------
// Graphics::findErrors
//------------------------------------------------------------------------
void Graphics::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  if(fCheckForOOBError)
  {
    auto max = iCtx.getCurrentPanelSize();
    auto p = getTopLeft();
    if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
    {
      oErrors.add("Out of bound");
    }
    else
    {
      p = getBottomRight();
      if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
      {
        oErrors.add("Out of bound");
      }
    }
  }

  if(hasTexture())
  {
    auto texture = getTexture();
    if(!texture->isValid())
      oErrors.add(texture->getFilmStrip()->errorMessage());
    else
    {
      if(fFilter)
      {
        if(!iCtx.checkTextureKeyMatchesFilter(texture->key(), fFilter))
          oErrors.add(fFilter.fDescription);
      }
    }
  }
  else
  {
    if(!fSizeEnabled)
      oErrors.add("Required");
  }
}

//------------------------------------------------------------------------
// Graphics::hdgui2D
//------------------------------------------------------------------------
void Graphics::hdgui2D(attribute_list_t &oAttributes) const
{
  hdgui2D(getParent()->getName(), oAttributes);
}

//------------------------------------------------------------------------
// Graphics::hdgui2D
//------------------------------------------------------------------------
void Graphics::hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const
{
  if(hasHitBoundaries())
  {
    oAttributes.emplace_back(attribute_t{fName, re::mock::fmt::printf("{ node = \"%s\", hit_boundaries = { left = %d, top = %d, right = %d, bottom = %d }}",
                                                                      iNodeName,
                                                                      static_cast<int>(fHitBoundaries.fLeftInset),
                                                                      static_cast<int>(fHitBoundaries.fTopInset),
                                                                      static_cast<int>(fHitBoundaries.fRightInset),
                                                                      static_cast<int>(fHitBoundaries.fBottomInset)
                                                                      )});
  }
  else
    oAttributes.emplace_back(attribute_t{fName, re::mock::fmt::printf("{ node = \"%s\" }", iNodeName)});
}

//------------------------------------------------------------------------
// Graphics::collectUsedTexturePaths
//------------------------------------------------------------------------
void Graphics::collectUsedTexturePaths(std::set<fs::path> &oPaths) const
{
  if(hasTexture())
    re::edit::impl::collectUsedTexturePaths(fDNZTexture.get(), fEffects, oPaths);
}

//------------------------------------------------------------------------
// Graphics::collectAllUsedTextureKeys
//------------------------------------------------------------------------
void Graphics::collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const
{
  if(hasTexture())
    re::edit::impl::collectAllUsedTextureKeys(fDNZTexture.get(), fEffects, oKeys);
}

//------------------------------------------------------------------------
// Graphics::collectFilmStripEffects
//------------------------------------------------------------------------
void Graphics::collectFilmStripEffects(std::vector<FilmStripFX> &oEffects) const
{
  if(hasValidTexture() && fEffects.hasAny())
  {
    oEffects.emplace_back(FilmStripFX{getTexture()->key(), fEffects});
  }
}

//------------------------------------------------------------------------
// Graphics::collectUsedTextureBuiltIns
//------------------------------------------------------------------------
void Graphics::collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const
{
  if(hasTexture())
  {
    auto filmStrip = getTexture()->getFilmStrip();
    if(filmStrip->hasBuiltIn())
      oKeys.emplace(filmStrip->key());
  }
}

//------------------------------------------------------------------------
// Graphics::device2D
//------------------------------------------------------------------------
std::string Graphics::device2D() const
{
  std::string path;
  if(hasTexture())
  {
    path = re::edit::impl::graphics_device2D_path(fDNZTexture.get(), fEffects);
  }
  else
  {
    auto size = getSize();
    path = re::mock::fmt::printf("size = { %d, %d }", stl::roundToInt(size.x),  stl::roundToInt(size.y));
  }
  auto pos = getPosition();
  if(pos.x == 0 && pos.y == 0)
    return re::mock::fmt::printf("{ { %s } }",
                                 path);
  else
    return re::mock::fmt::printf("{ offset = { %d, %d }, { %s } }",
                                 stl::roundToInt(pos.x),
                                 stl::roundToInt(pos.y),
                                 path);
}


//------------------------------------------------------------------------
// Graphics::toValueString
//------------------------------------------------------------------------
std::string Graphics::toValueString() const
{
  if(hasTexture())
  {
    auto texture = getTexture();
    if(fEffects.hasAny())
      return fmt::printf("graphics = \"%s\" (" ReGui_Icon_Effects ")", texture->key());
    else
      return fmt::printf("graphics = \"%s\"", texture->key());

  }
  else
  {
    auto size = getSize();
    return fmt::printf("graphics = { %d, %d }", stl::roundToInt(size.x),  stl::roundToInt(size.y));
  }
}

//------------------------------------------------------------------------
// Graphics::copyFromAction
//------------------------------------------------------------------------
bool Graphics::copyFromAction(Attribute const *iFromAttribute)
{
  auto fromAttribute = dynamic_cast<Graphics const *>(iFromAttribute);
  if(fromAttribute)
  {
    fHitBoundaries = fromAttribute->fHitBoundaries;
    fTexture = fromAttribute->fTexture;
    fDNZTexture = fromAttribute->fDNZTexture;
    fEffects = fromAttribute->fEffects;
    fEdited = true;
    return true;
  }
  else
    return false;
}

//------------------------------------------------------------------------
// Graphics::setTextureKey
//------------------------------------------------------------------------
void Graphics::setTextureKey(Texture::key_t const &iTextureKey)
{
  fTexture = iTextureKey;
  fFrameNumber = 0;
  fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey);
  fEffects = texture::kDefaultFX;
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::updateTextureKey
//------------------------------------------------------------------------
void Graphics::updateTextureKey(Texture::key_t const &iTextureKey)
{
  update([this, &iTextureKey] {
           setTextureKey(iTextureKey);
         },
         fmt::printf("Change %s graphics", getParent()->getName()));
}

//------------------------------------------------------------------------
// Graphics::initTextureKey
//------------------------------------------------------------------------
void Graphics::initTextureKey(Texture::key_t const &iTextureKey,
                              std::optional<Texture::key_t> const &iOriginalTextureKey,
                              texture::FX const &iEffects)
{
  if(iOriginalTextureKey)
  {
    fDNZTexture = AppContext::GetCurrent().findTexture(*iOriginalTextureKey);
    if(!fDNZTexture || !fDNZTexture->isValid())
    {
      if(iEffects.hasAny())
        RE_EDIT_LOG_WARNING("Could not locate texture %s to apply effects.", *iOriginalTextureKey);

      fTexture = iTextureKey;
      fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey);
    }
    else
    {
      fTexture = *iOriginalTextureKey;
      fEffects = iEffects;
    }
  }
  else
  {
    fTexture = iTextureKey;
    fDNZTexture = AppContext::GetCurrent().getTexture(iTextureKey);
  }
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::setSize
//------------------------------------------------------------------------
void Graphics::setSize(ImVec2 const &iSize)
{
  fTexture = iSize;
  fDNZTexture.reset();
  fEffects = texture::kDefaultFX;
  fEdited = true;
}

//------------------------------------------------------------------------
// Background::draw
//------------------------------------------------------------------------
bool Background::draw(AppContext &iCtx, ReGui::Canvas &iCanvas, Graphics const *iParent, ImU32 iBorderColor, bool xRay) const
{
  if(fProvided)
  {
    switch(iCtx.fCustomDisplayRendering)
    {
      case AppContext::ECustomDisplayRendering::kBackgroundSD:
      {
        auto texture = iCtx.findTexture(fValue);
        if(texture && texture->isValid())
        {
          auto fx = texture::kDefaultFX;
          fx.fSizeOverride = iParent->getSize();
          iCanvas.addTexture(texture.get(), iParent->fPosition, 0, iBorderColor, impl::computeTextureColor(xRay), fx);
          return true;
        }
        break;
      }

      case AppContext::ECustomDisplayRendering::kBackgroundHD:
      {
        auto texture = iCtx.findHDTexture(fValue);
        if(texture && texture->isValid())
        {
          iCanvas.addTexture(texture.get(), iParent->fPosition, 0, iBorderColor, impl::computeTextureColor(xRay));
          return true;
        }
        break;
      }
      default:
        RE_EDIT_FAIL("not reached");
    }
  }

  return false;
}

//------------------------------------------------------------------------
// Background::collectUsedTexturePaths
//------------------------------------------------------------------------
void Background::collectUsedTexturePaths(std::set<fs::path> &oPaths) const
{
  auto &app = AppContext::GetCurrent();

  auto texture = app.findTexture(fValue);
  if(texture && texture->isValid())
  {
    auto filmStrip = texture->getFilmStrip();
    if(filmStrip->hasPath())
      oPaths.emplace(filmStrip->path());
  }
  auto hdTexture = app.findHDTexture(fValue);
  if(hdTexture && hdTexture->isValid())
  {
    auto filmStrip = hdTexture->getFilmStrip();
    if(filmStrip->hasPath())
      oPaths.emplace(filmStrip->path());
  }
}

//------------------------------------------------------------------------
// Background::collectUsedTexturePaths
//------------------------------------------------------------------------
void Background::collectAllUsedTextureKeys(std::set<FilmStrip::key_t> &oKeys) const
{
  auto &app = AppContext::GetCurrent();

  auto texture = app.findTexture(fValue);
  if(texture && texture->isValid())
  {
    auto filmStrip = texture->getFilmStrip();
    if(filmStrip->hasPath())
      oKeys.emplace(filmStrip->key());
  }
  auto hdTexture = app.findHDTexture(fValue);
  if(hdTexture && hdTexture->isValid())
  {
    auto filmStrip = hdTexture->getFilmStrip();
    if(filmStrip->hasPath())
      oKeys.emplace(filmStrip->key());
  }
}

namespace impl {

inline bool ends_with(std::string const &s, std::string const &iSuffix)
{
  if(s.size() < iSuffix.size())
    return false;
  return s.substr(s.size() - iSuffix.size()) == iSuffix;
}

}

//------------------------------------------------------------------------
// Background::editView
//------------------------------------------------------------------------
void Background::editView(AppContext &iCtx)
{
  static const FilmStrip::Filter kBackgroundFilter{[](FilmStrip const &f) { return f.numFrames() == 1; }, "Must have exactly 1 frame"};

  menuView(iCtx);
  ImGui::SameLine();

  if(ImGui::BeginCombo(fName, fValue.c_str()))
  {
    auto textureKeys = ReGui::IsFilterEnabled() ? iCtx.findTextureKeys(kBackgroundFilter) : iCtx.getTextureKeys();
    for(auto const &p: textureKeys)
    {
      auto key = p;
      auto path = p;

      if(impl::ends_with(key, "-HD"))
      {
        key = key.substr(0, key.size() - 3);
        path = re::mock::fmt::printf("%s (HD)", key);
      }

      auto const isSelected = key == fValue;
      if(ImGui::Selectable(path.c_str(), isSelected))
      {
        updateAttribute([this, &key] {
          fValue = key;
          fProvided = true;
        });
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

//------------------------------------------------------------------------
// Background::hdgui2D
//------------------------------------------------------------------------
std::string Background::getValueAsLua() const
{
  return re::mock::fmt::printf("jbox.image{ path = \"%s\" }", fValue);
}

}