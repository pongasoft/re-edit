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
#include <memory>
#include <vector>
#include <imgui.h>

namespace re::edit {

class FilmStrip
{
public:
  using Filter = std::function<bool(FilmStrip const &iFilmStrip)>;

public:
  struct File
  {
    std::string fPath{};
    std::string fKey{};
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

  constexpr std::string const &key() const { return fFile->fKey; };
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
  explicit FilmStripMgr(std::string iDirectory) : fDirectory{std::move(iDirectory)} {}
  std::shared_ptr<FilmStrip> findFilmStrip(std::string const &iKey) const;
  std::shared_ptr<FilmStrip> getFilmStrip(std::string const &iKey) const;

  size_t scanDirectory();
  std::vector<std::string> const &getKeys() const { return fKeys; };
  std::vector<std::string> findKeys(FilmStrip::Filter const &iFilter) const;

  static std::vector<FilmStrip::File> scanDirectory(std::string const &iDirectory);

private:
  std::string fDirectory;
  mutable std::map<std::string, std::shared_ptr<FilmStrip>> fFilmStrips{};
  std::map<std::string, std::shared_ptr<FilmStrip::File>> fFiles{};
  std::vector<std::string> fKeys{};
};

}

#endif //RE_EDIT_FILMSTRIP_H