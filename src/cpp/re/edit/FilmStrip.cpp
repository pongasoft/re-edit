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
#include <sys/stat.h>
#include "Errors.h"
#include <re/mock/fmt.h>
#include <regex>
#include <filesystem>

namespace re::edit {

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::shared_ptr<File> iFile, char const *iErrorMessage) :
  fFile{std::move(iFile)},
  fWidth{},
  fHeight{},
  fData{},
  fErrorMessage{iErrorMessage}
{
}

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::shared_ptr<File> iFile, int iWidth, int iHeight, std::shared_ptr<Data> iData) :
  fFile{std::move(iFile)},
  fWidth{iWidth},
  fHeight{iHeight},
  fData{std::move(iData)},
  fErrorMessage{}
{}

//------------------------------------------------------------------------
// FilmStrip::Data::~Data
//------------------------------------------------------------------------
FilmStrip::Data::~Data()
{
  stbi_image_free(fData);
  fData = nullptr;
}

//------------------------------------------------------------------------
// FilmStrip::load
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStrip::load(std::shared_ptr<File> const &iFile)
{
  RE_EDIT_ASSERT(iFile->fNumFrames > 0);

  int width, height, channels;
  auto data = stbi_load(iFile->fPath.c_str(), &width, &height, &channels, 4);
  if(data)
  {
    return std::unique_ptr<FilmStrip>(new FilmStrip(iFile, width, height, std::make_unique<Data>(data)));
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error loading file [%s] | %s", iFile->fPath, stbi_failure_reason());
    return std::unique_ptr<FilmStrip>(new FilmStrip(iFile, stbi_failure_reason()));
  }
}

//------------------------------------------------------------------------
// FilmStripMgr::overrideNumFrames
//------------------------------------------------------------------------
void FilmStrip::overrideNumFrames(int iNumFrames)
{
  if(fNumFrames != 0 && fNumFrames != iNumFrames)
    RE_EDIT_LOG_WARNING("Mismatch number of frames [%d] and [%d] for [%s]", iNumFrames, fNumFrames, fFile->fKey);

  if(numFrames() != 1 && iNumFrames != 1)
    RE_EDIT_LOG_WARNING("Mismatch number of frames [%d] and [%d] for [%s]", iNumFrames, numFrames(), fFile->fKey);

  fNumFrames = iNumFrames;
}

//------------------------------------------------------------------------
// FilmStripMgr::findFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::findFilmStrip(std::string const &iKey) const
{
  auto iterFS = fFilmStrips.find(iKey);
  if(iterFS != fFilmStrips.end())
    return iterFS->second;
  else
  {
    auto iterFile = fFiles.find(iKey);
    if(iterFile != fFiles.end())
    {
      std::shared_ptr<FilmStrip> filmStrip = FilmStrip::load(iterFile->second);
      fFilmStrips[iKey] = filmStrip;
      return filmStrip;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------
// FilmStripMgr::findKeys
//------------------------------------------------------------------------
std::vector<std::string> FilmStripMgr::findKeys(FilmStrip::Filter const &iFilter) const
{
  std::vector<std::string> res{};
  res.reserve(fKeys.size());
  for(auto &key: fKeys)
  {
    auto fs = findFilmStrip(key);
    if(fs && fs->isValid() && iFilter(*fs))
      res.emplace_back(key);
  }
  return res;
}

//------------------------------------------------------------------------
// FilmStripMgr::getFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::getFilmStrip(std::string const &iKey) const
{
  auto fs = findFilmStrip(iKey);
  RE_EDIT_ASSERT(fs != nullptr);
  return fs;
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
size_t FilmStripMgr::scanDirectory()
{
  auto files = scanDirectory(fDirectory);
  fKeys.clear();
  for(auto const &file: files)
  {
    fKeys.emplace_back(file.fKey);
    auto previousFile = fFiles.find(file.fKey);
    if(previousFile != fFiles.end())
    {
      // the file has been modified on disk
      if(file.fLastModifiedTime > previousFile->second->fLastModifiedTime)
      {
        // this will trigger a "reload"
        fFilmStrips.erase(file.fKey);
        fFiles[file.fKey] = std::make_shared<FilmStrip::File>(file);
      }
    }
    else
    {
      fFiles[file.fKey] = std::make_shared<FilmStrip::File>(file);
    }
  }
  return fFiles.size();
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
std::vector<FilmStrip::File> FilmStripMgr::scanDirectory(std::string const &iDirectory)
{
  RE_EDIT_ASSERT(!iDirectory.empty());

  static const std::regex FILENAME_REGEX{"(([0-9]+)_?frames)?\\.png$", std::regex_constants::icase};

  std::vector<FilmStrip::File> res{};

  std::error_code errorCode;
  auto iter = std::filesystem::directory_iterator(iDirectory, errorCode);
  if(!errorCode)
  {
    for(const auto &ent: iter)
    {
      std::cmatch m;
      auto filename = ent.path().filename().string();
      if(std::regex_search(filename.c_str(), m, FILENAME_REGEX))
      {
        if(ent.exists(errorCode))
        {
          auto inferredNumFrames = m[2].matched ? std::stoi(m[2].str()) : 1;
          auto key = filename;
          key = key.substr(0, key.size() - 4); // remove .png
          res.emplace_back(FilmStrip::File{ent.path(), key, static_cast<long>(ent.last_write_time().time_since_epoch().count()), inferredNumFrames});
        }
        else
        {
          RE_EDIT_LOG_ERROR("Error with file [%s]: (%d | %s)", ent.path().c_str(), errorCode.value(), errorCode.message());
        }
      }
    }
  }
  else
  {
    RE_EDIT_LOG_ERROR("Could not scan directory [%s]: (%d | %s)", iDirectory, errorCode.value(), errorCode.message());
  }
  return res;
}


}