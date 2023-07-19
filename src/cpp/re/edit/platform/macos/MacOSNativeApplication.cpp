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

#include "MacOSNativeApplication.h"
#include "NSUserDefaultsManager.h"
#include "MacOSNetworkManager.h"
#include "MacOSMultipleInstanceManager.h"

namespace re::edit::platform::macos {

class MacOsContext : public re::edit::platform::RLContext
{
public:
  explicit MacOsContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager) :
    RLContext(std::move(iPreferencesManager))
  {
    // empty
  }

  std::shared_ptr<re::edit::NetworkManager> newNetworkManager() const override
  {
    return std::make_shared<re::edit::MacOSNetworkManager>();
  }

  float getScale() const override
  {
    return 1.0;
  }
};

//------------------------------------------------------------------------
// MacOSNativeApplication::newRLContext
//------------------------------------------------------------------------
std::unique_ptr<RLContext> MacOSNativeApplication::newRLContext() const
{
  return std::make_unique<MacOsContext>(newPreferencesManager());
}

//------------------------------------------------------------------------
// MacOSNativeApplication::newPreferencesManager
//------------------------------------------------------------------------
std::unique_ptr<NativePreferencesManager> MacOSNativeApplication::newPreferencesManager() const
{
  return std::make_unique<NSUserDefaultsManager>();
}

//------------------------------------------------------------------------
// MacOSNativeApplication::isSingleInstance
//------------------------------------------------------------------------
bool MacOSNativeApplication::isSingleInstance() const
{
  return MacOSMultipleInstanceManager::isSingleInstance();
}

//------------------------------------------------------------------------
// MacOSNativeApplication::registerInstance
//------------------------------------------------------------------------
bool MacOSNativeApplication::registerInstance() const
{
  // nothing to do on macOS
  return true;
}

}

namespace re::edit::platform {

//------------------------------------------------------------------------
// NativeApplication::create
//------------------------------------------------------------------------
std::unique_ptr<NativeApplication> NativeApplication::create()
{
  return std::make_unique<macos::MacOSNativeApplication>();
}

}