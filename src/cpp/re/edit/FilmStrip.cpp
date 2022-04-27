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

#include "FilmStrip.h"

namespace re::edit {

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::string iPath, int iNumFrames, char const *iErrorMessage) :
  fPath{std::move(iPath)},
  fWidth{},
  fHeight{},
  fNumFrames{iNumFrames},
  fData{},
  fErrorMessage{iErrorMessage}
{
}

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::string iPath, int iWidth, int iHeight, int iNumFrames, data_t *iData) :
  fPath{std::move(iPath)},
  fWidth{iWidth},
  fHeight{iHeight},
  fNumFrames{iNumFrames},
  fData{iData},
  fErrorMessage{}
{}

//------------------------------------------------------------------------
// FilmStrip::~FilmStrip
//------------------------------------------------------------------------
FilmStrip::~FilmStrip()
{
  stbi_image_free(fData);
  fData = nullptr;
}

//------------------------------------------------------------------------
// FilmStrip::load
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStrip::load(char const *iPath, int iNumFrames)
{
  DCHECK_F(iNumFrames > 0);

  int width, height, channels;
  auto data = stbi_load(iPath, &width, &height, &channels, 4);
  if(data)
  {
    return std::unique_ptr<FilmStrip>(new FilmStrip(iPath, width, height, iNumFrames, data));
  }
  else
  {
    return std::unique_ptr<FilmStrip>(new FilmStrip(iPath, iNumFrames, stbi_failure_reason()));
  }
}

//------------------------------------------------------------------------
// FilmStrip::addFilmStrip
//------------------------------------------------------------------------
void FilmStripMgr::addFilmStrip(char const *iPath, int iNumFrames)
{
  DCHECK_F(iNumFrames > 0);
  DCHECK_F(fFilmStrips.find(iPath) == fFilmStrips.end());

  auto fs = FilmStrip::load(iPath, iNumFrames);
  fFilmStrips[iPath] = std::move(fs);
}

//------------------------------------------------------------------------
// FilmStrip::maybeAddFilmStrip
//------------------------------------------------------------------------
bool FilmStripMgr::maybeAddFilmStrip(char const *iPath, int iNumFrames)
{
  auto iterFS = fFilmStrips.find(iPath);
  if(iterFS == fFilmStrips.end())
  {
    addFilmStrip(iPath, iNumFrames);
    return true;
  }
  DCHECK_F(iterFS->second->numFrames() == iNumFrames);
  return false;
}

//------------------------------------------------------------------------
// FilmStrip::findFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::findFilmStrip(std::string const &iPath) const
{
  auto iterFS = fFilmStrips.find(iPath);
  if(iterFS != fFilmStrips.end())
    return iterFS->second;
  else
    return nullptr;
}

//------------------------------------------------------------------------
// FilmStrip::getFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::getFilmStrip(std::string const &iPath) const
{
  auto fs = findFilmStrip(iPath);
  DCHECK_F(fs != nullptr);
  return fs;
}

}