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

#ifndef RE_EDIT_PROPERTY_MANAGER_H
#define RE_EDIT_PROPERTY_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <set>
#include <re/mock/Rack.h>

#include "Property.h"

namespace re::edit {

class PropertyManager
{
public:
//  void init(std::string const &iMotherboardDefLuaFilename);
  void init(std::string const &iDirectory);

  std::vector<Property const *> findProperties(Property::Filter const &iFilter) const;
  Property const *findProperty(std::string const &iPropertyPath) const;

protected:

private:
  re::mock::Rack fRack{};
  std::shared_ptr<re::mock::rack::Extension> fDevice{};
  std::map<std::string, Property> fProperties{};
};

}

#endif //RE_EDIT_PROPERTY_MANAGER_H