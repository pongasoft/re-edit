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
#include <gtest/gtest-matchers.h>
#include <re/edit/lua/HDGui2D.h>
#include <re/mock/fmt.h>
#include <gmock/gmock-matchers.h>
#include <ostream>

namespace re::edit::lua {

// Must be declared in same namespace to work
void PrintTo(std::shared_ptr<jbox_widget> const &iWidget, std::ostream *os)
{
  *os << iWidget->fWidget->getName();
}

}

namespace re::edit::lua::Test {

using namespace re::edit::widget::attribute;

std::string getResourceFile(std::string const &iFilename)
{
  return re::mock::fmt::path(RE_EDIT_PROJECT_DIR, "test", "resources", "re", "edit", "lua", iFilename);
}

//------------------------------------------------------------------------
// escapeString
//------------------------------------------------------------------------
std::string escapeString(std::string const &s)
{
  return re::mock::fmt::printf("\"%s\"", s);
}

//------------------------------------------------------------------------
// toString
//------------------------------------------------------------------------
std::string toString(std::vector<char const *> const &iValue)
{
  if(iValue.empty())
    return "{}";
  std::vector<std::string> l{};
  std::transform(iValue.begin(), iValue.end(), std::back_inserter(l), escapeString);
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// toString
//------------------------------------------------------------------------
std::string toString(std::vector<int> const &iValue)
{
  if(iValue.empty())
    return "{}";
  std::vector<std::string> l{};
  std::transform(iValue.begin(), iValue.end(), std::back_inserter(l), [](auto i) { return std::to_string(i); } );
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
}

//------------------------------------------------------------------------
// toString
//------------------------------------------------------------------------
std::string toString(bool iValue)
{
  return iValue ? "true" : "false";
}

//------------------------------------------------------------------------
// HasAttributeMatcher | Check if attribute exists
//------------------------------------------------------------------------
template<typename T>
class HasAttributeMatcher
{
public:
  using is_gtest_matcher = void;

  explicit HasAttributeMatcher(char const *iAttributeName) : fAttributeName{iAttributeName} {}

  bool MatchAndExplain(std::shared_ptr<jbox_widget> const &iWidget, std::ostream* os) const {
    fAttribute = iWidget->fWidget->template findAttributeByNameAndType<T>(fAttributeName);
    if(!fAttribute && os)
      DescribeNegationTo(os);
    return fAttribute != nullptr;
  }

  // Describes the property of a value matching this matcher.
  void DescribeTo(std::ostream* os) const { *os << "has an attribute " << fAttributeName << " with type " << typeid(T).name(); }

  // Describes the property of a value NOT matching this matcher.
  void DescribeNegationTo(std::ostream* os) const { *os << "does NOT have an attribute " << fAttributeName << " with type " << typeid(T).name(); }

  T const *attribute() const { return fAttribute; }

private:
  char const *fAttributeName;
  mutable T *fAttribute{};
};

//------------------------------------------------------------------------
// HasAttribute
//------------------------------------------------------------------------
template<typename T>
testing::Matcher<std::shared_ptr<jbox_widget> const &> HasAttribute(char const *iName) { return HasAttributeMatcher<T>(iName); }

//------------------------------------------------------------------------
// AttributeToStringMatcher | checks that the value of the attribute (expressed as toString()) matches
//------------------------------------------------------------------------
template<typename T>
class AttributeToStringMatcher
{
public:
  using is_gtest_matcher = void;

  AttributeToStringMatcher(char const *iAttributeName, std::string iExpectedValue) :
    fHasAttributeMatcher{iAttributeName},
    fExpectedValue{std::move(iExpectedValue)}
  {}

  bool MatchAndExplain(std::shared_ptr<jbox_widget> const &iWidget, std::ostream* os) const {
    if(!fHasAttributeMatcher.MatchAndExplain(iWidget, os))
      return false;
    auto att = fHasAttributeMatcher.attribute();
    if(att->toString() == fExpectedValue)
      return true;
    DescribeNegationTo(os);
    return false;
  }

  // Describes the property of a value matching this matcher.
  void DescribeTo(std::ostream* os) const { *os << fExpectedValue; }

  // Describes the property of a value NOT matching this matcher.
  void DescribeNegationTo(std::ostream* os) const {
    if(os)
      *os << fHasAttributeMatcher.attribute()->toString();
  }

private:
  HasAttributeMatcher<T> fHasAttributeMatcher;
  std::string fExpectedValue;
};

//------------------------------------------------------------------------
// AttributeToString | checks that the value of the attribute (expressed as toString()) matches
//------------------------------------------------------------------------
template<typename T, typename ... Args>
testing::Matcher<std::shared_ptr<jbox_widget> const &> AttributeToString(char const *iAttributeName, const std::string &format, Args ... args)
{
  return AttributeToStringMatcher<T>(iAttributeName, re::mock::fmt::printf(format, std::forward<Args>(args)...));
}

//! HasValue | value only
inline constexpr auto HasValue = [](char const *iExpectedPath) {
  return AttributeToString<Value>("value", R"(value={fUseSwitch=false,value={"%s",true},value_switch={"",false},values={{},false}})", iExpectedPath);
};

//! HasValueSwitch | value with switch
inline constexpr auto HasValueSwitch = [](char const *iExpectedValueSwitch, const std::vector<char const *>& iExpectedValues) {
  return AttributeToString<Value>("value", R"(value={fUseSwitch=true,value={"",false},value_switch={"%s",true},values={%s,true}})", iExpectedValueSwitch, toString(iExpectedValues));
};

//! HasNoVisibility | no visibility
inline constexpr auto HasNoVisibility = []() {
  return AttributeToString<Visibility>("visibility", R"(visibility={visibility_switch={"",false},visibility_values={{},false}})");
};

//! HasVisibility | visibility
inline constexpr auto HasVisibility = [](char const *iExpectedVisibilitySwitch, const std::vector<int>& iExpectedValues) {
  return AttributeToString<Visibility>("visibility", R"(visibility={visibility_switch={"%s",true},visibility_values={%s,true}})", iExpectedVisibilitySwitch, toString(iExpectedValues));
};

//! HasTooltipPosition | tooltip_position
inline constexpr auto HasTooltipPosition = [](char const *iExpectedTooltipPosition = nullptr) {
  if(iExpectedTooltipPosition)
    return AttributeToString<StaticStringList>("tooltip_position", R"(tooltip_position={"%s",true})", iExpectedTooltipPosition);
  else
    return AttributeToString<StaticStringList>("tooltip_position", R"(tooltip_position={"",false})");
};

//! HasShowRemoteBox | show_remote_box
inline constexpr auto HasShowRemoteBox = [](std::optional<bool> iExpected = std::nullopt) {
  if(iExpected)
    return AttributeToString<Bool>("show_remote_box", R"(show_remote_box={%s,true})", toString(*iExpected));
  else
    return AttributeToString<Bool>("show_remote_box", R"(show_remote_box={true,false})");
};

//! HasShowAutomationRect | show_automation_rect
inline constexpr auto HasShowAutomationRect = [](std::optional<bool> iExpected = std::nullopt) {
  if(iExpected)
    return AttributeToString<Bool>("show_automation_rect", R"(show_automation_rect={%s,true})", toString(*iExpected));
  else
    return AttributeToString<Bool>("show_automation_rect", R"(show_automation_rect={true,false})");
};

//! HasTooltipTemplate | tooltip_template
inline constexpr auto HasTooltipTemplate = [](char const *iExpected = nullptr) {
  if(iExpected)
    return AttributeToString<UIText>("tooltip_template", R"(tooltip_template={jbox.ui_text("%s"),true})", iExpected);
  else
    return AttributeToString<UIText>("tooltip_template", R"(tooltip_template={jbox.ui_text(""),false})");
};

//! HasBlendMode | blend_mode
inline constexpr auto HasBlendMode = [](char const *iExpected = nullptr) {
  if(iExpected)
    return AttributeToString<StaticStringList>("blend_mode", R"(blend_mode={"%s",true})", iExpected);
  else
    return AttributeToString<StaticStringList>("blend_mode", R"(blend_mode={"normal",false})");
};

//! HasOrientation | orientation
inline constexpr auto HasOrientation = [](char const *iExpected = nullptr) {
  if(iExpected)
    return AttributeToString<StaticStringList>("orientation", R"(orientation={"%s",true})", iExpected);
  else
    return AttributeToString<StaticStringList>("orientation", R"(orientation={"vertical",false})");
};


//! HasValues | values
inline constexpr auto HasValues = [](const std::vector<char const *>& iExpectedValues) {
  return AttributeToString<PropertyPathList>("values", R"(values={%s,true})", toString(iExpectedValues));
};

//! HasBoolValue | bool value
inline constexpr auto HasBoolValue = [](char const *iName, std::optional<bool> iExpectedValue = std::nullopt) {
  if(iExpectedValue)
    return AttributeToString<Bool>(iName, R"(%s={%s,true})", iName, *iExpectedValue ? "true" : "false");
  else
    return AttributeToString<Bool>(iName, R"(%s={false,false})", iName);
};

//! HasIntegerValue | integer value
inline constexpr auto HasIntegerValue = [](char const *iName, std::optional<int> iExpectedValue = std::nullopt) {
  if(iExpectedValue)
    return AttributeToString<Integer>(iName, R"(%s={%d,true})", iName, *iExpectedValue);
  else
    return AttributeToString<Integer>(iName, R"(%s={0,false})", iName);
};

//! HasStringValue | string value
inline constexpr auto HasStringValue = [](char const *iName, char const * iExpectedValue = nullptr) {
  if(iExpectedValue)
    return AttributeToString<String>(iName, R"(%s={"%s",true})", iName, iExpectedValue);
  else
    return AttributeToString<String>(iName, R"(%s={"",false})", iName);
};

//! HasBackground | background (custom display)
inline constexpr auto HasBackground = [](char const *iExpected = nullptr) {
  if(iExpected)
    return AttributeToString<Background>("background", R"(background={jbox.image{ path = "%s" },true})", iExpected);
  else
    return AttributeToString<Background>("background", R"(background={jbox.image{ path = "" },false})");
};

//! HasSocket | socket
inline constexpr auto HasSocket = [](char const *iExpectedPath) {
  return AttributeToString<Socket>("socket", R"(socket={"%s",true})", iExpectedPath);
};


TEST(HDGui2D, All)
{
  auto hdg = HDGui2D::fromFile(getResourceFile("all-hdgui_2D.lua"));

  auto front = hdg->front();
  ASSERT_EQ(9, front->fWidgets.size());
  ASSERT_EQ("Panel_front_bg", front->fGraphicsNode);
  ASSERT_EQ(std::nullopt, front->fCableOrigin);

  int id = 0;

  // ak1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAnalogKnob, w->fWidget->getType());
    ASSERT_EQ("ak1_node", w->fGraphics.fNode);
    ASSERT_EQ(std::nullopt, w->fGraphics.fHitBoundaries);
    ASSERT_THAT(w, HasValue("/ak1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // ak2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAnalogKnob, w->fWidget->getType());
    ASSERT_EQ("ak2_node", w->fGraphics.fNode);
    ASSERT_EQ((HitBoundaries{ 3, 1, 4, 2}), w->fGraphics.fHitBoundaries.value());
    ASSERT_THAT(w, HasValueSwitch("/ak2_switch", {"/ak2_v1", "/ak2_v2"}));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition("top"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("ak2_tooltip_template"));
  }

  // ak3
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAnalogKnob, w->fWidget->getType());
    ASSERT_EQ("ak3_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/ak3"));
    ASSERT_THAT(w, HasVisibility("/ak3_switch", {1,0,3}));
  }

