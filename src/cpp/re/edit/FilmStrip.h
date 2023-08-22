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
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <functional>
#include <variant>
#include <imgui.h>
#include "fs.h"
#include <raylib.h>
#include "Constants.h"
#include "Errors.h"
#include "fx.h"

//------------------------------------------------------------------------
// Comparing 2 raylib Image
//------------------------------------------------------------------------
inline bool operator==(Image const &lhs, Image const &rhs)
{
  return lhs.data == rhs.data &&
         lhs.width == rhs.width &&
         lhs.height == rhs.height &&
         lhs.mipmaps == rhs.mipmaps &&
         lhs.format == rhs.format;
}

namespace re::edit {

struct RLImageRGBA8
{
  using data_t = unsigned char;

  static inline constexpr PixelFormat kPixelFormat = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  static inline constexpr int kBytesPerPixel = 4;

  //! Create an invalid 100x100 image (to be used only when there is no image)
  RLImageRGBA8();

  //! Creates an uninitialized image of the provided size
  RLImageRGBA8(int iWidth, int iHeight);

  //! Assumes ownership of the provided image
  explicit RLImageRGBA8(Image iImage) : fImage{iImage} { ensureProperFormat(); }

  ~RLImageRGBA8() { UnloadImage(fImage); }


  RLImageRGBA8(RLImageRGBA8 &&iOther) noexcept : fImage{iOther.fImage}
  {
    iOther.fImage.data = nullptr;
    ensureProperFormat();
  };

  RLImageRGBA8(RLImageRGBA8 const &) = delete;
  RLImageRGBA8 &operator=(RLImageRGBA8 const &) = delete;

  RLImageRGBA8 &operator=(RLImageRGBA8 &&iOther) noexcept
  {
    fImage = iOther.fImage;
    iOther.fImage.data = nullptr;
    ensureProperFormat();
    return *this;
  }

  constexpr bool isValid() const { return fImage.data != nullptr; }
  constexpr int width() const { return fImage.width; }
  constexpr int height() const { return fImage.height; }
  constexpr data_t *data() const { return static_cast<data_t *>(fImage.data); }

  constexpr Image const &rlImageRef() const { return fImage; }
  constexpr Image *rlImagePtr() { return &fImage; }

  RLImageRGBA8 clone() const;

private:
  void ensureProperFormat() { ImageFormat(&fImage, RLImageRGBA8::kPixelFormat); }

private:
  Image fImage;
};

class FilmStrip;

struct BuiltIn
{
  int fNumFrames{1};
  char const *fCompressedDataBase85{};
};

namespace BuiltIns {

struct Def
{
  char const *fKey{};
  int fNumFrames{1};
};

constexpr Def kAudioSocket{"SharedAudioJack_3frames", 3};
constexpr Def kCVSocket{"SharedCVJack_3frames", 3};
constexpr Def kPatchBrowseGroup{"PatchBrowseGroup"};
constexpr Def kPlaceholder{"Placeholder"};
constexpr Def kSampleBrowseGroup{"SampleBrowseGroup"};
constexpr Def kTapeHorizontal{"Tape_Horizontal_1frames"};
constexpr Def kTapeVertical{"Tape_Vertical_1frames"};
constexpr Def kTrimKnob{"TrimKnob"};
constexpr Def kRoutingIcon01{"Routing_Icon_01"};
constexpr Def kRoutingIcon02{"Routing_Icon_02"};
constexpr Def kRoutingIcon03{"Routing_Icon_03"};
constexpr Def kRoutingIcon04{"Routing_Icon_04"};
constexpr Def kRoutingIcon05{"Routing_Icon_05"};
constexpr Def kRoutingIconWhite01{"Routing_Icon_White_01"};
constexpr Def kRoutingIconWhite02{"Routing_Icon_White_02"};
constexpr Def kRoutingIconWhite03{"Routing_Icon_White_03"};
constexpr Def kRoutingIconWhite04{"Routing_Icon_White_04"};
constexpr Def kRoutingIconWhite05{"Routing_Icon_White_05"};

static std::vector<Def> kDeviceBuiltIns{kAudioSocket,
                                        kCVSocket,
                                        kPatchBrowseGroup,
                                        kPlaceholder,
                                        kSampleBrowseGroup,
                                        kTapeHorizontal,
                                        kTapeVertical,
                                        kTrimKnob,
                                        kRoutingIcon01,
                                        kRoutingIcon02,
                                        kRoutingIcon03,
                                        kRoutingIcon04,
                                        kRoutingIcon05,
                                        kRoutingIconWhite01,
                                        kRoutingIconWhite02,
                                        kRoutingIconWhite03,
                                        kRoutingIconWhite04,
                                        kRoutingIconWhite05,
                                        };

constexpr Def kLogoDark{"logo_dark"};
constexpr Def kDeviceType{"DeviceType_4frames", 4};
constexpr Def kFoldButton{"SharedFoldButton_4frames", 4};
constexpr Def kRackRails{"RackRails_4frames", 4}; // 0 is Front_Left; 1 is Front_Right; 2 is Back_Left; 3 is Back_Right

static std::vector<Def> kGlobalBuiltIns{kLogoDark, kDeviceType, kFoldButton, kRackRails};

}

class FilmStrip
{
public:
  using key_t = std::string;

