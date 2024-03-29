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

#include "FontManager.h"
#include <IconsFAReEdit.h>
#include <IconsFAReEditCustom.h>
#include "Errors.h"
#include <rlImGui.h>

#include <IconsFAReEdit.cpp>
#include <IconsFAReEditCustom.cpp>
#include <JetBrainsMono-Regular.cpp>

namespace re::edit {

namespace impl {

//------------------------------------------------------------------------
// ::mergeFontAwesome
//------------------------------------------------------------------------
static void mergeFontAwesome(float iSize)
{
  auto &io = ImGui::GetIO();
  static const ImWchar icons_ranges[] = {fa::kMin, fa::kMax16, 0};
  static const ImWchar custom_icons_ranges[] = {fac::kMin, fac::kMax16, 0};
  ImFontConfig icons_config;
  icons_config.GlyphOffset = {0, 1};
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.OversampleH = 2;
  icons_config.FontDataOwnedByAtlas = false;
  icons_config.GlyphMinAdvanceX = iSize; // to make it monospace

  io.Fonts->AddFontFromMemoryCompressedBase85TTF(IconsFAReEdit_compressed_data_base85,
                                                 iSize,
                                                 &icons_config,
                                                 icons_ranges);

  io.Fonts->AddFontFromMemoryCompressedBase85TTF(IconsFAReEditCustom_compressed_data_base85,
                                                 iSize,
                                                 &icons_config,
                                                 custom_icons_ranges);

//  io.Fonts->AddFontFromFileTTF("/Volumes/Vault/Downloads/fontawesome-pro-6.2.0-web/webfonts/fa-solid-900.ttf",
//                               iSize,
//                               &icons_config,
//                               icons_ranges);

}

}

//------------------------------------------------------------------------
// FontManager::setCurrentFont
//------------------------------------------------------------------------
void FontManager::setCurrentFont(FontDef const &iFont)
{
//  fNativeFontManager->destroyFontsTexture();

  if(std::holds_alternative<BuiltInFont>(iFont.fSource))
  {
    RE_EDIT_INTERNAL_ASSERT(std::get<BuiltInFont>(iFont.fSource) == BuiltInFont::kJetBrainsMonoRegular);
    loadCompressedBase85Font(JetBrainsMonoRegular_compressed_data_base85, iFont.fSize);
  }

  if(std::holds_alternative<std::string>(iFont.fSource))
  {
    loadFontFromFile(std::get<std::string>(iFont.fSource).c_str(), iFont.fSize);
  }

  rlImGuiReloadFonts();
  fCurrentFont = iFont;
}

//------------------------------------------------------------------------
// FontManager::loadFont
//------------------------------------------------------------------------
bool FontManager::loadFont(float iSize, const std::function<bool(float iSizePixels, const ImFontConfig* iFontCfg)>& iFontLoader) const
{
  auto &io = ImGui::GetIO();
  io.Fonts->Clear();
  auto fontScale = getCurrentFontScale();
  auto size = std::floor(iSize * fontScale * getCurrentFontDpiScale());
  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  bool res = iFontLoader(size, &fontConfig);
  if(res)
  {
    impl::mergeFontAwesome(size);
    io.FontGlobalScale = 1.0f / fontScale;
  }
  else
  {
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.0f;
  }
  return res;
}

//------------------------------------------------------------------------
// FontManager::requestNewFont
//------------------------------------------------------------------------
void FontManager::requestNewFont(FontDef const &iFont)
{
  if(fFontChangeRequest)
    fFontChangeRequest = FontChangeRequest{iFont, fFontChangeRequest->fFontScale, fFontChangeRequest->fFontDpiScale};
  else
  {
    if(fCurrentFont != iFont)
      fFontChangeRequest = FontChangeRequest{iFont, fCurrentFontScale, fCurrentFontDpiScale};
  }
}

//------------------------------------------------------------------------
// FontManager::loadCompressedBase85Font
//------------------------------------------------------------------------
void FontManager::loadCompressedBase85Font(char const *iCompressedData, float iSize) const
{
  loadFont(iSize, [iCompressedData](float iSizePixels, const ImFontConfig* iFontCfg){
    auto &io = ImGui::GetIO();
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(iCompressedData, iSizePixels, iFontCfg);
    return true;
  });
}

//------------------------------------------------------------------------
// FontManager::loadFontFromFile
//------------------------------------------------------------------------
void FontManager::loadFontFromFile(char const *iFontFilename, float iSize) const
{
  loadFont(iSize, [iFontFilename](float iSizePixels, const ImFontConfig* iFontCfg){
    auto &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(iFontFilename, iSizePixels, iFontCfg);
    return true;
  });
}

//------------------------------------------------------------------------
// FontManager::applyFontChangeRequest
//------------------------------------------------------------------------
void FontManager::applyFontChangeRequest()
{
  RE_EDIT_INTERNAL_ASSERT(hasFontChangeRequest());
  auto fontChangeRequest = *fFontChangeRequest;
  fFontChangeRequest = std::nullopt;
  fCurrentFontScale = fontChangeRequest.fFontScale;
  fCurrentFontDpiScale = fontChangeRequest.fFontDpiScale;
  setCurrentFont(fontChangeRequest.fFontDef);
}

//------------------------------------------------------------------------
// FontManager::setFontScale
//------------------------------------------------------------------------
void FontManager::setFontScale(float iFontScale)
{
  if(fCurrentFontScale != iFontScale)
  {
    if(fFontChangeRequest)
      fFontChangeRequest = FontChangeRequest{fFontChangeRequest->fFontDef, iFontScale, fFontChangeRequest->fFontDpiScale};
    else
      fFontChangeRequest = FontChangeRequest{getCurrentFont(), iFontScale, getCurrentFontDpiScale()};
  }
}

//------------------------------------------------------------------------
// FontManager::setDpiFontScale
//------------------------------------------------------------------------
void FontManager::setDpiFontScale(float iFontDpiScale)
{
  if(iFontDpiScale != fCurrentFontDpiScale)
  {
    if(fFontChangeRequest)
      fFontChangeRequest = FontChangeRequest{fFontChangeRequest->fFontDef, fFontChangeRequest->fFontScale, iFontDpiScale};
    else
      fFontChangeRequest = FontChangeRequest{getCurrentFont(), getCurrentFontScale(), iFontDpiScale};
  }
}

/**
#include <regex>
#include <dirent.h>
#include <sys/stat.h>

 std::vector<FontDef> scanFonts(std::string const &iDirectory)
{
  RE_EDIT_ASSERT(!iDirectory.empty());

  static const std::regex FILENAME_REGEX{"\\.ttf$", std::regex_constants::icase};

  std::vector<FontDef> res{};

  DIR *dir;
  struct dirent *ent;
  if((dir = opendir(iDirectory.c_str())) != nullptr)
  {
    while((ent = readdir(dir)) != nullptr)
    {
      std::cmatch m;
      if(std::regex_search(ent->d_name, m, FILENAME_REGEX))
      {
        auto entry = re::mock::fmt::path(iDirectory, ent->d_name);
        struct stat buf{};
        if(stat(entry.c_str(), &buf) == 0)
        {
          auto key = std::string(ent->d_name);
          key = key.substr(0, key.size() - 4); // remove .ttf
          res.emplace_back(FontDef{key, entry});
        }
        else
        {
          RE_EDIT_LOG_ERROR("Error (%d) with file [%s] : ", errno, entry);
        }
      }
    }
    closedir(dir);
  }
  else
  {
    RE_EDIT_LOG_ERROR("Could not scan directory [%s]", iDirectory);
  }

  return res;
}

  auto static kFonts = []() {
    auto fonts = scanFonts("/Library/Fonts");
    auto userFonts = scanFonts("/Users/ypujante/Library/Fonts");
    std::move(userFonts.begin(), userFonts.end(), std::back_inserter(fonts));
    std::sort(fonts.begin(), fonts.end(), [](auto const &f1, auto const &f2) { return f1.fName < f2.fName; });
    return fonts;
  } ();

 */

}