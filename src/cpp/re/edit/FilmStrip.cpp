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
#include "external/stb_image_resize.h"
#include <regex>
#include <fstream>
#include <sstream>

extern "C" const char *stbi_failure_reason(void);

namespace re::edit {

namespace BuiltIns {
char const *getCompressedDataBase85(FilmStrip::key_t const &iKey);
}

namespace impl {
static void Decode85(const unsigned char* src, unsigned char* dst);
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
}

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::shared_ptr<Source> iSource, char const *iErrorMessage) :
  fSource{std::move(iSource)},
  fImage{},
  fErrorMessage{iErrorMessage}
{
//  RE_EDIT_LOG_DEBUG("%p | Error: FilmStrip::FilmStrip(%s) : %s", this, fSource->fKey, iErrorMessage);
}

//------------------------------------------------------------------------
// FilmStrip::FilmStrip
//------------------------------------------------------------------------
FilmStrip::FilmStrip(std::shared_ptr<Source> iSource, RLImageRGBA8 &&iImage) :
  fSource{std::move(iSource)},
  fImage{std::move(iImage)},
  fErrorMessage{}
{
//  RE_EDIT_LOG_DEBUG("%p | FilmStrip::FilmStrip(%s)", this, fSource->fKey);
}

//------------------------------------------------------------------------
// FilmStrip::~FilmStrip
//------------------------------------------------------------------------
FilmStrip::~FilmStrip()
{
//  RE_EDIT_LOG_DEBUG("%p | ~FilmStrip::FilmStrip(%s)", this, fSource->fKey);
}

namespace impl {


//------------------------------------------------------------------------
// impl::loadCompressedBase85
//------------------------------------------------------------------------
std::vector<unsigned char> loadCompressedBase85(char const *iCompressedBase85)
{
  // base85 => compressed binary
  auto compressedSize = ((strlen(iCompressedBase85) + 4) / 5) * 4;
  std::vector<unsigned char> compressedData(static_cast<size_t>(compressedSize));
  impl::Decode85((const unsigned char*) iCompressedBase85, compressedData.data());

  // compressed binary => binary
  const unsigned int buf_decompressed_size = impl::stb_decompress_length((const unsigned char*) compressedData.data());
  std::vector<unsigned char> decompressedData(static_cast<size_t>(buf_decompressed_size));
  impl::stb_decompress(decompressedData.data(), compressedData.data(), static_cast<unsigned int>(compressedData.size()));

  return decompressedData;
}

}

//------------------------------------------------------------------------
// FilmStrip::loadBuiltInCompressedBase85
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStrip::loadBuiltInCompressedBase85(std::shared_ptr<Source> const &iSource)
{
  auto decompressedData = impl::loadCompressedBase85(iSource->getBuiltIn().fCompressedDataBase85);
  RLImageRGBA8 image{LoadImageFromMemory(".png", decompressedData.data(), static_cast<int>(decompressedData.size()))};
  RE_EDIT_INTERNAL_ASSERT(image.isValid(), "%s", stbi_failure_reason());
  return std::unique_ptr<FilmStrip>(new FilmStrip(iSource, std::move(image)));
}

//------------------------------------------------------------------------
// FilmStrip::load
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStrip::load(std::shared_ptr<Source> const &iSource)
{
  RE_EDIT_ASSERT(iSource->fNumFrames > 0);

  if(iSource->hasPath())
  {
    RLImageRGBA8 image{LoadImage(iSource->getPath().u8string().c_str())};
    if(image.isValid())
    {
      return std::unique_ptr<FilmStrip>(new FilmStrip(iSource, std::move(image)));
    }
    else
    {
      auto reason = stbi_failure_reason();
      auto error = reason ? std::string(reason) : fmt::printf("File not found %s", iSource->getPath().u8string());
      RE_EDIT_LOG_ERROR("Error loading file [%s] | %s", iSource->getPath().u8string(), error);
      return std::unique_ptr<FilmStrip>(new FilmStrip(iSource, error.c_str()));
    }
  }
  else
  {
    return loadBuiltInCompressedBase85(iSource);
  }
}


