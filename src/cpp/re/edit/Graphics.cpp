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

namespace re::edit::widget::attribute {

//------------------------------------------------------------------------
// Graphics::draw
//------------------------------------------------------------------------
void Graphics::draw(DrawContext &iCtx, int iFrameNumber, ImVec4 const &iBorderCol) const
{
  if(hasTexture())
    iCtx.drawTexture(getTexture(), fPosition, iFrameNumber, iBorderCol);
  else
  {
    iCtx.drawRectFilled(fPosition, getSize(), ImVec4{1, 0, 0, 1});
    if(iBorderCol.w > 0.0f)
      iCtx.drawRect(fPosition, getSize(), iBorderCol);
  }
}

//------------------------------------------------------------------------
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(EditContext &iCtx,
                        FilmStrip::Filter const &iFilter,
                        std::function<void()> const &iOnReset,
                        std::function<void(std::string const &)> const &iOnTextureUpdate,
                        std::function<void(ImVec2 const &)> const &iOnSizeUpdate) const
{

  if(iOnReset)
  {
    if(ImGui::Button("X"))
      iOnReset();
    ImGui::SameLine();
  }

  ImGui::BeginGroup();
  auto const *texture = getTexture();
  auto key = texture ? texture->key() : "";
  if(ImGui::BeginCombo(fName.c_str(), key.c_str()))
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
// Graphics::editView
//------------------------------------------------------------------------
void Graphics::editView(EditContext &iCtx)
{
  return editView(iCtx,
                  fFilter,
                  {},
                  [this, &iCtx](auto &k) {
                    fTexture = iCtx.getTexture(k);
                  },
                  [this](auto &s) { fSize = s; }
  );
}

//------------------------------------------------------------------------
// Graphics::reset
//------------------------------------------------------------------------
void Graphics::reset()
{
  fSize = fTexture ? fTexture->frameSize() : ImVec2{100, 100};
  fTexture = nullptr;
}

//------------------------------------------------------------------------
// Graphics::hdgui2D
//------------------------------------------------------------------------
void Graphics::hdgui2D(std::string const &iNodeName, attribute_list_t &oAttributes) const
{
  oAttributes.emplace_back(attribute_t{fName, re::mock::fmt::printf("{ node = { \"%s\" } }", iNodeName)});
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

}