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

#ifndef RE_EDIT_EDIT_CONTEXT_H
#define RE_EDIT_EDIT_CONTEXT_H

#include <vector>
#include <string>
#include "Texture.h"
#include "Property.h"

namespace re::edit {

class EditContext
{
public:
  virtual std::vector<Property const *> findProperties(Property::Filter const &iFilter) const = 0;
  virtual Property const *findProperty(std::string const &iPropertyPath) const = 0;

  virtual std::vector<std::string> const &getTextureKeys() const = 0;
  virtual std::shared_ptr<Texture> getTexture(std::string const &iKey) const = 0;

  inline std::vector<Property const *> findProperties() const { return findProperties(Property::Filter{}); }
};

}
#endif //RE_EDIT_EDIT_CONTEXT_H
