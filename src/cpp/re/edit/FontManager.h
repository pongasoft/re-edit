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

#ifndef RE_EDIT_FONT_MANAGER_H
#define RE_EDIT_FONT_MANAGER_H

#include <variant>
#include <string>
#include <optional>

#include "TextureManager.h"

namespace re::edit {

enum class BuiltInFont
{
  kJetBrainsMonoRegular
};

struct FontDef
{
  std::string fName{};
  std::variant<BuiltInFont, std::string> fSource{};
  float fSize{13.0f};
};

class FontManager
{
public:

public:
  explicit FontManager(std::shared_ptr<TextureManager> iTextureManager) : fTextureManager{std::move(iTextureManager)}
  {}

  FontDef getCurrentFont() const { return fCurrentFont; }
  void requestNewFont(FontDef const &iFont) { fNewFontRequest = iFont; }
  bool hasNewFontRequest() const { return fNewFontRequest.has_value(); }
  void applyNewFontRequest();

protected:
  void loadCompressedBase85Font(char const *iCompressedData, float iSize) const;
  void loadFontFromFile(char const *iFontFilename, float iSize) const;
  bool loadFont(float iSize, const std::function<bool (float iSizePixels, const ImFontConfig* iFontCfg)>& iFontLoader) const;
  void setCurrentFont(FontDef const &iFont);

private:
  std::shared_ptr<TextureManager> fTextureManager{};
  FontDef fCurrentFont{};
  std::optional<FontDef> fNewFontRequest{};
};

}

#endif //RE_EDIT_FONT_MANAGER_H