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

using namespace re::edit::widget::attribute;

std::string getResourceFile(std::string const &iFilename)
{
  return re::mock::fmt::path(RE_EDIT_PROJECT_DIR, "test", "resources", "re", "edit", "lua", iFilename);
}

#define ASSERT_ATT_EQ(expected, widget, type, name) \
  ASSERT_EQ((expected), (widget)->fWidget->findAttributeByNameAndType<type>(name)->toString())

#define ASSERT_ATT_VALUE(value, widget) \
  ASSERT_ATT_EQ("value={fUseSwitch=false,value={\"" value "\",true},value_switch={\"\",false},values={{},false}}", widget, Value, "value")

#define ASSERT_ATT_VALUE_SWITCH(value_switch, values, widget) \
  ASSERT_ATT_EQ("value={fUseSwitch=true,value={\"\",false},value_switch={\"" value_switch "\",true},values={" values ",true}}", widget, Value, "value")

#define ASSERT_ATT_NO_VISIBILITY(widget) \
  ASSERT_ATT_EQ(R"(visibility={visibility_switch={"",false},visibility_values={{},false}})", widget, Visibility, "visibility")

#define ASSERT_ATT_VISIBILITY(visibility_switch, visibility_values, widget) \
  ASSERT_ATT_EQ("visibility={visibility_switch={\"" visibility_switch "\",true},visibility_values={" visibility_values ",true}}", widget, Visibility, "visibility")

TEST(HDGui2D, All)
{
  auto hdg = HDGui2D::fromFile(getResourceFile("all-hdgui_2D.lua"));

  auto front = hdg->front();
  ASSERT_EQ(3, front->fWidgets.size());
  ASSERT_EQ("Panel_front_bg", front->fGraphicsNode);
  ASSERT_EQ(std::nullopt, front->fCableOrigin);

  // ak1
  {
    auto w = front->fWidgets[0];
    ASSERT_EQ("ak1_node", w->fGraphics.fNode);
    ASSERT_ATT_VALUE("/ak1", w);
    ASSERT_ATT_NO_VISIBILITY(w);
  }

  // ak2
  {
    auto w = front->fWidgets[1];
    ASSERT_EQ("ak2_node", w->fGraphics.fNode);
    ASSERT_ATT_VALUE_SWITCH("/ak2_switch", "{ \"/ak2_v1\", \"/ak2_v2\" }", w);
    ASSERT_ATT_NO_VISIBILITY(w);
  }

  // ak3
  {
    auto w = front->fWidgets[2];
    ASSERT_EQ("ak3_node", w->fGraphics.fNode);
    ASSERT_ATT_VALUE("/ak3", w);
    ASSERT_ATT_VISIBILITY("/ak3_switch", "{ 1, 0, 3 }", w);
  }


  ASSERT_EQ(hdg->getStackString(), "<empty>");
}

}