  // sd1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kStaticDecoration, w->fWidget->getType());
    ASSERT_EQ("sd1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasBlendMode());
    ASSERT_THAT(w, HasNoVisibility());
  }

  // sd2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kStaticDecoration, w->fWidget->getType());
    ASSERT_EQ("sd2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasBlendMode("luminance"));
    ASSERT_THAT(w, HasVisibility("/sd2_switch", {4, 9, 1}));
  }

  // scd1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kCustomDisplay, w->fWidget->getType());
    ASSERT_EQ("cd1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasIntegerValue("display_width_pixels", 30));
    ASSERT_THAT(w, HasIntegerValue("display_height_pixels", 10));
    ASSERT_THAT(w, HasValues({"/cd1"}));
    ASSERT_THAT(w, HasStringValue("invalidate_function"));
    ASSERT_THAT(w, HasStringValue("draw_function", "draw_cd1"));
    ASSERT_THAT(w, HasStringValue("gesture_function"));
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasBackground());
  }

  // scd2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kCustomDisplay, w->fWidget->getType());
    ASSERT_EQ("cd2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValues({"/cd2_1", "/cd2_2"}));
    ASSERT_THAT(w, HasIntegerValue("display_width_pixels", 35));
    ASSERT_THAT(w, HasIntegerValue("display_height_pixels", 40));
    ASSERT_THAT(w, HasStringValue("invalidate_function", "invalidate_cd2"));
    ASSERT_THAT(w, HasStringValue("draw_function", "draw_cd2"));
    ASSERT_THAT(w, HasStringValue("gesture_function", "gesture_cd2"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasVisibility("/cd2_switch", {8, 1}));
    ASSERT_THAT(w, HasBackground("cd2_bg"));
  }

  // sf1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSequenceFader, w->fWidget->getType());
    ASSERT_EQ("sf1_node", w->fGraphics.fNode);
    ASSERT_EQ(std::nullopt, w->fGraphics.fHitBoundaries);
    ASSERT_THAT(w, HasValue("/sf1"));
    ASSERT_THAT(w, HasOrientation());
    ASSERT_THAT(w, HasBoolValue("inverted"));
    ASSERT_THAT(w, HasIntegerValue("inset1"));
    ASSERT_THAT(w, HasIntegerValue("inset2"));
    ASSERT_THAT(w, HasIntegerValue("handle_size"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // sf2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSequenceFader, w->fWidget->getType());
    ASSERT_EQ("sf2_node", w->fGraphics.fNode);
    ASSERT_EQ(std::nullopt, w->fGraphics.fHitBoundaries);
    ASSERT_THAT(w, HasValueSwitch("/sf2_switch", {"/sf2_v1"}));
    ASSERT_THAT(w, HasOrientation("horizontal"));
    ASSERT_THAT(w, HasBoolValue("inverted", true));
    ASSERT_THAT(w, HasIntegerValue("inset1", 10));
    ASSERT_THAT(w, HasIntegerValue("inset2", 20));
    ASSERT_THAT(w, HasIntegerValue("handle_size", 30));
    ASSERT_THAT(w, HasVisibility("/sf2_switch", {3, 1}));
    ASSERT_THAT(w, HasTooltipPosition("top"));
    ASSERT_THAT(w, HasTooltipTemplate("sf2_tooltip_template"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
  }

  //------------------------------------------------------------------------
  // Back
  //------------------------------------------------------------------------
  ASSERT_EQ(hdg->getStackString(), "<empty>");

  auto back = hdg->back();
  ASSERT_EQ(6, back->fWidgets.size());
  ASSERT_EQ("Panel_back_bg", back->fGraphicsNode);
  ASSERT_EQ(std::nullopt, front->fCableOrigin);

  id = 0;

  // au_in_1
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAudioInputSocket, w->fWidget->getType());
    ASSERT_EQ("au_in_1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasSocket("/audio_inputs/au_in_1"));
  }

  // au_ou_1
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAudioOutputSocket, w->fWidget->getType());
    ASSERT_EQ("au_out_1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasSocket("/audio_outputs/au_out_1"));
  }

  // cv_in_1
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kCVInputSocket, w->fWidget->getType());
    ASSERT_EQ("cv_in_1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasSocket("/cv_inputs/cv_in_1"));
  }

  // cv_out_1
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kCVOutputSocket, w->fWidget->getType());
    ASSERT_EQ("cv_out_1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasSocket("/cv_outputs/cv_out_1"));
  }

  // cv_trim_1
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kCVTrimKnob, w->fWidget->getType());
    ASSERT_EQ("cv_trim_knob_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasSocket("/cv_inputs/cv_trim_1"));
  }

  // placeholder
  {
    auto w = back->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPlaceholder, w->fWidget->getType());
    ASSERT_EQ("ph1_node", w->fGraphics.fNode);
  }


}

}