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
// class PropertyManagerAction<T>
//------------------------------------------------------------------------
template<typename T>
class PropertyManagerAction : public ValueAction<PropertyManager, T>
{
public:
  PropertyManager *getTarget() const override
  {
    return fPropertyManager.get();
  }

  friend class PropertyManager;

protected:
  std::shared_ptr<PropertyManager> fPropertyManager;
};

//------------------------------------------------------------------------
// PropertyManager::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t PropertyManager::executeAction(Args &&... args)
{
  auto action = UndoManager::createAction<T>(std::forward<Args>(args)...);
  action->fPropertyManager = shared_from_this();
  return fUndoManager->execute<typename T::result_t, typename T::action_t>(std::move(action));
}

//------------------------------------------------------------------------
// PropertyManager::PropertyManager
//------------------------------------------------------------------------
PropertyManager::PropertyManager(std::shared_ptr<UndoManager> iUndoManager) : fUndoManager{std::move(iUndoManager) }
{
  // empty
}

//------------------------------------------------------------------------
// PropertyManager::init
//------------------------------------------------------------------------
Info PropertyManager::init(fs::path const &iDirectory)
{
  static const resource::String kRTC{R"(
format_version = "1.0"
rtc_bindings = {
  { source = "/environment/system_sample_rate", dest = "/global_rtc/init_instance" },
}
global_rtc = {
  init_instance = function(source_property_path, new_value)
  end,
}
rt_input_setup = { notify = { } }
)"};

  struct NoOpDevice
  {
    explicit NoOpDevice(int /* iSampleRate */) {}
    void renderBatch(const TJBox_PropertyDiff [], TJBox_UInt32) {}
  };

  auto config = DeviceConfig<NoOpDevice>(Info::from_file(iDirectory / "info.lua"))
    .device_root_dir(iDirectory)
    .device_resources_dir(iDirectory / "Resources")
    .rtc(kRTC)
//    .rtc_file(iDirectory / "realtime_controller.lua")
    .rt([](Realtime &rt) { rt = Realtime{}; }); // no object creation at all
//    .rt([](Realtime &rt) { rt = Realtime::byDefault<NoOpDevice>(); });

  auto motherboard_def = iDirectory / "motherboard_def.lua";

  if(fs::exists(motherboard_def))
    config.mdef_file(motherboard_def);
  else
    config.mdef(Config::skeletonMotherboardDef());

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
    if(info.fValueType == Property::Type::kSample && parent.fInfo.fType == Object::Type::kUserSamples)
      fUserSamplesCount++;
  }

  // we run the first batch which initialize the device
  fRack.nextBatch();

  // we disable notifications because we are not running the device
  fDevice->disableRTCNotify();
  fDevice->disableRTCBindings();

  return fDevice->getDeviceInfo();
}