//------------------------------------------------------------------------
// FilmStripMgr::FilmStripMgr
//------------------------------------------------------------------------
FilmStripMgr::FilmStripMgr(std::vector<BuiltIns::Def> const &iBuiltIns,
                           std::optional<fs::path> iDirectory) :
  fDirectory{std::move(iDirectory)}
{
  for(auto &def: iBuiltIns)
  {
    FilmStrip::key_t key{def.fKey};
    fBuiltIns[key] = { def.fNumFrames, BuiltIns::getCompressedDataBase85(key) };
    fSources[key] = toSource(key, fBuiltIns[key]);
  }
}

//------------------------------------------------------------------------
// FilmStripMgr::overrideNumFrames
//------------------------------------------------------------------------
int FilmStrip::overrideNumFrames(int iNumFrames)
{
  if(iNumFrames < 1)
    iNumFrames = 1;

  auto res = fNumFrames;
  fNumFrames = iNumFrames;
  return res;
}

//------------------------------------------------------------------------
// operator<< JboxColor3
//------------------------------------------------------------------------
static inline std::ostream &operator<<(std::ostream &o, JboxColor3 const &h)
{
  auto flags = o.flags();
  o << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << h.fRed << std::setw(2) << h.fGreen << std::setw(2) << h.fBlue;
  o.flags(flags);
  return o;
}


//------------------------------------------------------------------------
// FilmStrip::computeKey
//------------------------------------------------------------------------
FilmStrip::key_t FilmStrip::computeKey(FilmStrip::key_t const &iKey, int iNumFrames, texture::FX const &iEffects)
{
  static const std::regex kKeyRegex{"(_?([0-9]+)_?frames)?$", std::regex_constants::icase};

  std::string_view base = iKey;

  std::cmatch m;
  if(std::regex_search(iKey.c_str(), m, kKeyRegex))
  {
    if(m[2].matched)
    {
      auto numFrames = m[1].str();
      base.remove_suffix(numFrames.size());
    }
  }

  std::ostringstream s{};

  s << base;

  if(iEffects.hasAny())
  {
    if(iEffects.hasTint())
    {
      s << "_T" << ReGui::GetJboxColor3(iEffects.fTint);
    }

    if(iEffects.hasBrightness())
    {
      if(iEffects.fBrightness < 0)
        s << "_b" << -iEffects.fBrightness;
      else
        s << "_B" << iEffects.fBrightness;
    }

    if(iEffects.hasContrast())
    {
      if(iEffects.fContrast < 0)
        s << "_c" << -iEffects.fContrast;
      else
        s << "_C" << iEffects.fContrast;
    }

    if(iEffects.isFlippedX())
      s << "_X";

    if(iEffects.isFlippedY())
      s << "_Y";

    if(iEffects.hasSizeOverride())
    {
      s << "_S"
        << static_cast<int>(std::roundf(iEffects.fSizeOverride->x))
        << "x"
        << static_cast<int>(std::roundf(iEffects.fSizeOverride->y));
    }
  }

  s << "_"
    << iNumFrames
    << "frames";

  return s.str();
}

namespace impl {

/**
 * Creates a new image (allocates size) but does not initialize it */
Image NewImageRGBA8(int width, int height)
{
  return {
    /* .data    = */ RL_CALLOC(width * height, sizeof(Color)),
    /* .width   = */ width,
    /* .height  = */ height,
    /* .mipmaps = */ 1,
    /* .format  = */ PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
  };
}

/**
 * This version, not part of raylib, resizes the image into a new image already pre-allocated */
void ImageRGBA8Resize(Image image, Image newImage)
{
  RE_EDIT_ASSERT(image.format == RLImageRGBA8::kPixelFormat);
  stbir_resize_uint8(static_cast<unsigned char *>(image.data), image.width, image.height, 0,
                     static_cast<unsigned char *>(newImage.data), newImage.width, newImage.height, 0, 4);
}

/**
 * This version, not part of raylib, flips the image into a new image already pre-allocated */
void ImageRGBA8Flip(Image image, Image newImage)
{
  RE_EDIT_ASSERT(image.format == RLImageRGBA8::kPixelFormat);
  RE_EDIT_ASSERT(image.width == newImage.width && image.height == newImage.height);

  auto widthInBytes = image.width * RLImageRGBA8::kBytesPerPixel;

  for (int i = (image.height - 1), offsetSize = 0; i >= 0; i--)
  {
    memcpy(static_cast<unsigned char *>(newImage.data) + offsetSize,
           static_cast<unsigned char *>(image.data) + i * widthInBytes,
           widthInBytes);
    offsetSize += widthInBytes;
  }
}

}

