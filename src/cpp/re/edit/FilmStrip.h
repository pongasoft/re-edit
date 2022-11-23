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
#include <variant>
#include <imgui.h>
#include "fs.h"
#include "Constants.h"
#include "Errors.h"

namespace re::edit {

class FilmStrip;

struct BuiltIn
{
  int fNumFrames{1};
  char const *fCompressedDataBase85{};

  static constexpr char const *kAudioSocket = "SharedAudioJack_3frames";
  static constexpr char const *kCVSocket = "SharedCVJack_3frames";
  static constexpr char const *kPatchBrowseGroup = "PatchBrowseGroup";
  static constexpr char const *kPlaceholder = "Placeholder";
  static constexpr char const *kSampleBrowseGroup = "SampleBrowseGroup";
  static constexpr char const *kTapeHorizontal = "Tape_Horizontal_1frames";
  static constexpr char const *kTapeVertical = "Tape_Vertical_1frames";
  static constexpr char const *kTrimKnob = "TrimKnob";
  static constexpr char const *kRoutingIcon01 = "Routing_Icon_01";
  static constexpr char const *kRoutingIcon02 = "Routing_Icon_02";
  static constexpr char const *kRoutingIcon03 = "Routing_Icon_03";
  static constexpr char const *kRoutingIcon04 = "Routing_Icon_04";
  static constexpr char const *kRoutingIcon05 = "Routing_Icon_05";
  static constexpr char const *kRoutingIconWhite01 = "Routing_Icon_White_01";
  static constexpr char const *kRoutingIconWhite02 = "Routing_Icon_White_02";
  static constexpr char const *kRoutingIconWhite03 = "Routing_Icon_White_03";
  static constexpr char const *kRoutingIconWhite04 = "Routing_Icon_White_04";
  static constexpr char const *kRoutingIconWhite05 = "Routing_Icon_White_05";
};

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
  using data_t = stbi_uc;

  struct Data
  {
    explicit Data(data_t *iData) : fData{iData} {}
    ~Data();

    constexpr data_t const *data() const { return fData; }

  private:
    data_t *fData{};
  };

  inline key_t const &key() const { return fSource->fKey; };
  constexpr bool hasPath() const { return fSource->hasPath(); }
  inline fs::path const &path() const { return fSource->getPath(); };
  constexpr bool hasBuiltIn() const { return fSource->hasBuiltIn(); }

  constexpr std::string const &errorMessage() const { return fErrorMessage; };

  inline bool isValid() const { return fData != nullptr; }

  constexpr int width() const { return fWidth; }
  constexpr int height() const { return fHeight; }
  constexpr int numFrames() const { return fNumFrames > 0 ? fNumFrames : fSource->fNumFrames; }

  constexpr int frameWidth() const { return fWidth; }
  constexpr int frameHeight() const { return fHeight / numFrames(); }

  constexpr data_t const *data() const { return fData->data(); }

  void overrideNumFrames(int iNumFrames);

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

  ~FilmStrip();

  friend class FilmStripMgr;

private:
  FilmStrip(std::shared_ptr<Source> iSource, char const *iErrorMessage);
  FilmStrip(std::shared_ptr<Source> iSource, int iWidth, int iHeight, std::shared_ptr<Data> iData);

  static std::unique_ptr<FilmStrip> loadBuiltInCompressedBase85(std::shared_ptr<Source> const &iSource);

private:
  std::shared_ptr<Source> fSource;
  int fNumFrames{0};
  int fWidth{100};
  int fHeight{100};
  std::shared_ptr<Data> fData{};

  std::string fErrorMessage;
};

class FilmStripMgr
{
public:
  explicit FilmStripMgr(fs::path iDirectory);
  std::shared_ptr<FilmStrip> findFilmStrip(FilmStrip::key_t const &iKey) const;
  std::shared_ptr<FilmStrip> getFilmStrip(FilmStrip::key_t const &iKey) const;

  std::set<FilmStrip::key_t> scanDirectory();
  std::vector<FilmStrip::key_t> getKeys() const { return findKeys(FilmStrip::kAllFilter); }
  std::vector<FilmStrip::key_t> findKeys(FilmStrip::Filter const &iFilter) const;
  std::optional<FilmStrip::key_t> importTexture(fs::path const &iTexturePath);
  std::set<FilmStrip::key_t> importBuiltIns(std::set<FilmStrip::key_t> const &iKeys, UserError *oErrors = nullptr);

  static std::vector<FilmStrip::Source> scanDirectory(fs::path const &iDirectory);

private:
  static std::shared_ptr<FilmStrip::Source> toSource(FilmStrip::key_t const &iKey, BuiltIn const &iBuiltIn);

private:
  fs::path fDirectory;
  std::map<FilmStrip::key_t, BuiltIn> fBuiltIns{};
  mutable std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip>> fFilmStrips{};
  mutable std::map<FilmStrip::key_t, std::shared_ptr<FilmStrip::Source>> fSources{};
};

}

#endif //RE_EDIT_FILMSTRIP_H