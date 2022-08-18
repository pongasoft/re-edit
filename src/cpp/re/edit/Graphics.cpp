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
namespace re::edit::widget::attribute {

//------------------------------------------------------------------------
// Graphics::draw
//------------------------------------------------------------------------
void Graphics::draw(AppContext &iCtx, ImVec4 const &iBorderCol) const
{
  if(hasTexture())
    iCtx.drawTexture(getTexture(), fPosition, fFrameNumber);
  else
    iCtx.drawRectFilled(fPosition, getSize(), ImVec4{1, 0, 0, 1});

  if(fHitBoundariesEnabled && iCtx.fShowBorder == AppContext::ShowBorder::kHitBoundaries)
    drawHitBoundaries(iCtx, kHitBoundariesColor);

  if(iBorderCol.w > 0.0f)
    drawBorder(iCtx, iBorderCol);
}

//------------------------------------------------------------------------
// Graphics::drawBorder
//------------------------------------------------------------------------
void Graphics::drawBorder(AppContext &iCtx, ImVec4 const &iBorderCol) const
{
  iCtx.drawRect(fPosition, getSize(), iBorderCol);
}

//------------------------------------------------------------------------
// Graphics::drawHitBoundaries
//------------------------------------------------------------------------
void Graphics::drawHitBoundaries(AppContext &iCtx, ImVec4 const &iColor) const
{
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
  if(ImGui::Button("X"))
    reset();
  ImGui::SameLine();

  ImGui::BeginGroup();
  auto const *texture = getTexture();
  auto key = texture ? texture->key() : "";
  if(ImGui::BeginCombo(fName, key.c_str()))
  {
    auto textureKeys = iFilter ? iCtx.findTextureKeys(iFilter) : iCtx.getTextureKeys();
    for(auto &p: textureKeys)
    {
      auto const isSelected = p == key;
      if(ImGui::Selectable(p.c_str(), isSelected))
        iOnTextureUpdate(p);
      if(isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if(texture && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(re::mock::fmt::printf("%dx%d | %d frames",
                                                 static_cast<int>(texture->frameWidth()),
                                                 static_cast<int>(texture->frameHeight()),
                                                 texture->numFrames()).c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

  if(!texture)
  {
    auto size = fSize;
    ImGui::Indent();
    auto updated = ReGui::InputInt("w", &size.x, 1, 5);
    updated |= ReGui::InputInt("h", &size.y, 1, 5);
    if(updated)
      iOnSizeUpdate(size);
    ImGui::Unindent();
  }
  ImGui::EndGroup();

}

//------------------------------------------------------------------------
// Graphics::editPositionView
//------------------------------------------------------------------------
void Graphics::editPositionView(AppContext &iCtx)
{
  if(ReGui::InputInt("x", &fPosition.x, 1, 5))
    fEdited = true;

  if(ReGui::InputInt("y", &fPosition.y, 1, 5))
    fEdited = true;

  if(hasTexture())
  {
    auto numFrames = getTexture()->numFrames();
    if(numFrames > 1)
      ImGui::SliderInt("Frame", &fFrameNumber, 0, numFrames - 1);
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
             fTexture = iCtx.getTexture(k);
             fFrameNumber = 0;
             fEdited = true;
           },
           [this](auto &s) {
             fSize = s;
             fTexture = nullptr;
             fFrameNumber = 0;
             fEdited = true;
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
  if(fHitBoundariesEnabled && iCtx.fShowBorder == AppContext::ShowBorder::kHitBoundaries)
  {
    float *tb[] = { &fHitBoundaries.fTopInset, &fHitBoundaries.fBottomInset };
    if(ReGui::SliderInt2("hit_boundaries - Top | Bottom", tb, 0, static_cast<int>(getSize().y), "inset: %d", ImGuiSliderFlags_AlwaysClamp))
      fEdited = true;

    float *lr[] = { &fHitBoundaries.fLeftInset, &fHitBoundaries.fRightInset };
    if(ReGui::SliderInt2("hit_boundaries - Left | Right", lr, 0, static_cast<int>(getSize().x), "inset: %d", ImGuiSliderFlags_AlwaysClamp))
      fEdited = true;
  }
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fSize = fTexture ? fTexture->frameSize() : ImVec2{100, 100};
  fTexture = nullptr;
  fHitBoundaries = {};
  fEdited = true;
}

//------------------------------------------------------------------------
// Graphics::checkForErrors
//------------------------------------------------------------------------
Attribute::error_t Graphics::checkForErrors(AppContext &iCtx) const
{
  static const Attribute::error_t kOutOfBoundError = "Out of bound";

  auto max = iCtx.getPanelSize();
  auto p = getTopLeft();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    return kOutOfBoundError;
  }
  p = getBottomRight();
  if(p.x < 0 || p.y < 0 || p.x > max.x || p.y > max.y)
  {
    return kOutOfBoundError;
  }
  return Attribute::kNoError;
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
  auto texture = getTexture();
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
// Background::draw
//------------------------------------------------------------------------
bool Background::draw(AppContext &iCtx, Graphics const *iParent) const
{
  if(fProvided)
  {
    switch(iCtx.fShowCustomDisplay)
    {
      case AppContext::ShowCustomDisplay::kBackgroundSD:
      {
        auto texture = iCtx.findTexture(fValue);
        if(texture)
        {
          auto zoom = iCtx.fZoom * iParent->getSize().x / texture->frameWidth();
          texture->draw(iParent->fPosition, iCtx.fZoom, zoom, 0);
          return true;
        }
        break;
      }

      case AppContext::ShowCustomDisplay::kBackgroundHD:
      {
        auto texture = iCtx.findHDTexture(fValue);
        if(texture)
        {
          iCtx.drawTexture(texture.get(), iParent->fPosition);
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
  static const auto kBackgroundFilter = [](FilmStrip const &f) { return f.numFrames() == 1; };

  if(ImGui::Button("X"))
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