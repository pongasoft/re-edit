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
#include <imgui.h>
#include <cmath>
#include <memory>
#include <functional>

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

  friend bool operator==(FontDef const &lhs, FontDef const &rhs)
  {
    return lhs.fName == rhs.fName &&
           lhs.fSource == rhs.fSource &&
           lhs.fSize == rhs.fSize;
  }

  friend bool operator!=(FontDef const &lhs, FontDef const &rhs)
  {
    return !(rhs == lhs);
  }
};

class FontManager
{
public:

public:
  FontManager() = default;

  inline float getCurrentDpiScaledFontSize() const { return computeDpiScaledFontSize(fCurrentFont.fSize); }
  constexpr float getCurrentFontScale() const { return fCurrentFontScale; }
  constexpr float getCurrentFontDpiScale() const { return fCurrentFontDpiScale; }
  void setFontScale(float iFontScale);
  void setDpiFontScale(float iFontDpiScale);
  FontDef getCurrentFont() const { return fCurrentFont; }
  void requestNewFont(FontDef const &iFont);
  bool hasFontChangeRequest() const { return fFontChangeRequest.has_value(); }
  void applyFontChangeRequest();

protected:
  void loadCompressedBase85Font(char const *iCompressedData, float iSize) const;
  void loadFontFromFile(char const *iFontFilename, float iSize) const;
  bool loadFont(float iSize, const std::function<bool (float iSizePixels, const ImFontConfig* iFontCfg)>& iFontLoader) const;
  void setCurrentFont(FontDef const &iFont);
  inline float computeDpiScaledFontSize(float iFontSize) const { return std::floor(iFontSize * fCurrentFontDpiScale); }

private:
  struct FontChangeRequest
  {
    FontDef fFontDef{};
    float fFontScale{1.0f};
    float fFontDpiScale{1.0f};

  };
private:
  FontDef fCurrentFont{};
  float fCurrentFontScale{1.0f};
  float fCurrentFontDpiScale{1.0f};
  std::optional<FontChangeRequest> fFontChangeRequest{};
};

}

#endif //RE_EDIT_FONT_MANAGER_H