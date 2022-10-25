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

#ifndef RE_EDIT_FILMSTRIP_H
#define RE_EDIT_FILMSTRIP_H

#include <string>
#include <stb_image.h>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <functional>
#include <imgui.h>
#include "fs.h"

namespace re::edit {

class FilmStrip
{
public:
  using Filter = std::function<bool(FilmStrip const &iFilmStrip)>;
  using key_t = std::string;

public:
  struct File
  {
    fs::path fPath{};
    key_t fKey{};
    long fLastModifiedTime{};
    int fNumFrames{1};
  };

public:
  using data_t = stbi_uc;

  struct Data
  {
    explicit Data(data_t *iData) : fData{iData} {}
    ~Data();

    constexpr data_t const *data() const { return fData; }

  private:
    data_t *fData{};
  };

  inline key_t const &key() const { return fFile->fKey; };
  constexpr std::string const &errorMessage() const { return fErrorMessage; };

  inline bool isValid() const { return fData != nullptr; }

  constexpr int width() const { return fWidth; }
  constexpr int height() const { return fHeight; }
  constexpr int numFrames() const { return fNumFrames > 0 ? fNumFrames : fFile->fNumFrames; }

  constexpr int frameWidth() const { return fWidth; }
  constexpr int frameHeight() const { return fHeight / numFrames(); }

  constexpr data_t const *data() const { return fData->data(); }

  void overrideNumFrames(int iNumFrames);

  static std::unique_ptr<FilmStrip> load(std::shared_ptr<File> const &iFile);

  static constexpr auto bySizeFilter(ImVec2 const &iSize) {
    return [&iSize](FilmStrip const &iFilmStrip) {
      return iFilmStrip.frameWidth() == static_cast<int>(iSize.x) &&
             iFilmStrip.frameHeight() == static_cast<int>(iSize.y) &&
             iFilmStrip.numFrames() == 1;
    };
  }

  static Filter orFilter(Filter iFilter1, Filter iFilter2) {
    if(!iFilter1)
      return iFilter2;

    if(!iFilter2)
      return iFilter1;

    return [f1 = std::move(iFilter1), f2 = std::move(iFilter2)](FilmStrip const &iFilmStrip) {
      return f1(iFilmStrip) || f2(iFilmStrip);
    };
  }

  ~FilmStrip();

private:
  FilmStrip(std::shared_ptr<File> iFile, char const *iErrorMessage);
  FilmStrip(std::shared_ptr<File> iFile, int iWidth, int iHeight, std::shared_ptr<Data> iData);

private:
  std::shared_ptr<File> fFile;
  int fNumFrames{0};
  int fWidth{};
  int fHeight{};
  std::shared_ptr<Data> fData{};

  std::string fErrorMessage;
};

class FilmStripMgr
{
public:
  explicit FilmStripMgr(fs::path iDirectory) : fDirectory{std::move(iDirectory)} {}
  std::shared_ptr<FilmStrip> findFilmStrip(FilmStrip::key_t const &iKey) const;
  std::shared_ptr<FilmStrip> getFilmStrip(FilmStrip::key_t const &iKey) const;

  std::set<FilmStrip::key_t> scanDirectory();
  std::vector<FilmStrip::key_t> const &getKeys() const { return fKeys; };
  std::vector<FilmStrip::key_t> findKeys(FilmStrip::Filter const &iFilter) const;

  static std::vector<FilmStrip::File> scanDirectory(fs::path const &iDirectory);

private:
  fs::path fDirectory;
  mutable std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip>> fFilmStrips{};
  std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip::File>> fFiles{};
  std::vector<FilmStrip::key_t> fKeys{};
};

}

#endif //RE_EDIT_FILMSTRIP_H