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
#include "fs.h"

#include "Property.h"

namespace re::edit {

constexpr char const* deviceTypeToString(re::mock::DeviceType t) {
  switch(t)
  {
    case mock::DeviceType::kInstrument:
      return "instrument";
    case mock::DeviceType::kCreativeFX:
      return "creative_fx";
    case mock::DeviceType::kStudioFX:
      return "studio_fx";
    case mock::DeviceType::kHelper:
      return "helper";
    case mock::DeviceType::kNotePlayer:
      return "note_player";
    default:
      return "unknown";
  }
};

class PropertyManager
{
public:
//  void init(std::string const &iMotherboardDefLuaFilename);
  // return the size of the device in RU
  mock::Info init(fs::path const &iDirectory);
  re::mock::Info const &getDeviceInfo() const;

  std::vector<Object const *> findObjects(Object::Filter const &iFilter) const;
  Object const *findObject(std::string const &iObjectPath) const;

  std::vector<Property const *> findProperties(Property::Filter const &iFilter) const;
  std::vector<std::string> findPropertyNames(Property::Filter const &iFilter) const;
  void sortProperties(std::vector<std::string> &ioProperties, Property::Comparator const &iComparator) const;
  Property const *findProperty(std::string const &iPropertyPath) const;
  bool hasProperty(std::string const &iPropertyPath) const;
  std::string getPropertyInfo(std::string const &iPropertyPath) const;

  int getValueAsInt(std::string const &iPropertyPath) const;
  void setValueAsInt(std::string const &iPropertyPath, int iValue);

  constexpr int getUserSamplesCount() const { return fUserSamplesCount; }

  void addToWatchlist(std::string const &iPropertyPath);
  void removeFromWatchlist(std::string const &iPropertyPath);
  void clearWatchList() { fPropertyWatchlist.clear(); }

  std::set<std::string> const &getWatchList() const { return fPropertyWatchlist; }

  std::set<std::string> getNotWatchList() const;

  void editView(Property const *iProperty);
  void editView(std::string const &iPropertyPath) { editView(findProperty(iPropertyPath)); }

  friend class AppContext;

protected:
  template<typename Num>
  void setNumValue(std::string const &iPropertyPath, Num iValue);
  template<typename Num>
  Num setNumValueAction(std::string const &iPropertyPath, Num iValue);

  void setBoolValue(std::string const &iPropertyPath, bool iValue);
  bool setBoolValueAction(std::string const &iPropertyPath, bool iValue);

  std::string setStringValueAction(std::string const &iPropertyPath, std::string const &iValue);

  template<typename T, typename F>
  void updateProperty(F &&f, std::string const &iPropertyPath, T iValue);

  template<class T, class... Args >
  typename T::result_t executeAction(Args&&... args);

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