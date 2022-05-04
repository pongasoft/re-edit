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
#include "logging/logging.h"

namespace re::edit {

class FilmStrip
{
public:
  using data_t = stbi_uc;

  FilmStrip(FilmStrip const &) = delete;
  FilmStrip(FilmStrip &&) = delete;
  ~FilmStrip();

  constexpr std::string const &path() const { return fPath; };
  constexpr std::string const &errorMessage() const { return fErrorMessage; };

  constexpr bool isValid() const { return fData != nullptr; }

  constexpr int width() const { return fWidth; }
  constexpr int height() const { return fHeight; }
  constexpr int numFrames() const { return fNumFrames; }

  constexpr int frameWidth() const { return fWidth; }
  constexpr int frameHeight() const { return fHeight / fNumFrames; }

  constexpr data_t const *data() const { return fData; }
  constexpr data_t const *data(int iFrameNumber) const {
    DCHECK_F(iFrameNumber < numFrames());
    return fData + iFrameNumber * frameWidth() * 4 * frameHeight();
  }

  static std::unique_ptr<FilmStrip> load(char const *iPath, int iNumFrames);

private:
  FilmStrip(std::string iPath, int iNumFrames, char const *iErrorMessage);
  FilmStrip(std::string iPath, int iWidth, int iHeight, int iNumFrames, data_t *iData);

private:
  std::string fPath;
  int fWidth{};
  int fHeight{};
  int fNumFrames{};
  data_t *fData{};

  std::string fErrorMessage;
};

class FilmStripMgr
{
public:
  void addFilmStrip(char const *iPath, int iNumFrames);
  bool maybeAddFilmStrip(char const *iPath, int iNumFrames);
  std::shared_ptr<FilmStrip> findFilmStrip(std::string const &iPath) const;
  std::shared_ptr<FilmStrip> getFilmStrip(std::string const &iPath) const;

private:
  std::map<std::string, std::shared_ptr<FilmStrip>> fFilmStrips{};
};

}

#endif //RE_EDIT_FILMSTRIP_H