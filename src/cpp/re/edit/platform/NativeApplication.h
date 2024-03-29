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

#ifndef RE_EDIT_NATIVE_APPLICATION_H
#define RE_EDIT_NATIVE_APPLICATION_H

#include <memory>
#include "RLContext.h"

namespace re::edit::platform {

class NativeApplication
{
public:
  static std::unique_ptr<NativeApplication> create();

  virtual std::unique_ptr<RLContext> newRLContext() const = 0;
  virtual bool isSingleInstance() const = 0;
  virtual bool registerInstance() const = 0;

  virtual ~NativeApplication() = default;

protected:
  virtual std::unique_ptr<NativePreferencesManager> newPreferencesManager() const = 0;
};

}
#endif //RE_EDIT_NATIVE_APPLICATION_H