//------------------------------------------------------------------------
// FilmStrip::applyEffects
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStrip::applyEffects(texture::FX const &iEffects) const
{
  RE_EDIT_ASSERT(fImage.isValid());

  auto image = fImage.clone();

  if(iEffects.hasTint())
    ImageColorTint(image.rlImagePtr(), ReGui::GetRLColor(iEffects.fTint));

  if(iEffects.hasBrightness())
    ImageColorBrightness(image.rlImagePtr(), iEffects.fBrightness);

  if(iEffects.hasContrast())
    ImageColorContrast(image.rlImagePtr(), static_cast<float>(iEffects.fContrast));

  if(iEffects.isFlippedX())
    ImageFlipHorizontal(image.rlImagePtr());

  if(iEffects.isFlippedY())
  {
    if(numFrames() == 1)
      ImageFlipVertical(image.rlImagePtr());
    else
    {
      // Implementation note: we need to flip each frame. Using a more efficient API than raylib
      RLImageRGBA8 newImage{image.width(), image.height()};

      auto frameRange = FrameRGBA8Range::create(image, numFrames());
      auto newFrameRange = FrameRGBA8Range::create(newImage, numFrames());

      for(auto frame = frameRange.begin(), newFrame = newFrameRange.begin();
          frame != frameRange.end() && newFrame != newFrameRange.end();
          ++frame, ++newFrame)
      {
        impl::ImageRGBA8Flip(*frame, *newFrame);
      }

      image = std::move(newImage);
    }

  }

  if(iEffects.hasSizeOverride())
  {
    auto newWidth = static_cast<int>(iEffects.fSizeOverride->x);
    auto newFrameHeight = static_cast<int>(iEffects.fSizeOverride->y);

    if(numFrames() == 1)
    {
      ImageResize(image.rlImagePtr(), newWidth, newFrameHeight);
    }
    else
    {
      // Implementation note: resizing the entire image when there are multiple frames would lead to bleeding between
      // frames. So we need to resize each frame separately. The raylib apis would be very inefficient in this instance,
      // thus going down one level and using stbi directly
      RLImageRGBA8 newImage{newWidth, newFrameHeight * numFrames()};

      auto frameRange = FrameRGBA8Range::create(image, numFrames());
      auto newFrameRange = FrameRGBA8Range::create(newImage, numFrames());

      for(auto frame = frameRange.begin(), newFrame = newFrameRange.begin();
          frame != frameRange.end() && newFrame != newFrameRange.end();
          ++frame, ++newFrame)
      {
        impl::ImageRGBA8Resize(*frame, *newFrame);
      }

      image = std::move(newImage);
    }
  }

  return std::unique_ptr<FilmStrip>(new FilmStrip(nullptr, std::move(image)));;
}

