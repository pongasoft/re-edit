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

#include "PropertyManager.h"

using namespace re::mock;

namespace re::edit {

//------------------------------------------------------------------------
// PropertyManager::init
//------------------------------------------------------------------------
void PropertyManager::init(std::string const &iDirectory)
{
  // TODO: this will NOT work if the original device has a different constructor
  //       since it is instantiated via realtime_controller.lua
  struct NoOpDevice
  {
    explicit NoOpDevice(int /* iSampleRate */) {}
    void renderBatch(const TJBox_PropertyDiff [], TJBox_UInt32) {}
  };

  auto config = DeviceConfig<NoOpDevice>(Info::from_file(fmt::path(iDirectory, "info.lua")))
    .device_root_dir(iDirectory)
    .device_resources_dir(fmt::path(iDirectory, "Resources"))
    .mdef_file(fmt::path(iDirectory, "motherboard_def.lua"))
    .rtc_file(fmt::path(iDirectory, "realtime_controller.lua"))
    .rt([](Realtime &rt) { rt = Realtime::byDefault<NoOpDevice>(); });

  fDevice = std::make_shared<rack::Extension>(fRack.newExtension(config.getConfig()));

  auto infos = fDevice->getPropertyInfos();

  for(auto const &info: infos)
    fProperties[info.fPropertyPath] = Property{info};
}

//------------------------------------------------------------------------
// PropertyManager::findProperties
//------------------------------------------------------------------------
std::vector<Property const *> PropertyManager::findProperties(Property::Filter const &iFilter) const
{
  std::vector<Property const *> res{};
  if(iFilter)
  {
    for(auto const &[name, property]: fProperties)
    {
      if(iFilter(property))
        res.emplace_back(&property);
    }
  }

  return res;
}

//------------------------------------------------------------------------
// PropertyManager::findProperty
//------------------------------------------------------------------------
Property const *PropertyManager::findProperty(std::string const &iPropertyPath) const
{
  auto iter = fProperties.find(iPropertyPath);
  if(iter != fProperties.end())
    return &iter->second;
  else
    return nullptr;
}

}