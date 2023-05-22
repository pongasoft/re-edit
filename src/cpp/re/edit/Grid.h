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

#ifndef RE_EDIT_GRID_H
#define RE_EDIT_GRID_H

#include <imgui.h>
#include "Errors.h"

namespace re::edit {

struct Grid
{
  constexpr explicit Grid(float iWidth = 1.0f, float iHeight = 1.0f) : fSize{iWidth, iHeight} {}

  constexpr float width() const { return fSize.x; }
  constexpr float height() const { return fSize.y; }

  ImVec2 fSize;

  static constexpr Grid unity() { return Grid{}; };

  //------------------------------------------------------------------------
  // clamp
  //------------------------------------------------------------------------
  constexpr ImVec2 clamp(ImVec2 const &v) const
  {
    return { clampValue(v.x, fSize.x), clampValue(v.y, fSize.y) };
  }

private:
#if __clang__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#endif
  // clamp a single value
  static constexpr float clampValue(float v, float g)
  {
    RE_EDIT_INTERNAL_ASSERT(g > 0);

    if(v == 0)
      return 0;

    if(g == 1.0f)
      return v;

    if(v < 0)
      return -clampValue(-v, g);

    // This is clearly not the best implementation but can't figure out using
    // fmod (which would not be constexpr)
    float res = 0;
    while(v >= g)
    {
      res += g;
      v -= g;
    }
    return res;
  }
#if __clang__
#pragma clang diagnostic pop
#endif
};

}

#endif //RE_EDIT_GRID_H
