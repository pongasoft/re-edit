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
#include "Errors.h"
#include <regex>

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
//  RE_MOCK_LOG_INFO("%p | Error: FilmStrip::FilmStrip(%s) : %s", this, fFile->fKey, iErrorMessage);
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
{
//  RE_MOCK_LOG_INFO("%p | FilmStrip::FilmStrip(%s)", this, fFile->fKey);
}

//------------------------------------------------------------------------
// FilmStrip::~FilmStrip
//------------------------------------------------------------------------
FilmStrip::~FilmStrip()
{
//  RE_MOCK_LOG_INFO("%p | ~FilmStrip::FilmStrip(%s)", this, fFile->fKey);
}

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
  auto data = stbi_load(iFile->fPath.string().c_str(), &width, &height, &channels, 4);
  if(data)
  {
    return std::unique_ptr<FilmStrip>(new FilmStrip(iFile, width, height, std::make_unique<Data>(data)));
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error loading file [%s] | %s", iFile->fPath.c_str(), stbi_failure_reason());
    return std::unique_ptr<FilmStrip>(new FilmStrip(iFile, stbi_failure_reason()));
  }
}

//------------------------------------------------------------------------
// FilmStripMgr::overrideNumFrames
//------------------------------------------------------------------------
void FilmStrip::overrideNumFrames(int iNumFrames)
{
  if(iNumFrames < 1)
    iNumFrames = 1;

  fNumFrames = iNumFrames;
}

//------------------------------------------------------------------------
// FilmStripMgr::findFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::findFilmStrip(FilmStrip::key_t const &iKey) const
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
std::vector<FilmStrip::key_t> FilmStripMgr::findKeys(FilmStrip::Filter const &iFilter) const
{
  std::vector<FilmStrip::key_t> res{};
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
std::shared_ptr<FilmStrip> FilmStripMgr::getFilmStrip(FilmStrip::key_t const &iKey) const
{
  auto iterFS = fFilmStrips.find(iKey);
  if(iterFS != fFilmStrips.end())
    return iterFS->second;

  auto iterFile = fFiles.find(iKey);
  if(iterFile == fFiles.end())
  {
    fFiles[iKey] = std::make_shared<FilmStrip::File>(FilmStrip::File::from(iKey, fDirectory));
    iterFile = fFiles.find(iKey);
  }

  std::shared_ptr<FilmStrip> filmStrip = FilmStrip::load(iterFile->second);
  fFilmStrips[iKey] = filmStrip;

  return filmStrip;
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
std::set<FilmStrip::key_t> FilmStripMgr::scanDirectory()
{
  auto files = scanDirectory(fDirectory);
  std::set<FilmStrip::key_t> previousKeys{fKeys.begin(), fKeys.end()};
  std::set<FilmStrip::key_t> modifiedKeys{};
  fKeys.clear();
  for(auto const &file: files)
  {
    fKeys.emplace_back(file.fKey);
    previousKeys.erase(file.fKey);
    auto previousFile = fFiles.find(file.fKey);
    if(previousFile != fFiles.end())
    {
      // the file has been modified on disk
      if(file.fLastModifiedTime != previousFile->second->fLastModifiedTime)
      {
        // this will trigger a "reload"
        fFilmStrips.erase(file.fKey);
        fFiles[file.fKey] = std::make_shared<FilmStrip::File>(file);
        modifiedKeys.emplace(file.fKey);
      }
    }
    else
    {
      fFiles[file.fKey] = std::make_shared<FilmStrip::File>(file);
    }
  }

  // remove the ones that got deleted
  for(auto &k: previousKeys)
  {
    // this will trigger a "reload" which will fail
    fFiles[k]->fLastModifiedTime = 0;
    fFilmStrips.erase(k);
  }

  RE_EDIT_LOG_DEBUG("Scan complete: %ld textures (%ld modified, %ld removed)", fFiles.size(), modifiedKeys.size(), previousKeys.size());

  if(!modifiedKeys.empty())
  {
    previousKeys.insert(modifiedKeys.begin(), modifiedKeys.end());
  }

  return previousKeys;
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
std::vector<FilmStrip::File> FilmStripMgr::scanDirectory(fs::path const &iDirectory)
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
      auto filename = ent.path().filename().u8string();
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
    RE_EDIT_LOG_ERROR("Could not scan directory [%s]: (%d | %s)", iDirectory.c_str(), errorCode.value(), errorCode.message());
  }
  return res;
}

//------------------------------------------------------------------------
// FilmStripMgr::importTexture
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::importTexture(fs::path const &iTexturePath)
{
  if(fs::is_regular_file(iTexturePath))
  {
    // already there... skipping
    if(iTexturePath.parent_path() == fDirectory)
      return nullptr;

    std::error_code errorCode;
    if(fs::copy_file(iTexturePath, fDirectory / iTexturePath.filename(), fs::copy_options::overwrite_existing, errorCode))
    {
      auto key = iTexturePath.filename().u8string();
      key = key.substr(0, key.size() - 4); // remove .png
      std::set<FilmStrip::key_t> keys{fKeys.begin(), fKeys.end()};
      keys.emplace(key);
      fKeys.clear();
      std::copy(keys.begin(), keys.end(), std::back_inserter(fKeys));
      fFilmStrips.erase(key);
      return getFilmStrip(key); // loads the texture
    }
    else
    {
      RE_EDIT_LOG_ERROR("Error while copying file [%s]: (%d | %s)", iTexturePath, errorCode.value(), errorCode.message());
    }
  }
  return nullptr;
}


//------------------------------------------------------------------------
// FilmStripMgr::from
//------------------------------------------------------------------------
FilmStrip::File FilmStrip::File::from(key_t const &iKey, fs::path const &iDirectory)
{
  static const std::regex KEY_REGEX{"(([0-9]+)_?frames)?$", std::regex_constants::icase};

  auto path = iDirectory / fmt::printf("%s.png", iKey);

  std::cmatch m;
  std::regex_search(iKey.c_str(), m, KEY_REGEX);
  auto inferredNumFrames = m[2].matched ? std::stoi(m[2].str()) : 1;

  std::error_code errorCode;
  auto lastWriteTime = fs::last_write_time(path, errorCode);
  if(!errorCode)
  {
    return {path, iKey, static_cast<long>(lastWriteTime.time_since_epoch().count()), inferredNumFrames};
  }
  else
  {
    RE_EDIT_LOG_ERROR("Error with file [%s]: (%d | %s)", path, errorCode.value(), errorCode.message());
    return {path, iKey, 0, inferredNumFrames};
  }
}
}