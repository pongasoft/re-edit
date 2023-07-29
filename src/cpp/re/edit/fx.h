/*
 * Copyright (c) 2023 pongasoft
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

#ifndef RE_EDIT_FX_H
#define RE_EDIT_FX_H

#include "ReGui.h"
#include <optional>

namespace re::edit::texture {

struct FX
{
  ImU32 fTint{kDefaultTintColor};
  float fBrightness{kDefaultBrightness};
  bool fFlipX{};
  bool fFlipY{};
  std::optional<ImVec2> fSizeOverride{};

  constexpr bool hasTint() const noexcept { return fTint != kDefaultTintColor; }
  constexpr bool hasBrightness() const noexcept { return fBrightness != kDefaultBrightness; }
  constexpr bool hasShaderFX() const noexcept { return hasTint() || hasBrightness(); }
  constexpr bool isFlippedX() const noexcept { return fFlipX; }
  constexpr bool isFlippedY() const noexcept { return fFlipY; }
  constexpr bool hasSizeOverride() const noexcept { return fSizeOverride.has_value(); }

  constexpr bool hasAny() const noexcept { return
      hasTint() ||
      hasBrightness() ||
      isFlippedX() ||
      isFlippedY() ||
      hasSizeOverride()
      ;
  }

  bool operator==(FX const &rhs) const
  {
    return fTint == rhs.fTint &&
           fBrightness == rhs.fBrightness &&
           fFlipX == rhs.fFlipX &&
           fFlipY == rhs.fFlipY &&
           fSizeOverride == rhs.fSizeOverride;
  }

  bool operator!=(FX const &rhs) const
  {
    return !(rhs == *this);
  }
};

constexpr FX kDefaultFX{};

}

#endif //RE_EDIT_FX_H
