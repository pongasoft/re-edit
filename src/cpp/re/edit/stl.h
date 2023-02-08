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

#ifndef RE_EDIT_STL_H
#define RE_EDIT_STL_H

#include <stdexcept>

namespace re::edit::stl {

//------------------------------------------------------------------------
// stl::popLast
// throw exception on empty
//------------------------------------------------------------------------
template<typename C>
inline typename C::value_type popLast(C &v)
{
  if(v.empty())
    throw std::out_of_range("empty");

  auto iter = v.end() - 1;
  auto res = std::move(*iter);
  v.erase(iter);
  return res;
}

//------------------------------------------------------------------------
// stl::popLastOrDefault
// return C::value_type{} on empty
//------------------------------------------------------------------------
template<typename C>
inline typename C::value_type popLastOrDefault(C &v)
{
  if(v.empty())
    return {};

  auto iter = v.end() - 1;
  auto res = std::move(*iter);
  v.erase(iter);
  return res;
}


}

#endif //RE_EDIT_STL_H