  struct Filter
  {
    using type = std::function<bool(FilmStrip const &iFilmStrip)>;

    Filter() = default;
    Filter(type iAction, std::string iDescription) : fAction{std::move(iAction)}, fDescription{std::move(iDescription)} {}
    explicit operator bool() const { return fAction.operator bool(); }
    bool operator()(FilmStrip const &f) const { return fAction(f); }
    type fAction{};
    std::string fDescription{};
  };

public:
  struct Source
  {
    using origin_t = std::variant<fs::path, BuiltIn>;

    constexpr bool hasPath() const { return std::holds_alternative<fs::path>(fOrigin); }
    constexpr fs::path const &getPath() const { return std::get<fs::path>(fOrigin); }

    constexpr bool hasBuiltIn() const { return std::holds_alternative<BuiltIn>(fOrigin); }
    constexpr BuiltIn const &getBuiltIn() const { return std::get<BuiltIn>(fOrigin); }

    origin_t fOrigin{};
    key_t fKey{};
    long fLastModifiedTime{};
    int fNumFrames{1};

    static Source from(key_t const &iKey, fs::path const &iDirectory);
  };

public:

  inline key_t const &key() const { return fSource->fKey; };
  constexpr bool hasPath() const { return fSource->hasPath(); }
  inline fs::path const &path() const { return fSource->getPath(); };
  constexpr bool hasBuiltIn() const { return fSource->hasBuiltIn(); }

  inline key_t computeKey(texture::FX const &iEffects) const { return computeKey(key(), numFrames(), iEffects); }

  static key_t computeKey(key_t const &iKey, int iNumFrames, texture::FX const &iEffects);

  constexpr std::string const &errorMessage() const { return fErrorMessage; };

  constexpr bool isValid() const { return fImage.isValid(); }

  constexpr int width() const { return fImage.width(); }
  constexpr int height() const { return fImage.height(); }
  constexpr int numFrames() const { return fNumFrames > 0 ? fNumFrames : fSource->fNumFrames; }

  constexpr int frameWidth() const { return width(); }
  constexpr int frameHeight() const { return height() / numFrames(); }

  constexpr RLImageRGBA8::data_t const *data() const { return fImage.data(); }

  constexpr Image const &rlImage() const { return fImage.rlImageRef(); }

  int overrideNumFrames(int iNumFrames);

  std::unique_ptr<FilmStrip> applyEffects(texture::FX const &iEffects) const;

  static std::unique_ptr<FilmStrip> load(std::shared_ptr<Source> const &iSource);

  static inline auto bySizeFilter(ImVec2 const &iMinSize, ImVec2 const &iMaxSize, std::string iDescription) {
    return Filter([iMinSize, iMaxSize](FilmStrip const &iFilmStrip) {
      auto w = iFilmStrip.frameWidth();
      auto h = iFilmStrip.frameHeight();
      return w >= static_cast<int>(iMinSize.x) && h >= static_cast<int>(iMinSize.y) &&
             w <= static_cast<int>(iMaxSize.x) && h <= static_cast<int>(iMaxSize.y);
    }, std::move(iDescription));
  }

  static inline auto bySizeFilter(ImVec2 const &iMinSize, float iDelta) {
    return bySizeFilter(iMinSize, ImVec2{iMinSize.x + iDelta, iMinSize.y + iDelta},
                        fmt::printf("Size must be roughly %dx%d", static_cast<int>(iMinSize.x), static_cast<int>(iMinSize.y)));
  }

  static inline auto bySizeFilter(ImVec2 const &iSize) {
    return Filter([iSize](FilmStrip const &iFilmStrip) {
      return iFilmStrip.frameWidth() == static_cast<int>(iSize.x) &&
             iFilmStrip.frameHeight() == static_cast<int>(iSize.y);
    }, fmt::printf("Size must be %dx%d", static_cast<int>(iSize.x), static_cast<int>(iSize.y)));
  }

  static inline auto bySingleFrameFilter() {
    return Filter([](FilmStrip const &iFilmStrip) {
      return iFilmStrip.numFrames() == 1;
    }, "Must have exactly one frame");
  }

  static Filter orFilter(Filter iFilter1, Filter iFilter2) {
    if(!iFilter1)
      return iFilter2;

    if(!iFilter2)
      return iFilter1;

    return {[f1 = std::move(iFilter1.fAction), f2 = std::move(iFilter2.fAction)](FilmStrip const &iFilmStrip) {
      return f1(iFilmStrip) || f2(iFilmStrip);
    }, fmt::printf("%s or %s", iFilter1.fDescription, iFilter2.fDescription)};
  }

