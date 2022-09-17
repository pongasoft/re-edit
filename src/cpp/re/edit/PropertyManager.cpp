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
#include "ReGui.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include "Errors.h"

using namespace re::mock;

namespace re::edit {

//------------------------------------------------------------------------
// PropertyManager::init
//------------------------------------------------------------------------
int PropertyManager::init(std::string const &iDirectory)
{
  // TODO: this will NOT work if the original device has a different constructor
  //       since it is instantiated via realtime_controller.lua
  struct NoOpDevice
  {
    explicit NoOpDevice(int /* iSampleRate */) {}
    void renderBatch(const TJBox_PropertyDiff [], TJBox_UInt32) {}
  };

  auto config = DeviceConfig<NoOpDevice>(Info::from_file(re::mock::fmt::path(iDirectory, "info.lua")))
    .device_root_dir(iDirectory)
    .device_resources_dir(re::mock::fmt::path(iDirectory, "Resources"))
    .mdef_file(re::mock::fmt::path(iDirectory, "motherboard_def.lua"))
    .rtc_file(re::mock::fmt::path(iDirectory, "realtime_controller.lua"))
    .rt([](Realtime &rt) { rt = Realtime{}; }); // no object creation at all
//    .rt([](Realtime &rt) { rt = Realtime::byDefault<NoOpDevice>(); });

  fDevice = std::make_shared<rack::Extension>(fRack.newExtension(config.getConfig()));

  auto objectInfos = fDevice->getObjectInfos();

  std::map<TJBox_ObjectRef, Object> objectsByRef{};

  for(auto const &info: objectInfos)
  {
    fObjects[info.fObjectPath] = Object{info};
    objectsByRef[info.fObjectRef] = Object{info};
  }

  auto propertyInfos = fDevice->getPropertyInfos();

  for(auto const &info: propertyInfos)
  {
    auto const parent = objectsByRef.at(info.fPropertyRef.fObject);
    fProperties[info.fPropertyPath] = Property{info, parent};
    if(info.fValueType == kJBox_Sample && parent.fInfo.fType == mock::JboxObjectType::kUserSamples)
      fUserSamplesCount++;
  }

  // we run the first batch which initialize the device
  fRack.nextBatch();

  // we disable notifications because we are not running the device
  fDevice->disableRTCNotify();
  fDevice->disableRTCBindings();

  return fDevice->getDeviceInfo().fDeviceHeightRU;
}

//------------------------------------------------------------------------
// PropertyManager::findObjects
//------------------------------------------------------------------------
std::vector<Object const *> PropertyManager::findObjects(Object::Filter const &iFilter) const
{
  std::vector<Object const *> res{};
  if(iFilter)
  {
    for(auto const &[name, object]: fObjects)
    {
      if(iFilter(object))
        res.emplace_back(&object);
    }
  }
  return res;
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
// PropertyManager::findPropertyNames
//------------------------------------------------------------------------
std::vector<std::string> PropertyManager::findPropertyNames(Property::Filter const &iFilter) const
{
  std::vector<std::string> res{};
  if(iFilter)
  {
    for(auto const &[name, property]: fProperties)
    {
      if(iFilter(property))
        res.emplace_back(name);
    }
  }
  return res;
}

//------------------------------------------------------------------------
// PropertyManager::sortProperties
//------------------------------------------------------------------------
void PropertyManager::sortProperties(std::vector<std::string> &ioProperties,
                                     Property::Comparator const &iComparator) const
{
  std::vector<const Property *> properties{};
  properties.reserve(ioProperties.size());
  for(auto &path: ioProperties)
    properties.emplace_back(&fProperties.at(path));
  std::sort(properties.begin(), properties.end(), iComparator);
  ioProperties.clear();
  for(auto &p: properties)
    ioProperties.emplace_back(p->path());
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

//------------------------------------------------------------------------
// PropertyManager::getIntValue
//------------------------------------------------------------------------
int PropertyManager::getIntValue(std::string const &iPropertyPath) const
{
  return fDevice->getNum<int>(iPropertyPath);
}

//------------------------------------------------------------------------
// PropertyManager::setIntValue
//------------------------------------------------------------------------
void PropertyManager::setIntValue(std::string const &iPropertyPath, int iValue)
{
  fDevice->setNum<int>(iPropertyPath, iValue);
}

//------------------------------------------------------------------------
// PropertyManager::beforeRenderFrame
//------------------------------------------------------------------------
void PropertyManager::beforeRenderFrame()
{
  // nothing to do
}

//------------------------------------------------------------------------
// PropertyManager::afterRenderFrame
//------------------------------------------------------------------------
void PropertyManager::afterRenderFrame()
{
  // nothing to do
}

//------------------------------------------------------------------------
// PropertyManager::addToWatchlist
//------------------------------------------------------------------------
void PropertyManager::addToWatchlist(std::string const &iPropertyPath)
{
  fPropertyWatchlist.emplace(iPropertyPath);
}

//------------------------------------------------------------------------
// PropertyManager::removeFromWatchlist
//------------------------------------------------------------------------
void PropertyManager::removeFromWatchlist(std::string const &iPropertyPath)
{
  fPropertyWatchlist.erase(iPropertyPath);
}

//------------------------------------------------------------------------
// PropertyManager::getNotWatchList
//------------------------------------------------------------------------
std::set<std::string> PropertyManager::getNotWatchList() const
{
  std::set<std::string> res{};
  for(auto const&[name, _]: fProperties)
  {
    if(fPropertyWatchlist.find(name) == fPropertyWatchlist.end())
      res.emplace(name);
  }
  return res;
}

//------------------------------------------------------------------------
// ::toOwnerString
//------------------------------------------------------------------------
static char const *toOwnerString(PropertyOwner iOwner)
{
  switch(iOwner)
  {
    case PropertyOwner::kHostOwner:
      return "Host";
    case PropertyOwner::kRTOwner:
      return "RT";
    case PropertyOwner::kRTCOwner:
      return "RTC";
    case PropertyOwner::kDocOwner:
      return "Document";
    case PropertyOwner::kGUIOwner:
      return "GUI";
  }
}

//------------------------------------------------------------------------
// ::toTypeString
//------------------------------------------------------------------------
static char const *toTypeString(TJBox_ValueType iValueType)
{
  switch(iValueType)
  {
    case kJBox_Nil:
      return "Nil";
    case kJBox_Number:
      return "Number";
    case kJBox_String:
      return "String";
    case kJBox_Boolean:
      return "Boolean";
    case kJBox_Sample:
      return "Sample";
    case kJBox_BLOB:
      return "Blob";
    case kJBox_DSPBuffer:
      return "DSP Buffer";
    case kJBox_NativeObject:
      return "Native Object";
    case kJBox_Incompatible:
      return "Incompatible";
  }
}

//------------------------------------------------------------------------
// ::toPersistenceString
//------------------------------------------------------------------------
static char const *toPersistenceString(lua::EPersistence iPersistence)
{
  switch(iPersistence)
  {
    case lua::EPersistence::kPatch:
      return "Patch";
    case lua::EPersistence::kSong:
      return "Song";
    case lua::EPersistence::kNone:
      return "None";
  }
}


//------------------------------------------------------------------------
// PropertyManager::editView
//------------------------------------------------------------------------
std::string PropertyManager::getPropertyInfo(std::string const &iPropertyPath) const
{
  auto p = findProperty(iPropertyPath);
  RE_EDIT_INTERNAL_ASSERT(p != nullptr);
  if(p->isDiscrete())
    return re::mock::fmt::printf("path = %s\ntype = %s\nsteps = %d\nowner = %s\ntag = %d\npersistence = %s\nvalue = %s",
                                 iPropertyPath,
                                 toTypeString(p->type()),
                                 p->stepCount(),
                                 toOwnerString(p->owner()),
                                 p->tag(),
                                 toPersistenceString(p->persistence()),
                                 fDevice->toString(p->path()));
  else
    return re::mock::fmt::printf("path = %s\ntype = %s\nowner = %s\ntag = %d\npersistence = %s\nvalue = %s",
                                 iPropertyPath,
                                 toTypeString(p->type()),
                                 toOwnerString(p->owner()),
                                 p->tag(),
                                 toPersistenceString(p->persistence()),
                                 fDevice->toString(p->path()));
}

//------------------------------------------------------------------------
// PropertyManager::editView
//------------------------------------------------------------------------
void PropertyManager::editView(Property const *iProperty)
{
  if(iProperty == nullptr)
    return;

  switch(iProperty->type())
  {
    case kJBox_Number:
    {
      if(iProperty->isDiscrete())
      {
        int value = getIntValue(iProperty->path());
        if(ImGui::SliderInt("value", &value, 0, iProperty->stepCount() - 1))
          setIntValue(iProperty->path(), value);
      }
      else
      {
        auto floatValue = static_cast<float>(fDevice->getNum(iProperty->path()));
        if(ImGui::DragFloat("value", &floatValue))
          fDevice->setNum(iProperty->path(), floatValue);
      }
      break;
    }
    case kJBox_String:
    {
      auto value = iProperty->owner() != mock::PropertyOwner::kRTOwner ? fDevice->getString(iProperty->path()) : fDevice->getRTString(iProperty->path());
      if(ImGui::InputText("value", &value))
      {
        if(iProperty->owner() != mock::PropertyOwner::kRTOwner)
          fDevice->setString(iProperty->path(), iProperty->path());
        else
          fDevice->setRTString(iProperty->path(), iProperty->path());
      }
      break;
    }

    case kJBox_Boolean:
    {
      auto value = fDevice->getBool(iProperty->path());
      if(ReGui::ToggleButton("false", "true", &value))
        fDevice->setBool(iProperty->path(), value);
      break;
    }

    default:
      ImGui::Text("%s", fDevice->toString(iProperty->path()).c_str());
      break;
  }
}


}