//------------------------------------------------------------------------
// FilmStrip::markDeleted
//------------------------------------------------------------------------
void FilmStrip::markDeleted()
{
  fImage = {};
  fErrorMessage = "File has been deleted";
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
    auto iterSource = fSources.find(iKey);
    if(iterSource != fSources.end())
    {
      std::shared_ptr<FilmStrip> filmStrip = FilmStrip::load(iterSource->second);
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
  res.reserve(fFilmStrips.size());
  for(auto const &[k, source]: fSources)
  {
    auto fs = getFilmStrip(k);
    if(fs && fs->isValid() && iFilter(*fs))
      res.emplace_back(k);
  }
  return res;
}


//------------------------------------------------------------------------
// FilmStripMgr::checkKeyMatchesFilter
//------------------------------------------------------------------------
bool FilmStripMgr::checkKeyMatchesFilter(FilmStrip::key_t const &iKey, FilmStrip::Filter const &iFilter) const
{
  if(fSources.find(iKey) != fSources.end())
  {
    auto fs = getFilmStrip(iKey);
    if(fs && fs->isValid() && iFilter(*fs))
      return true;
  }
  return false;
}

//------------------------------------------------------------------------
// FilmStripMgr::getFilmStrip
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip> FilmStripMgr::getFilmStrip(FilmStrip::key_t const &iKey) const
{
  auto iterFS = fFilmStrips.find(iKey);
  if(iterFS != fFilmStrips.end())
    return iterFS->second;


  auto iterSource = fSources.find(iKey);
  // When the key is not found in fSources it means, it is not a valid key (not built in, nor file)
  if(iterSource == fSources.end())
  {
    RE_EDIT_INTERNAL_ASSERT(fDirectory != std::nullopt);
    fSources[iKey] = std::make_shared<FilmStrip::Source>(FilmStrip::Source::from(iKey, *fDirectory));
    iterSource = fSources.find(iKey);
  }

  std::shared_ptr<FilmStrip> filmStrip = FilmStrip::load(iterSource->second);
  fFilmStrips[iKey] = filmStrip;

  return filmStrip;
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
std::set<FilmStrip::key_t> FilmStripMgr::scanDirectory()
{
  auto const sources = fDirectory ? scanDirectory(*fDirectory) : std::vector<FilmStrip::Source>{};
  auto previousSources = fSources;
  std::set<FilmStrip::key_t> modifiedKeys{};

  for(auto const &source: sources)
  {
    previousSources.erase(source.fKey);
    auto s = fSources.find(source.fKey);
    if(s != fSources.end())
    {
      auto const &previousSource = s->second;
      // the source has been modified on disk
      if(source.fLastModifiedTime != previousSource->fLastModifiedTime)
      {
        // this will trigger a "reload"
        fFilmStrips.erase(source.fKey);
        fSources[source.fKey] = std::make_shared<FilmStrip::Source>(source);
        modifiedKeys.emplace(source.fKey);
      }
    }
    else
    {
      fSources[source.fKey] = std::make_shared<FilmStrip::Source>(source);
    }
  }

  // handle remaining sources
  for(auto &[key, source]: previousSources)
  {
    if(source->hasPath())
    {
      fFilmStrips.erase(key);
      modifiedKeys.emplace(key);
      auto builtInIter = fBuiltIns.find(key);
      // there is a builtIn for the file removed... replace
      if(builtInIter != fBuiltIns.end())
        fSources[key] = toSource(key, builtInIter->second);
      else
        fSources[key]->fLastModifiedTime = 0;
    }

    // we don't touch the builtIns
  }

  RE_EDIT_LOG_DEBUG("Scan complete: %ld disk textures (%ld modified)", sources.size(), modifiedKeys.size());

  return modifiedKeys;
}

static const std::regex FILENAME_REGEX{"(([0-9]+)_?frames)?\\.png$", std::regex_constants::icase};

//------------------------------------------------------------------------
// FilmStripMgr::isValidImagePath
//------------------------------------------------------------------------
bool FilmStripMgr::isValidTexturePath(fs::path const &iPath)
{
  std::cmatch m;
  auto filename = iPath.filename().u8string();
  return std::regex_search(iPath.filename().u8string().c_str(), m, FILENAME_REGEX);
}

//------------------------------------------------------------------------
// FilmStripMgr::scanDirectory
//------------------------------------------------------------------------
std::vector<FilmStrip::Source> FilmStripMgr::scanDirectory(fs::path const &iDirectory)
{

  std::vector<FilmStrip::Source> res{};

  if(!fs::exists(iDirectory))
    return res;

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
          res.emplace_back(FilmStrip::Source{ent.path(), key, static_cast<long>(ent.last_write_time().time_since_epoch().count()), inferredNumFrames});
        }
        else
        {
          RE_EDIT_LOG_ERROR("Error with file [%s]: (%d | %s)", ent.path().u8string().c_str(), errorCode.value(), errorCode.message());
        }
      }
    }
  }
  else
  {
    RE_EDIT_LOG_ERROR("Could not scan directory [%s]: (%d | %s)", iDirectory.u8string().c_str(), errorCode.value(), errorCode.message());
  }
  return res;
}

