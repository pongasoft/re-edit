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

#include "NSUserDefaultsManager.h"
#import <Foundation/Foundation.h>

namespace re::edit {

// Implementation note: this file is written in Objective-C because there is no C api to access NSUserDefaults

constexpr static NSString *kPreferenceKey = @"preferences";

//------------------------------------------------------------------------
// NSUserDefaultsManager::load
//------------------------------------------------------------------------
std::optional<std::string> NSUserDefaultsManager::load() const
{
  auto userDefaults = [NSUserDefaults standardUserDefaults];
  auto obj = [userDefaults stringForKey:kPreferenceKey];
  if(obj == nullptr)
    return std::nullopt;
  else
    return std::string([obj UTF8String]);
}

//------------------------------------------------------------------------
// NSUserDefaultsManager::save
//------------------------------------------------------------------------
void NSUserDefaultsManager::save(std::string const &iPreferences) const
{
  auto userDefaults = [NSUserDefaults standardUserDefaults];
  [userDefaults setObject:[NSString stringWithUTF8String:iPreferences.c_str()] forKey:kPreferenceKey];
}


}