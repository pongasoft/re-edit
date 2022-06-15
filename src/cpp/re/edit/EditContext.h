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

#ifndef RE_EDIT_EDITCONTEXT_H
#define RE_EDIT_EDITCONTEXT_H

#include <vector>
#include <string>

namespace re::edit {

class EditContext
{
public:
  virtual std::vector<std::string> getPropertyNames() const = 0;
};

}
#endif //RE_EDIT_EDITCONTEXT_H
