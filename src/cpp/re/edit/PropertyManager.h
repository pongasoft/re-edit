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
  // return the size of the device in RU
  int init(std::string const &iDirectory);

  std::vector<Object const *> findObjects(Object::Filter const &iFilter) const;
//  Object const *findObject(std::string const &iObjectPath) const;

  std::vector<Property const *> findProperties(Property::Filter const &iFilter) const;
  std::vector<std::string> findPropertyNames(Property::Filter const &iFilter) const;
  Property const *findProperty(std::string const &iPropertyPath) const;
  std::string getPropertyInfo(std::string const &iPropertyPath) const;

  int getIntValue(std::string const &iPropertyPath) const;
  void setIntValue(std::string const &iPropertyPath, int iValue);

  constexpr int getUserSamplesCount() const { return fUserSamplesCount; }

  void addToWatchlist(std::string const &iPropertyPath);
  void removeFromWatchlist(std::string const &iPropertyPath);
  void clearWatchList() { fPropertyWatchlist.clear(); }

  std::set<std::string> const &getWatchList() const { return fPropertyWatchlist; }

  std::set<std::string> getNotWatchList() const;

  void editView(Property const *iProperty);
  void editView(std::string const &iPropertyPath) { editView(findProperty(iPropertyPath)); }

  friend class Application;

protected:
  void beforeRenderFrame();
  void afterRenderFrame();

private:
  re::mock::Rack fRack{};
  std::shared_ptr<re::mock::rack::Extension> fDevice{};
  std::map<std::string, Property> fProperties{};
  std::set<std::string> fPropertyWatchlist{};
  std::map<std::string, Object> fObjects{};
  int fUserSamplesCount{};
};

}

#endif //RE_EDIT_PROPERTY_MANAGER_H