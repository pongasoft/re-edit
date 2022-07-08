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

#include <gtest/gtest.h>
#include <re/edit/lua/HDGui2D.h>
#include <re/mock/fmt.h>

namespace re::edit::lua::Test {

std::string getResourceFile(std::string const &iFilename)
{
  return re::mock::fmt::path(RE_EDIT_PROJECT_DIR, "test", "resources", "re", "edit", "lua", iFilename);
}

TEST(HDGui2D, All)
{
  auto hdg = HDGui2D::fromFile(getResourceFile("all-hdgui_2D.lua"));

  auto front = hdg->front();
  ASSERT_EQ(1, front->fWidgets.size());
  ASSERT_EQ("Panel_front_bg", front->fGraphicsNode);
  ASSERT_EQ(std::nullopt, front->fCableOrigin);

  ASSERT_EQ(hdg->getStackString(), "<empty>");
}

}