//------------------------------------------------------------------------
// PropertyManager::getDeviceInfo
//------------------------------------------------------------------------
re::mock::Info const &PropertyManager::getDeviceInfo() const
{
  return fDevice->getDeviceInfo();
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
// PropertyManager::findAllObjects
//------------------------------------------------------------------------
std::vector<Object const *> PropertyManager::findAllObjects() const
{
  std::vector<Object const *> res{};
  res.reserve(fObjects.size());
  for(auto const &[name, object]: fObjects)
  {
    res.emplace_back(&object);
  }
  return res;
}

//------------------------------------------------------------------------
// PropertyManager::findObject
//------------------------------------------------------------------------
Object const *PropertyManager::findObject(std::string const &iObjectPath) const
{
  auto iter = fObjects.find(iObjectPath);
  if(iter != fObjects.end())
    return &iter->second;
  else
    return nullptr;
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
// PropertyManager::findAllProperties
//------------------------------------------------------------------------
std::vector<Property const *> PropertyManager::findAllProperties() const
{
  std::vector<Property const *> res{};
  res.reserve(fProperties.size());
  for(auto const &[name, property]: fProperties)
  {
    res.emplace_back(&property);
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
  {
    auto property = findProperty(path);
    if(property)
      properties.emplace_back(property);
    else
      RE_EDIT_LOG_WARNING("Invalid property %s", path);
  }
  std::sort(properties.begin(), properties.end(), iComparator);
  ioProperties.clear();
  for(auto &p: properties)
    ioProperties.emplace_back(p->path());
}

//------------------------------------------------------------------------
// PropertyManager::hasProperty
//------------------------------------------------------------------------
bool PropertyManager::hasProperty(std::string const &iPropertyPath) const
{
  return fProperties.find(iPropertyPath) != fProperties.end();
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
// PropertyManager::getValueAsInt
//------------------------------------------------------------------------
int PropertyManager::getValueAsInt(std::string const &iPropertyPath) const
{
  auto iter = fProperties.find(iPropertyPath);
  if(iter != fProperties.end())
  {
    switch(iter->second.type())
    {
      case Property::Type::kNumber:
        return fDevice->getNum<int>(iPropertyPath);
      case Property::Type::kBoolean:
        return fDevice->getBool(iPropertyPath) ? 1 : 0;
      default:
        return 0;
    }
  }
  else
    return 0;
}

//------------------------------------------------------------------------
// PropertyManager::setValueAsInt
//------------------------------------------------------------------------
void PropertyManager::setValueAsInt(std::string const &iPropertyPath, int iValue)
{
  auto iter = fProperties.find(iPropertyPath);
  if(iter != fProperties.end())
  {
    switch(iter->second.type())
    {
      case Property::Type::kNumber:
        setNumValue(iPropertyPath, iValue);
        break;
      case Property::Type::kBoolean:
        setBoolValue(iPropertyPath, iValue != 0);
        break;
      default:
        break;
    }
  }
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
    default:
      RE_EDIT_FAIL("not reached");
  }
}

//------------------------------------------------------------------------
// ::toTypeString
//------------------------------------------------------------------------
static char const *toTypeString(Property::Type iValueType)
{
  switch(iValueType)
  {
    case Property::Type::kNil:
      return "Nil";
    case Property::Type::kNumber:
      return "Number";
    case Property::Type::kString:
      return "String";
    case Property::Type::kBoolean:
      return "Boolean";
    case Property::Type::kSample:
      return "Sample";
    case Property::Type::kBlob:
      return "Blob";
    case Property::Type::kDSPBuffer:
      return "DSP Buffer";
    case Property::Type::kNativeObject:
      return "Native Object";
    case Property::Type::kIncompatible:
      return "Incompatible";
    default:
      RE_EDIT_FAIL("not reached");
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
    default:
      RE_EDIT_FAIL("not reached");
  }
}


//------------------------------------------------------------------------
// PropertyManager::getPropertyInfo
//------------------------------------------------------------------------
std::string PropertyManager::getPropertyInfo(std::string const &iPropertyPath) const
{
  auto p = findProperty(iPropertyPath);
  if(p)
  {
    if(p->isDiscrete())
      return fmt::printf("path  = %s\n"
                         "type  = %s\n"
                         "steps = %d\n"
                         "owner = %s\n"
                         "tag   = %d\n"
                         "pers. = %s\n"
                         "value = %s",
                         iPropertyPath,
                         toTypeString(p->type()),
                         p->stepCount(),
                         toOwnerString(p->owner()),
                         p->tag(),
                         toPersistenceString(p->persistence()),
                         fDevice->toString(p->path()));
    else
      return fmt::printf("path  = %s\n"
                         "type  = %s\n"
                         "owner = %s\n"
                         "tag   = %d\n"
                         "pers. = %s\n"
                         "value = %s",
                         iPropertyPath,
                         toTypeString(p->type()),
                         toOwnerString(p->owner()),
                         p->tag(),
                         toPersistenceString(p->persistence()),
                         fDevice->toString(p->path()));
  }
  else
  {
    return fmt::printf("path  = %s\n"
                       "error = Invalid property (missing from motherboard)\n", iPropertyPath);
  }
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
    case Property::Type::kNumber:
    {
      if(iProperty->isDiscrete())
      {
        auto value = fDevice->getNum<int>(iProperty->path());
        if(ImGui::SliderInt("value", &value, 0, iProperty->stepCount() - 1))
          setNumValue(iProperty->path(), value);
      }
      else
      {
        auto floatValue = fDevice->getNum<float>(iProperty->path());
        if(ImGui::DragFloat("value", &floatValue))
          setNumValue(iProperty->path(), floatValue);
      }
      break;
    }
    case Property::Type::kString:
    {
      auto value = iProperty->owner() != mock::PropertyOwner::kRTOwner ? fDevice->getString(iProperty->path()) : fDevice->getRTString(iProperty->path());
      if(ImGui::InputText("value", &value))
      {
        updateProperty(&PropertyManager::setStringValueAction, iProperty->path(), value);
      }
      break;
    }

    case Property::Type::kBoolean:
    {
      auto value = fDevice->getBool(iProperty->path());
      if(ReGui::ToggleButton("false", "true", &value))
        setBoolValue(iProperty->path(), value);
      break;
    }

    default:
      ImGui::Text("%s", fDevice->toString(iProperty->path()).c_str());
      break;
  }
}

//------------------------------------------------------------------------
// PropertyManager::editViewAsInt
//------------------------------------------------------------------------
void PropertyManager::editViewAsInt(Property const *iProperty, std::function<void(int)> const &iOnChange) const
{
  if(iProperty == nullptr)
    return;

  switch(iProperty->type())
  {
    case Property::Type::kNumber:
    {
      if(iProperty->isDiscrete())
      {
        auto value = fDevice->getNum<int>(iProperty->path());
        if(ImGui::SliderInt("value", &value, 0, iProperty->stepCount() - 1))
          iOnChange(value);
      }
      break;
    }

    case Property::Type::kBoolean:
    {
      auto value = fDevice->getBool(iProperty->path()) ? 1 : 0;
      if(ImGui::SliderInt("value", &value, 0, 1))
        iOnChange(value ? 1 : 0);
      break;
    }

    default:
      break;
  }

}

//------------------------------------------------------------------------
// PropertyManager::updateProperty
//------------------------------------------------------------------------
template<typename T, typename F>
void PropertyManager::updateProperty(F &&f, std::string const &iPropertyPath, T iValue)
{
  auto property = findProperty(iPropertyPath);
  if(property)
  {
    executeAction<PropertyManagerAction<T>>([path = iPropertyPath, f = std::forward<F>(f)](auto *mgr, auto v) {
                                              return std::invoke(f, mgr, path, v);
                                            },
                                            iValue,
                                            fmt::printf("Update property [%s]", iPropertyPath),
                                            MergeKey::from(const_cast<Property *>(property)));
  }
}

//------------------------------------------------------------------------
// PropertyManager::setBoolValueAction
//------------------------------------------------------------------------
bool PropertyManager::setBoolValueAction(std::string const &iPropertyPath, bool iValue)
{
  auto value = fDevice->getBool(iPropertyPath);
  fDevice->setBool(iPropertyPath, iValue);
  return value;
}

//------------------------------------------------------------------------
// PropertyManager::setBoolValue
//------------------------------------------------------------------------
void PropertyManager::setBoolValue(std::string const &iPropertyPath, bool iValue)
{
  updateProperty(&PropertyManager::setBoolValueAction, iPropertyPath, iValue);
}

//------------------------------------------------------------------------
// PropertyManager::setStringValueAction
//------------------------------------------------------------------------
std::string PropertyManager::setStringValueAction(std::string const &iPropertyPath, std::string const &iValue)
{
  std::string res{};

  auto property = findProperty(iPropertyPath);
  if(property)
  {
    if(property->owner() != mock::PropertyOwner::kRTOwner)
    {
      res = fDevice->getString(iPropertyPath);
      fDevice->setString(iPropertyPath, iValue);
    }
    else
    {
      res = fDevice->getRTString(iPropertyPath);
      fDevice->setRTString(iPropertyPath, iValue);
    }
  }
  return res;
}

//------------------------------------------------------------------------
// PropertyManager::setNumValueAction
//------------------------------------------------------------------------
template<typename Num>
Num PropertyManager::setNumValueAction(std::string const &iPropertyPath, Num iValue)
{
  auto value = fDevice->getNum<Num>(iPropertyPath);
  fDevice->setNum<Num>(iPropertyPath, iValue);
  return value;
}

//------------------------------------------------------------------------
// PropertyManager::setNumValue
//------------------------------------------------------------------------
template<typename Num>
void PropertyManager::setNumValue(std::string const &iPropertyPath, Num iValue)
{
  updateProperty<Num>(&PropertyManager::setNumValueAction<Num>, iPropertyPath, iValue);
}



}