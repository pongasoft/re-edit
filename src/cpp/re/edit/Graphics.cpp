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
  return fmt::printf("{ { %s } }", hasTexture() ? fmt::printf("path = \"%s\"", getTexture()->key()) : "");
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fTextureKey = "";
  fDNZTexture = nullptr;
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
      iCtx.addUndoLambda(fTextureKey, std::string{},
                         "Reset background graphics",
                         [](UndoAction *iAction, auto const &iValue) {
                           auto panel = AppContext::GetCurrent().getPanel(iAction->fPanelType);
                           panel->setBackgroundKey(iValue);
                         });
      reset();
    }

    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ImportImages, "Import")))
    {
      auto textureKey = iCtx.importTextureBlocking();
      if(textureKey)
      {
        iCtx.addUndoLambda(fTextureKey, *textureKey,
                           "Change background graphics",
                           [](UndoAction *iAction, auto const &iValue) {
                             auto panel = AppContext::GetCurrent().getPanel(iAction->fPanelType);
                             panel->setBackgroundKey(iValue);
                           });
        fTextureKey = *textureKey;
        fDNZTexture = iCtx.getTexture(*textureKey);
        fEdited = true;
      }
    }

    ImGui::EndPopup();
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
        fDNZTexture = iCtx.getTexture(p);
        fEdited = true;
      }
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
        ImGui::TextUnformatted(fmt::printf("%dx%d | %d frames",
                                           static_cast<int>(texture->frameWidth()),
                                           static_cast<int>(texture->frameHeight()),
                                           texture->numFrames()).c_str());
      else
        ImGui::TextUnformatted(fmt::printf("%s", texture->getFilmStrip()->errorMessage()).c_str());
    });
  }

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
      if(fFilter)
      {
        if(!iCtx.checkTextureKeyMatchesFilter(texture->key(), fFilter))
          oErrors.add(fFilter.fDescription);
      }
    }
  }
  else
    oErrors.add("Required");
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
void Graphics::draw(AppContext &iCtx, ReGui::Canvas &iCanvas, ImU32 iBorderColor, bool iXRay) const
{
  auto texture = hasTexture() ? getTexture() : nullptr;
  if(texture && texture->isValid())
  {
    iCanvas.addTexture(texture, fPosition, fFrameNumber, iBorderColor, impl::computeTextureColor(iXRay));
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
                        std::function<void(ImVec2 const &)> const &iOnSizeUpdate)
{
  if(ReGui::MenuButton())
    ImGui::OpenPopup("Menu");

  auto numFramesPopup = ImGui::GetID("NumFrames_popup");

  if(ImGui::BeginPopup("Menu"))
  {
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Reset, "Reset")))
    {
      iCtx.addUndoAttributeReset(this);
      reset();
    }

    ImGui::BeginDisabled(hasSize());
    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Frames, "Change number of frames")))
      ImGui::OpenPopup(numFramesPopup);
    ImGui::EndDisabled();

    if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ImportImages, "Import")))
    {
      auto textureKey = iCtx.importTextureBlocking();
      if(textureKey)
        iOnTextureUpdate(*textureKey);
    }

    ImGui::EndPopup();
  }

  if(ImGui::BeginPopup("NumFrames_popup"))
  {
    auto texture = getTexture();
    auto numFrames = texture->numFrames();
    if(ImGui::InputInt("frames", &numFrames, 1, 10))
    {
      iCtx.addOrMergeUndoLambda(&fTexture, texture->numFrames(), numFrames,
                                fmt::printf("Change number of frames (%s)", texture->key()),
                                [key = texture->key()](UndoAction *iAction, auto const &iValue) {
                                  AppContext::GetCurrent().overrideTextureNumFrames(key, iValue);
                                });
      iCtx.overrideTextureNumFrames(texture->key(), numFrames);
      fEdited = true;
    }
    if(ImGui::Button("Ok"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
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

  if(hasTexture() && ReGui::ShowQuickView())
  {
    ReGui::ToolTip([this] {
      auto texture = getTexture();
      ReGui::TextSeparator(texture->key().c_str());
      ImGui::Text("path   = GUI2D/%s.png", texture->key().c_str());
      if(texture->isValid())
      {
        ImGui::Text("size   = %dx%d", static_cast<int>(texture->frameWidth()), static_cast<int>(texture->frameHeight()));
        ImGui::Text("frames = %d", texture->numFrames());
      }
      else
        ImGui::Text("error  = %s", texture->getFilmStrip()->errorMessage().c_str());
    });
  }

  if(hasSize() && fSizeEnabled)
  {
    auto editedSize = std::get<ImVec2>(fTexture);
    if(ReGui::InputInt("w", &editedSize.x, 1, 5))
    {
      iOnSizeUpdate(editedSize);
    }

    if(ReGui::InputInt("h", &editedSize.y, 1, 5))
    {
      iOnSizeUpdate(editedSize);
    }
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
  if(fDNZTexture && fDNZTexture->isValid())
    fTexture = fDNZTexture->frameSize();
  else
    fTexture = kNoGraphics;
  fDNZTexture = nullptr;
  fHitBoundaries = {};
  fEdited = true;
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
  hdgui2D(AppContext::GetCurrent().getCurrentWidget()->getName(), oAttributes);
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
  {
    auto filmStrip = getTexture()->getFilmStrip();
    if(filmStrip->hasPath())
      oPaths.emplace(filmStrip->path());
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
    auto texture = getTexture();
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
// Graphics::toValueString
//------------------------------------------------------------------------
std::string Graphics::toValueString() const
{
  if(hasTexture())
  {
    auto texture = getTexture();
    return fmt::printf("graphics = \"%s\"", texture->key());
  }
  else
  {
    auto size = getSize();
    return fmt::printf("graphics = { %d, %d }", static_cast<int>(size.x),  static_cast<int>(size.y));
  }
}

//------------------------------------------------------------------------
// Graphics::copyFrom
//------------------------------------------------------------------------
bool Graphics::copyFrom(Attribute const *iFromAttribute)
{
  auto fromAttribute = dynamic_cast<Graphics const *>(iFromAttribute);
  if(fromAttribute)
  {
    fHitBoundaries = fromAttribute->fHitBoundaries;
    fTexture = fromAttribute->fTexture;
    fDNZTexture = fromAttribute->fDNZTexture;
    fEdited = true;
    return true;
  }
  else
    return false;
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
          auto scale = iParent->getSize() / texture->frameSize();
          iCanvas.addScaledTexture(texture.get(), scale, iParent->fPosition, 0, iBorderColor, impl::computeTextureColor(xRay));
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