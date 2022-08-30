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

#ifndef RE_EDIT_COLOR_H
#define RE_EDIT_COLOR_H

namespace re::edit {

struct JboxColor3
{
  int fRed{};
  int fGreen{};
  int fBlue{};

  inline void reset() { fRed = 0; fGreen = 0; fBlue = 0; }

  friend bool operator==(JboxColor3 const &lhs, JboxColor3 const &rhs)
  {
    return lhs.fRed == rhs.fRed &&
           lhs.fGreen == rhs.fGreen &&
           lhs.fBlue == rhs.fBlue;
  }

  friend bool operator!=(JboxColor3 const &lhs, JboxColor3 const &rhs)
  {
    return !(rhs == lhs);
  }
};

}



#endif //RE_EDIT_COLOR_H