  static Filter andFilter(Filter iFilter1, Filter iFilter2) {
    if(!iFilter1)
      return iFilter2;

    if(!iFilter2)
      return iFilter1;

    return {[f1 = std::move(iFilter1.fAction), f2 = std::move(iFilter2.fAction)](FilmStrip const &iFilmStrip) {
      return f1(iFilmStrip) && f2(iFilmStrip);
    }, fmt::printf("%s and %s", iFilter1.fDescription, iFilter2.fDescription)};
  }

  static inline Filter kAllFilter{[] (FilmStrip const &iFilmStrip) { return true; }, "Match all"};

//  std::unique_ptr<FilmStrip> clone() const;

  ~FilmStrip();

  friend class FilmStripMgr;

private:
  struct FrameRGBA8Iterator
  {
    FrameRGBA8Iterator(Image iImage, int iNumFrames, int iCurrentFrame = 0);
    Image operator*() const { return fCurrentImage; }
    Image const *operator->() const { return &fCurrentImage; }
    FrameRGBA8Iterator &operator++() { fCurrentFrame++; computeCurrentImage(); return *this; }
    bool operator==(FrameRGBA8Iterator const &rhs) const;
    inline bool operator!=(FrameRGBA8Iterator const &rhs) const { return !(rhs == *this); }


  private:
    void computeCurrentImage();

  private:
    Image fImage;
    int fNumFrames;
    int fCurrentFrame;
    Image fCurrentImage;
  };

  struct FrameRGBA8Range
  {
    FrameRGBA8Iterator fBegin;
    FrameRGBA8Iterator fEnd;

    inline static FrameRGBA8Range create(RLImageRGBA8 const &iImage, int iNumFrames)
    {
      return {
        FrameRGBA8Iterator{iImage.rlImageRef(), iNumFrames},            // begin
        FrameRGBA8Iterator{iImage.rlImageRef(), iNumFrames, iNumFrames} // end
      };
    }

    constexpr FrameRGBA8Iterator const &begin() const { return fBegin; }
    constexpr FrameRGBA8Iterator const &end() const { return fEnd; }
  };

private:
  FilmStrip(std::shared_ptr<Source> iSource, char const *iErrorMessage);
  FilmStrip(std::shared_ptr<Source> iSource, RLImageRGBA8 &&iImage);

  void updateSource(std::shared_ptr<Source> iSource) { fSource = std::move(iSource); }
  void markDeleted();

  static std::unique_ptr<FilmStrip> loadBuiltInCompressedBase85(std::shared_ptr<Source> const &iSource);

private:
  std::shared_ptr<Source> fSource;
  RLImageRGBA8 fImage;
  int fNumFrames{0};

  std::string fErrorMessage;
};

struct FilmStripFX
{
  FilmStrip::key_t fKey{};
  texture::FX fEffects{};
};



class FilmStripMgr
{
public:
  explicit FilmStripMgr(std::vector<BuiltIns::Def> const &iBuiltIns, std::optional<fs::path> iDirectory = std::nullopt);
  std::shared_ptr<FilmStrip> findFilmStrip(FilmStrip::key_t const &iKey) const;
  std::shared_ptr<FilmStrip> getFilmStrip(FilmStrip::key_t const &iKey) const;

  std::set<FilmStrip::key_t> scanDirectory();
  std::vector<FilmStrip::key_t> getKeys() const { return findKeys(FilmStrip::kAllFilter); }
  std::vector<FilmStrip::key_t> findKeys(FilmStrip::Filter const &iFilter) const;
  bool checkKeyMatchesFilter(FilmStrip::key_t const &iKey, FilmStrip::Filter const &iFilter) const;
  std::optional<FilmStrip::key_t> importTexture(fs::path const &iTexturePath);
  std::set<FilmStrip::key_t> importBuiltIns(std::set<FilmStrip::key_t> const &iKeys, UserError *oErrors = nullptr);
  bool remove(FilmStrip::key_t const &iKey);

  std::optional<FilmStrip::key_t> applyEffects(FilmStrip::key_t const &iKey, texture::FX const &iEffects, UserError *oErrors = nullptr);
  std::set<FilmStrip::key_t> applyEffects(std::vector<FilmStripFX> const &iEffects, UserError *oErrors = nullptr);

  static std::vector<FilmStrip::Source> scanDirectory(fs::path const &iDirectory);
  static bool isValidTexturePath(fs::path const &iPath);

private:
  static std::shared_ptr<FilmStrip::Source> toSource(FilmStrip::key_t const &iKey, BuiltIn const &iBuiltIn);
  std::unique_ptr<FilmStrip> save(FilmStrip::key_t const &iKey, std::unique_ptr<FilmStrip> iFilmStrip);

private:
  std::map<FilmStrip::key_t, BuiltIn> fBuiltIns{};
  std::optional<fs::path> fDirectory;
  mutable std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip>> fFilmStrips{};
  mutable std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip::Source>> fSources{};
};

}

#endif //RE_EDIT_FILMSTRIP_H