//------------------------------------------------------------------------
// FilmStripMgr::importTexture
//------------------------------------------------------------------------
std::optional<FilmStrip::key_t> FilmStripMgr::importTexture(fs::path const &iTexturePath)
{
  if(!fDirectory)
    return std::nullopt;

  if(fs::is_regular_file(iTexturePath))
  {
    if(iTexturePath.parent_path() != *fDirectory)
    {
      // need to copy
      std::error_code errorCode;
      if(!fs::copy_file(iTexturePath, *fDirectory / iTexturePath.filename(), fs::copy_options::overwrite_existing, errorCode))
      {
        RE_EDIT_LOG_ERROR("Error while copying file [%s]: (%d | %s)", iTexturePath, errorCode.value(), errorCode.message());
        return std::nullopt;
      }
    }

    auto key = iTexturePath.filename().u8string();
    key = key.substr(0, key.size() - 4); // remove .png
    fFilmStrips.erase(key);
    fSources[key] = std::make_shared<FilmStrip::Source>(FilmStrip::Source::from(key, *fDirectory));
    return key; // loads the texture
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// FilmStripMgr::importBuiltIns
//------------------------------------------------------------------------
std::set<FilmStrip::key_t> FilmStripMgr::importBuiltIns(std::set<FilmStrip::key_t> const &iKeys, UserError *oErrors)
{
  std::set<FilmStrip::key_t> modifiedKeys{};

  if(!fDirectory)
    return modifiedKeys;

  for(auto &key: iKeys)
  {
    auto sourceIter = fSources.find(key);
    if(sourceIter != fSources.end() && sourceIter->second->hasBuiltIn())
    {
      auto path = *fDirectory / fmt::printf("%s.png", key);
      // we make sure that the file has not been created in the meantime
      if(!fs::exists(path))
      {
        auto data = impl::loadCompressedBase85(sourceIter->second->getBuiltIn().fCompressedDataBase85);
        std::fstream file(path.u8string().c_str(), std::ios::out | std::ios::binary);
        if(!file)
        {
          if(oErrors)
            oErrors->add("Error writing %s | %s", path.u8string().c_str(), strerror(errno));
          else
            RE_EDIT_LOG_WARNING("Error writing %s | %s", path.u8string().c_str(), strerror(errno));
        }
        else
        {
          auto cdata = static_cast<void *>(data.data());
          file.write(static_cast<char const *>(cdata), static_cast<std::streamsize>(data.size()));
          if(!file)
          {
            if(oErrors)
              oErrors->add("Error writing %s | %s", path.u8string().c_str(), strerror(errno));
            else
              RE_EDIT_LOG_WARNING("Error writing %s | %s", path.u8string().c_str(), strerror(errno));
          }
        }
      }
      fFilmStrips.erase(key);
      fSources[key] = std::make_shared<FilmStrip::Source>(FilmStrip::Source::from(key, *fDirectory));

      modifiedKeys.emplace(key);
    }

  }
  return modifiedKeys;
}


//------------------------------------------------------------------------
// FilmStripMgr::applyEffects
//------------------------------------------------------------------------
std::optional<FilmStrip::key_t> FilmStripMgr::applyEffects(FilmStrip::key_t const &iKey,
                                                           texture::FX const &iEffects,
                                                           UserError *oErrors)
{
  auto filmStrip = findFilmStrip(iKey);
  if(filmStrip && filmStrip->isValid())
  {
    // no effects => return
    if(!iEffects.hasAny())
      return std::nullopt;

    auto keyFX = FilmStrip::computeKey(iKey, filmStrip->numFrames(), iEffects);

    // do we already know about this?
    auto filmStripFX = findFilmStrip(keyFX);
    if(filmStripFX && filmStripFX->isValid())
      return std::nullopt;
    else
    {
      // no we don't so save and add to map
      filmStripFX = save(keyFX, filmStrip->applyEffects(iEffects));
      if(filmStripFX)
      {
        fFilmStrips[keyFX] = filmStripFX;
        return keyFX;
      }
      else
      {
        if(oErrors && fDirectory)
          oErrors->add("Error saving file [%s.png]", (*fDirectory / keyFX).u8string());
      }
    }
  }

  return std::nullopt;
}

//------------------------------------------------------------------------
// FilmStripMgr::applyEffects
//------------------------------------------------------------------------
std::set<FilmStrip::key_t> FilmStripMgr::applyEffects(std::vector<FilmStripFX> const &iEffects, UserError *oErrors)
{
  std::set<FilmStrip::key_t> modifiedKeys{};

  for(auto const &e: iEffects)
  {
    auto keyFX = applyEffects(e.fKey, e.fEffects, oErrors);
    if(keyFX)
      modifiedKeys.emplace(*keyFX);
  }

  return modifiedKeys;
}

//------------------------------------------------------------------------
// FilmStripMgr::toSource
//------------------------------------------------------------------------
std::shared_ptr<FilmStrip::Source> FilmStripMgr::toSource(FilmStrip::key_t const &iKey, BuiltIn const &iBuiltIn)
{
  return std::make_shared<FilmStrip::Source>(FilmStrip::Source {
    iBuiltIn,
    iKey,
    0,
    iBuiltIn.fNumFrames
  });
}

//------------------------------------------------------------------------
// FilmStripMgr::save
//------------------------------------------------------------------------
std::unique_ptr<FilmStrip> FilmStripMgr::save(FilmStrip::key_t const &iKey, std::unique_ptr<FilmStrip> iFilmStrip)
{
  if(!fDirectory)
    return nullptr;

  auto path = *fDirectory / fmt::printf("%s.png", iKey);

  if(!ExportImage(iFilmStrip->rlImage(), path.u8string().c_str()))
  {
    RE_EDIT_LOG_WARNING("Error while saving file [%s]", path.u8string());
    return nullptr;
  }

  auto source = std::make_shared<FilmStrip::Source>(FilmStrip::Source::from(iKey, *fDirectory));
  fSources[iKey] = source;
  iFilmStrip->updateSource(source);

  return std::move(iFilmStrip);
}

//------------------------------------------------------------------------
// FilmStripMgr::remove
//------------------------------------------------------------------------
bool FilmStripMgr::remove(FilmStrip::key_t const &iKey)
{
  auto iterFS = fFilmStrips.find(iKey);
  if(iterFS != fFilmStrips.end())
  {
    auto filmstrip = iterFS->second;
    if(filmstrip->hasPath())
    {
      std::error_code errorCode;
      fs::remove(filmstrip->path(), errorCode);
      if(!errorCode)
      {
        filmstrip->markDeleted();
        fFilmStrips.erase(iKey);
        fSources.erase(iKey);
        return true;
      }
      else
      {
        RE_EDIT_LOG_ERROR("Error while deleting [%s]: (%d | %s)", filmstrip->path().u8string(), errorCode.value(), errorCode.message());
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------
// FilmStripMgr::from
//------------------------------------------------------------------------
FilmStrip::Source FilmStrip::Source::from(key_t const &iKey, fs::path const &iDirectory)
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
    RE_EDIT_LOG_ERROR("Error with file [%s]: (%d | %s)", path.u8string(), errorCode.value(), errorCode.message());
    return {path, iKey, 0, inferredNumFrames};
  }
}

//------------------------------------------------------------------------
// FilmStrip::FrameIterator::FrameIterator
//------------------------------------------------------------------------
FilmStrip::FrameRGBA8Iterator::FrameRGBA8Iterator(Image iImage, int iNumFrames, int iCurrentFrame) :
  fImage{iImage}, fNumFrames{iNumFrames}, fCurrentFrame{iCurrentFrame}
{
  RE_EDIT_ASSERT(iImage.data != nullptr && fImage.format == RLImageRGBA8::kPixelFormat);
  fImage.height = fImage.height / iNumFrames;
  computeCurrentImage();
}

//------------------------------------------------------------------------
// FilmStrip::FrameIterator::computeCurrentImage
//------------------------------------------------------------------------
void FilmStrip::FrameRGBA8Iterator::computeCurrentImage()
{
  if(fCurrentFrame < fNumFrames)
  {
    fCurrentImage = fImage;
    fCurrentImage.data = static_cast<unsigned char *>(fImage.data) + (fImage.width * fImage.height * RLImageRGBA8::kBytesPerPixel * fCurrentFrame);
  }
  else
  {
    fCurrentImage = {};
  }
}

//------------------------------------------------------------------------
// FilmStrip::FrameIterator::operator==
//------------------------------------------------------------------------
bool FilmStrip::FrameRGBA8Iterator::operator==(FilmStrip::FrameRGBA8Iterator const &rhs) const
{
  return fImage == rhs.fImage &&
         fNumFrames == rhs.fNumFrames &&
         fCurrentFrame == rhs.fCurrentFrame;
}

//------------------------------------------------------------------------
// RLImageRGBA8::RLImageRGBA8
//------------------------------------------------------------------------
RLImageRGBA8::RLImageRGBA8(int iWidth, int iHeight) : fImage{impl::NewImageRGBA8(iWidth, iHeight)}
{
  ensureProperFormat();
}

//------------------------------------------------------------------------
// RLImageRGBA8::RLImageRGBA8
//------------------------------------------------------------------------
RLImageRGBA8::RLImageRGBA8() : fImage{
  /* .data    = */ nullptr,
  /* .width   = */ 100,
  /* .height  = */ 100,
  /* .mipmaps = */ 1,
  /* .format  = */ 0 // not a valid format on purpose
}
{
}

//------------------------------------------------------------------------
// RLImageRGBA8::clone
//------------------------------------------------------------------------
RLImageRGBA8 RLImageRGBA8::clone() const
{
  return RLImageRGBA8{ImageCopy(fImage) };
}

namespace impl
{
static constexpr int Decode85Byte(unsigned char c) { return c >= '\\' ? c-36 : c-35; }

static void Decode85(const unsigned char* src, unsigned char* dst)
{
  while (*src)
  {
    unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
    dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
    src += 5;
    dst += 4;
  }
}

static unsigned int stb_decompress_length(const unsigned char *input)
{
  return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

#define stbIn2(x)   ((i[x] << 8) + i[(x)+1])
#define stbIn3(x)   ((i[x] << 16) + stbIn2((x)+1))
#define stbIn4(x)   ((i[x] << 24) + stbIn3((x)+1))

struct stb_decompress_context
{
  unsigned char *barrier_out_e{};
  unsigned char *barrier_out_b{};
  const unsigned char *barrier_in_b{};
  unsigned char *dout{};
};

//static unsigned char *barrier_out_e, *barrier_out_b;
//static const unsigned char *barrier_in_b;
//static unsigned char *dout;

static void stbMatch(stb_decompress_context &ctx, const unsigned char *data, unsigned int length)
{
  // INVERSE of memmove... write each byte before copying the next...
  IM_ASSERT(ctx.dout + length <= ctx.barrier_out_e);
  if (ctx.dout + length > ctx.barrier_out_e) { ctx.dout += length; return; }
  if (data < ctx.barrier_out_b) { ctx.dout = ctx.barrier_out_e+1; return; }
  while (length--) *ctx.dout++ = *data++;
}

static void stbLit(stb_decompress_context &ctx, const unsigned char *data, unsigned int length)
{
  IM_ASSERT(ctx.dout + length <= ctx.barrier_out_e);
  if (ctx.dout + length > ctx.barrier_out_e) { ctx.dout += length; return; }
  if (data < ctx.barrier_in_b) { ctx.dout = ctx.barrier_out_e+1; return; }
  memcpy(ctx.dout, data, length);
  ctx.dout += length;
}

static const unsigned char *stb_decompress_token(stb_decompress_context &ctx, const unsigned char *i)
{
  if (*i >= 0x20) { // use fewer if's for cases that expand small
    if (*i >= 0x80)       stbMatch(ctx, ctx.dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
    else if (*i >= 0x40)  stbMatch(ctx, ctx.dout-(stbIn2(0) - 0x4000 + 1), i[2]+1), i += 3;
    else /* *i >= 0x20 */ stbLit(ctx, i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
  } else { // more ifs for cases that expand large, since overhead is amortized
    if (*i >= 0x18)       stbMatch(ctx, ctx.dout-(stbIn3(0) - 0x180000 + 1), i[3]+1), i += 4;
    else if (*i >= 0x10)  stbMatch(ctx, ctx.dout-(stbIn3(0) - 0x100000 + 1), stbIn2(3)+1), i += 5;
    else if (*i >= 0x08)  stbLit(ctx, i+2, stbIn2(0) - 0x0800 + 1), i += 2 + (stbIn2(0) - 0x0800 + 1);
    else if (*i == 0x07)  stbLit(ctx, i+3, stbIn2(1) + 1), i += 3 + (stbIn2(1) + 1);
    else if (*i == 0x06)  stbMatch(ctx, ctx.dout-(stbIn3(1)+1), i[4]+1), i += 5;
    else if (*i == 0x04)  stbMatch(ctx, ctx.dout-(stbIn3(1)+1), stbIn2(4)+1), i += 6;
  }
  return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
  const unsigned long ADLER_MOD = 65521;
  unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
  unsigned long blocklen = buflen % 5552;

  unsigned long i;
  while (buflen) {
    for (i=0; i + 7 < blocklen; i += 8) {
      s1 += buffer[0], s2 += s1;
      s1 += buffer[1], s2 += s1;
      s1 += buffer[2], s2 += s1;
      s1 += buffer[3], s2 += s1;
      s1 += buffer[4], s2 += s1;
      s1 += buffer[5], s2 += s1;
      s1 += buffer[6], s2 += s1;
      s1 += buffer[7], s2 += s1;

      buffer += 8;
    }

    for (; i < blocklen; ++i)
      s1 += *buffer++, s2 += s1;

    s1 %= ADLER_MOD, s2 %= ADLER_MOD;
    buflen -= blocklen;
    blocklen = 5552;
  }
  return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
  stb_decompress_context ctx{};

  if (stbIn4(0) != 0x57bC0000) return 0;
  if (stbIn4(4) != 0)          return 0; // error! stream is > 4GB
  const unsigned int olen = stb_decompress_length(i);
  ctx.barrier_in_b = i;
  ctx.barrier_out_e = output + olen;
  ctx.barrier_out_b = output;
  i += 16;

  ctx.dout = output;
  for (;;) {
    const unsigned char *old_i = i;
    i = stb_decompress_token(ctx, i);
    if (i == old_i) {
      if (*i == 0x05 && i[1] == 0xfa) {
        IM_ASSERT(ctx.dout == output + olen);
        if (ctx.dout != output + olen) return 0;
        if (stb_adler32(1, output, olen) != (unsigned int) stbIn4(2))
          return 0;
        return olen;
      } else {
        IM_ASSERT(0); /* NOTREACHED */
        return 0;
      }
    }
    IM_ASSERT(ctx.dout <= output + olen);
    if (ctx.dout > output + olen)
      return 0;
  }
}

}

}