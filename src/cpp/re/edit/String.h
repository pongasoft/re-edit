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

#ifndef RE_EDIT_STRING_H
#define RE_EDIT_STRING_H

#include <string>

namespace re::edit {

class StringWithHash
{
public:
  using string_t = std::string;
  using hash_t = unsigned int;

public:
  StringWithHash() = default;
  explicit StringWithHash(char const *s);
  explicit StringWithHash(std::string s);

  inline string_t const &value() const { return fValue; }
  inline char const *c_str() const { return fValue.c_str(); }
  inline hash_t hash() const { return fHash; }

private:
  string_t fValue{};
  hash_t fHash{};
};


}



#endif //RE_EDIT_STRING_H