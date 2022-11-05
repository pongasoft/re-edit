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
#include "Widget.h"
#include "Errors.h"
#include "Panel.h"

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

}

namespace re::edit::panel {

//------------------------------------------------------------------------
// Graphics::device2D
//------------------------------------------------------------------------
std::string Graphics::device2D() const
{
  auto texture = findTexture();
  std::string path;
  if(texture)
    path = fmt::printf("path = \"%s\"", texture->key());
  return fmt::printf("{ { %s } }", path);
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fTextureKey = "";
  fDNZTexture.reset();
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(AppContext &iCtx)
{
  if(ReGui::ResetButton())
  {
    reset();
  }
  ImGui::SameLine();

  auto key = getTextureKey();
  if(ImGui::BeginCombo("graphics", key.c_str()))
  {
    auto textureKeys = fFilter ? iCtx.findTextureKeys(fFilter) : iCtx.getTextureKeys();
    for(auto &p: textureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        iCtx.addUndoLambda(fTextureKey, p,
                           "Change background graphics",
                           [](UndoAction *iAction, auto const &iValue) {
                             auto panel = AppContext::GetCurrent().getPanel(iAction->fPanelType);
                             panel->setBackgroundKey(iValue);
                           });
        fTextureKey = p;
        fDNZTexture.reset();
        fEdited = true;
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    auto texture = findTexture();
    if(texture)
    {
      if(texture->isValid())
        ImGui::TextUnformatted(fmt::printf("%dx%d | %d frames",
                                           static_cast<int>(texture->frameWidth()),
                                           static_cast<int>(texture->frameHeight()),
                                           texture->numFrames()).c_str());
      else
        ImGui::TextUnformatted(fmt::printf("%s", texture->getFilmStrip()->errorMessage()).c_str());
    }
    else
    {
      ImGui::TextUnformatted("Missing png");
    }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

}

//------------------------------------------------------------------------
// Graphics::findErrors
//------------------------------------------------------------------------
void Graphics::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto texture = findTexture();
  if(!texture)
    oErrors.add("Missing png");
  else
  {
    if(!texture->isValid())
      oErrors.add("Invalid png: %s", texture->getFilmStrip()->errorMessage());
    else
    {
      if(fFilter)
      {
        auto keys = iCtx.findTextureKeys(fFilter);
        if(std::find(keys.begin(), keys.end(), texture->key()) == keys.end())
          oErrors.add(fFilter.fDescription);
      }
    }
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
         ReGui::GetColorU32(kWhiteColor) :
         ReGui::GetColorU32(kXRayColor);
}

}

//------------------------------------------------------------------------
// Graphics::draw
//------------------------------------------------------------------------
void Graphics::draw(AppContext &iCtx, ImU32 iBorderColor, bool iXRay) const
{
  auto texture = hasTexture() ? findTexture() : nullptr;
  if(texture)
  {
    iCtx.drawTexture(texture.get(), fPosition, fFrameNumber, iBorderColor, impl::computeTextureColor(iXRay));
  }
  else
  {
    auto color = iXRay ?
                 iCtx.getUserPreferences().fWidgetNoGraphicsColor :
                 iCtx.getUserPreferences().fWidgetNoGraphicsXRayColor;
    switch(iCtx.fNoGraphicsRendering)
    {
      case AppContext::ENoGraphicsRendering::kFill:
        iCtx.drawRectFilled(fPosition, getSize(), color);
        break;
      case AppContext::ENoGraphicsRendering::kBorder:
        iCtx.drawRect(fPosition, getSize(), color);
        break;
      case AppContext::ENoGraphicsRendering::kNone:
        // do nothing
        break;
    }
    drawBorder(iCtx, iBorderColor);
  }
}

//------------------------------------------------------------------------
// Graphics::drawBorder
//------------------------------------------------------------------------
void Graphics::drawBorder(AppContext &iCtx, ImU32 iBorderColor) const
{
  if(!ReGui::ColorIsTransparent(iBorderColor))
    iCtx.drawRect(fPosition, getSize(), iBorderColor);
}

//------------------------------------------------------------------------
// Graphics::drawHitBoundaries
//------------------------------------------------------------------------
void Graphics::drawHitBoundaries(AppContext &iCtx, ImU32 iColor) const
{
  if(fHitBoundariesEnabled)
    iCtx.drawRect(fPosition + ImVec2{fHitBoundaries.fLeftInset, fHitBoundaries.fTopInset},
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
                        std::function<void(ImVec2 const &)> const &iOnSizeUpdate)
{
  if(ReGui::ResetButton())
  {
    iCtx.addUndoAttributeReset(this);
    reset();
  }
  ImGui::SameLine();

  ImGui::BeginGroup();

  auto key = hasTexture() ? getTextureKey() : "";
  if(ImGui::BeginCombo(fName, key.c_str()))
  {
    auto textureKeys = iFilter ? iCtx.findTextureKeys(iFilter) : iCtx.getTextureKeys();
    for(auto &p: textureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
      {
        iOnTextureUpdate(p);
      }
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(hasTexture() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    auto texture = findTexture();
    if(texture)
    {
      if(texture->isValid())
        ImGui::TextUnformatted(fmt::printf("%dx%d | %d frames",
                                           static_cast<int>(texture->frameWidth()),
                                           static_cast<int>(texture->frameHeight()),
                                           texture->numFrames()).c_str());
      else
        ImGui::TextUnformatted(fmt::printf("%s", texture->getFilmStrip()->errorMessage()).c_str());
    }
    else
    {
      ImGui::TextUnformatted("Missing png");
    }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

  if(hasSize())
  {
    auto editedSize = std::get<ImVec2>(fTexture);
    ImGui::Indent();
    if(ReGui::InputInt("w", &editedSize.x, 1, 5))
    {
      iOnSizeUpdate(editedSize);
    }

    if(ReGui::InputInt("h", &editedSize.y, 1, 5))
    {
      iOnSizeUpdate(editedSize);
    }

    ImGui::Unindent();
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
    iCtx.addOrMergeUndoCurrentWidgetChange(nullptr, fPosition, editedPosition,
                                           fmt::printf("Move %s", iCtx.getCurrentWidget()->getName()));
    fPosition = editedPosition;
    fEdited = true;
  }
  ImGui::PopID();

  ImGui::SameLine();

  if(ReGui::InputInt("x", &editedPosition.x, 1, 5))
  {
    iCtx.addOrMergeUndoCurrentWidgetChange(&fPosition, fPosition, editedPosition,
                                           fmt::printf("Move %s", iCtx.getCurrentWidget()->getName()));
    fPosition = editedPosition;
    fEdited = true;
  }

  ImGui::PushID("ResetY");
  if(ReGui::ResetButton())
  {
    editedPosition.y = 0;
    iCtx.addOrMergeUndoCurrentWidgetChange(nullptr, fPosition, editedPosition,
                                           fmt::printf("Move %s", iCtx.getCurrentWidget()->getName()));
    fPosition = editedPosition;
    fEdited = true;
  }
  ImGui::PopID();

  ImGui::SameLine();

  if(ReGui::InputInt("y", &editedPosition.y, 1, 5))
  {
    iCtx.addOrMergeUndoCurrentWidgetChange(&fPosition, fPosition, editedPosition,
                                           fmt::printf("Move %s", iCtx.getCurrentWidget()->getName()));
    fPosition = editedPosition;
    fEdited = true;
  }

  auto texture = findTexture();
  if(texture)
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

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(AppContext &iCtx)
{
  editView(iCtx,
           fFilter,
           [this, &iCtx](auto &k) {
             iCtx.addUndoCurrentWidgetChange(fmt::printf("Change %s graphics", iCtx.getCurrentWidget()->getName()));
             setTextureKey(k);
             fFrameNumber = 0;
           },
           [this, &iCtx](auto &s) {
             iCtx.addOrMergeUndoCurrentWidgetChange(&fTexture, getSize(), s,
                                                    fmt::printf("Change %s size", iCtx.getCurrentWidget()->getName()));
             setSize(s);
             fFrameNumber = 0;
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
      iCtx.addOrMergeUndoCurrentWidgetChange(&fHitBoundaries.fTopInset, fHitBoundaries, editedHB,
                                             fmt::printf("Change %s Hit Boundaries", iCtx.getCurrentWidget()->getName()));
      fHitBoundaries = editedHB;
      fEdited = true;
    }

    float *lr[] = { &editedHB.fLeftInset, &editedHB.fRightInset };
    if(ReGui::SliderInt2("hit_boundaries - Left | Right", lr, 0, static_cast<int>(getSize().x), "inset: %d", ImGuiSliderFlags_AlwaysClamp))
    {
      iCtx.addOrMergeUndoCurrentWidgetChange(&fHitBoundaries.fLeftInset, fHitBoundaries, editedHB,
                                             fmt::printf("Change %s Hit Boundaries", iCtx.getCurrentWidget()->getName()));
      fHitBoundaries = editedHB;
      fEdited = true;
    }
  }
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  auto texture = findTexture();
  fTexture = texture ? texture->frameSize() : kNoGraphics;
  fDNZTexture.reset();
  fHitBoundaries = {};
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::checkForErrors
//------------------------------------------------------------------------
void Graphics::findErrors(AppContext &iCtx, UserError &oErrors) const
{
  auto max = iCtx.getCurrentPanelSize();
  auto p = getTopLeft();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    oErrors.add("Out of bound");
  }
  p = getBottomRight();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    oErrors.add("Out of bound");
  }

  if(hasTexture())
  {
    auto texture = findTexture();
    if(!texture)
      oErrors.add("Missing png");
    else
    {
      if(!texture->isValid())
        oErrors.add("Invalid png: %s", texture->getFilmStrip()->errorMessage());
      else
      {
        if(fFilter)
        {
          auto keys = iCtx.findTextureKeys(fFilter);
          if(std::find(keys.begin(), keys.end(), texture->key()) == keys.end())
            oErrors.add(fFilter.fDescription);
        }
      }
    }
  }
}

//------------------------------------------------------------------------
// Graphics::hdgui2D
//------------------------------------------------------------------------
void Graphics::hdgui2D(AppContext &iCtx, attribute_list_t &oAttributes) const
{
  hdgui2D(iCtx.getCurrentWidget()->getName(), oAttributes);
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
// Graphics::device2D
//------------------------------------------------------------------------
std::string Graphics::device2D() const
{
  auto texture = findTexture();
  std::string path;
  if(texture)
  {
    if(texture->numFrames() > 1)
      path = re::mock::fmt::printf("path = \"%s\", frames = %d", texture->key(), texture->numFrames());
    else
      path = re::mock::fmt::printf("path = \"%s\"", texture->key());
  }
  else
  {
    auto size = getSize();
    path = re::mock::fmt::printf("size = { %d, %d }", static_cast<int>(size.x),  static_cast<int>(size.y));
  }
  auto pos = getPosition();
  if(pos.x == 0 && pos.y == 0)
    return re::mock::fmt::printf("{ { %s } }",
                                 path);
  else
    return re::mock::fmt::printf("{ offset = { %d, %d }, { %s } }",
                                 static_cast<int>(pos.x),
                                 static_cast<int>(pos.y),
                                 path);
}

//------------------------------------------------------------------------
// Graphics::copyFrom
//------------------------------------------------------------------------
bool Graphics::copyFrom(Attribute const *iFromAttribute)
{
  auto fromAttribute = dynamic_cast<Graphics const *>(iFromAttribute);
  if(fromAttribute)
  {
    fPosition = fromAttribute->fPosition;
    fHitBoundaries = fromAttribute->fHitBoundaries;
    fTexture = fromAttribute->fTexture;
    fDNZTexture.reset();
    fEdited = true;
    return true;
  }
  else
    return false;
}

//------------------------------------------------------------------------
// Background::draw
//------------------------------------------------------------------------
bool Background::draw(AppContext &iCtx, Graphics const *iParent, ImU32 iBorderColor, bool xRay) const
{
  if(fProvided)
  {
    switch(iCtx.fCustomDisplayRendering)
    {
      case AppContext::ECustomDisplayRendering::kBackgroundSD:
      {
        auto texture = iCtx.findTexture(fValue);
        if(texture)
        {
          auto zoom = iCtx.fZoom * iParent->getSize().x / texture->frameWidth();
          texture->draw(iParent->fPosition, iCtx.fZoom, zoom, 0, iBorderColor, impl::computeTextureColor(xRay));
          return true;
        }
        break;
      }

      case AppContext::ECustomDisplayRendering::kBackgroundHD:
      {
        auto texture = iCtx.findHDTexture(fValue);
        if(texture)
        {
          iCtx.drawTexture(texture.get(), iParent->fPosition, 0, iBorderColor, impl::computeTextureColor(xRay));
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

  if(ReGui::ResetButton())
    reset();
  ImGui::SameLine();

  if(ImGui::BeginCombo(fName, fValue.c_str()))
  {
    auto textureKeys = iCtx.findTextureKeys(kBackgroundFilter);
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
        iCtx.addUndoAttributeChange(this);
        fValue = key;
        fProvided = true;
        fEdited = true;
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