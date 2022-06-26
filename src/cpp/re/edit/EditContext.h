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

#ifndef RE_EDIT_EDITCONTEXT_H
#define RE_EDIT_EDITCONTEXT_H

#include <vector>
#include <string>
#include "Texture.h"

namespace re::edit {

class EditContext
{
public:
  enum class PropertyKind
  {
    kAny,
    kDiscrete,
  };
public:
  std::vector<std::string> getPropertyNames(PropertyKind iPropertyKind = PropertyKind::kAny) const
  {
    return doGetPropertyNames(iPropertyKind);
  }
  virtual int getStepCount(std::string const &iPropertyPath) const = 0;

  virtual std::vector<std::string> const &getTextureKeys() const = 0;
  virtual std::shared_ptr<Texture> getTexture(std::string const &iKey) const = 0;

protected:
  virtual std::vector<std::string> doGetPropertyNames(PropertyKind iPropertyKind) const = 0;
};

}
#endif //RE_EDIT_EDITCONTEXT_H
