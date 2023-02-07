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

#ifndef RE_EDIT_TEXTURE_MANAGER_H
#define RE_EDIT_TEXTURE_MANAGER_H

#include "FilmStrip.h"
#include "Texture.h"
#include <map>
#include <memory>

namespace re::edit {

class TextureManager
{
public:
  TextureManager() = default;
  virtual ~TextureManager() = default;

  void init(std::vector<BuiltIns::Def> const &iBuiltIns, std::optional<fs::path> iDirectory = std::nullopt);

  std::shared_ptr<Texture> getTexture(std::string const &iKey) const;
  std::shared_ptr<Texture> findTexture(std::string const &iKey) const;
  std::shared_ptr<Texture> findHDTexture(std::string const &iKey) const;

//  bool loadFilmStrip(char const *iPath, int iNumFrames = 1) { return fFilmStripMgr.maybeAddFilmStrip(iPath, iNumFrames); }

  void scanDirectory();
  inline std::vector<std::string> getTextureKeys() const { return fFilmStripMgr->getKeys(); };
  inline std::vector<std::string> findTextureKeys(FilmStrip::Filter const &iFilter) const { return fFilmStripMgr->findKeys(iFilter); }
  inline bool checkTextureKeyMatchesFilter(FilmStrip::key_t const &iKey, FilmStrip::Filter const &iFilter) const { return fFilmStripMgr->checkKeyMatchesFilter(iKey, iFilter); }
  void overrideNumFrames(std::map<std::string, int> const &iNumFrames) const;
  int overrideNumFrames(std::string const &iKey, int iNumFrames) const;
  std::optional<FilmStrip::key_t> importTexture(fs::path const &iTexturePath);
  void importBuiltIns(std::set<FilmStrip::key_t> const &iKeys, UserError *oErrors = nullptr);

protected:
  virtual std::unique_ptr<Texture> createTexture() const = 0;
  virtual void populateTexture(std::shared_ptr<Texture> const &iTexture) const = 0;
  void updateTexture(FilmStrip::key_t const &iKey);
  std::shared_ptr<Texture> updateTexture(std::shared_ptr<Texture> const &iTexture, std::shared_ptr<FilmStrip> const &iFilmStrip) const;

private:
  std::unique_ptr<FilmStripMgr> fFilmStripMgr{};
  mutable std::map<std::string, std::shared_ptr<Texture>> fTextures{};
};

}

#endif //RE_EDIT_TEXTURE_MANAGER_H