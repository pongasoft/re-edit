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

#include "WindowsNativeApplication.h"
#include "WindowsNetworkManager.h"
#include "WindowsMultipleInstanceManager.h"
#include "LocalSettingsManager.h"

namespace re::edit::platform::windows {

//------------------------------------------------------------------------
// class WindowsContext
//------------------------------------------------------------------------
class WindowsContext : public re::edit::platform::RLContext
{
public:
  explicit WindowsContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager) :
    RLContext(std::move(iPreferencesManager))
  {
    // empty
  }

  float getScale() const override
  {
    return getFontDpiScale();
  }

  std::shared_ptr<re::edit::NetworkManager> newNetworkManager() const override
  {
    return std::make_shared<re::edit::WindowsNetworkManager>();
  }

protected:
  void initializeScaling(re::edit::Application *iApplication) const override
  {
    iApplication->onNativeWindowFontDpiScaleChange(getFontDpiScale());
  }

  bool isWindowIconAllowed() const override
  {
    return true;
  }


};

//------------------------------------------------------------------------
// WindowsNativeApplication::newRLContext
//------------------------------------------------------------------------
std::unique_ptr<RLContext> WindowsNativeApplication::newRLContext() const
{
  return std::make_unique<WindowsContext>(newPreferencesManager());
}

//------------------------------------------------------------------------
// WindowsNativeApplication::newPreferencesManager
//------------------------------------------------------------------------
std::unique_ptr<NativePreferencesManager> WindowsNativeApplication::newPreferencesManager() const
{
  return std::make_unique<LocalSettingsManager>();
}

//------------------------------------------------------------------------
// WindowsNativeApplication::isSingleInstance
//------------------------------------------------------------------------
bool WindowsNativeApplication::isSingleInstance() const
{
  return WindowsMultipleInstanceManager::isSingleInstance();
}

//------------------------------------------------------------------------
// WindowsNativeApplication::registerInstance
//------------------------------------------------------------------------
bool WindowsNativeApplication::registerInstance() const
{
  return WindowsMultipleInstanceManager::registerInstance();
}

}

namespace re::edit::platform {

//------------------------------------------------------------------------
// NativeApplication::create
//------------------------------------------------------------------------
std::unique_ptr<NativeApplication> NativeApplication::create()
{
  return std::make_unique<windows::WindowsNativeApplication>();
}

}