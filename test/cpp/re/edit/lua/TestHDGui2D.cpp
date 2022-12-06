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
#include <re/edit/Application.h>
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
  return fs::path(RE_EDIT_PROJECT_DIR) / "test" / "resources" / "re" / "edit" / "lua" / iFilename;
}

//------------------------------------------------------------------------
// escapeString
//------------------------------------------------------------------------
std::string escapeString(char const *s)
{
  return re::mock::fmt::printf("\"%s\"", s);
}

//------------------------------------------------------------------------
// toUIText
//------------------------------------------------------------------------
std::string toUIText(char const *s)
{
  return re::mock::fmt::printf("jbox.ui_text(\"%s\")", s);
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
// toUIText
//------------------------------------------------------------------------
std::string toUITexts(std::vector<char const *> const &iValue)
{
  if(iValue.empty())
    return "{}";
  std::vector<std::string> l{};
  std::transform(iValue.begin(), iValue.end(), std::back_inserter(l), toUIText);
  return re::mock::fmt::printf("{ %s }", re::mock::stl::join_to_string(l));
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
  return AttributeToString<PropertyPath>("value", R"(value={"%s",true})", iExpectedPath);
};

//! HasValueNoSwitch | value only
inline constexpr auto HasValueNoSwitch = [](char const *iExpectedPath) {
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

//! HasTextStyle | text_style
inline constexpr auto HasTextStyle = [](char const *iExpectedTextStyle) {
  return AttributeToString<StaticStringList>("text_style", R"(text_style={"%s",true})", iExpectedTextStyle);
};

//! HasColor
inline constexpr auto HasColor = [](char const *iName, JboxColor3 const &iColor) {
  return AttributeToString<Color3>(iName, R"(%s={{%d,%d,%d},true})", iName, iColor.fRed, iColor.fGreen, iColor.fBlue);
};

//! HasTextColor | text_color
inline constexpr auto HasTextColor = [](JboxColor3 const &iColor) {
  return HasColor("text_color", iColor);
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

//! HasHorizontalJustification | horizontal_justification
inline constexpr auto HasHorizontalJustification = [](char const *iExpected = nullptr) {
  if(iExpected)
    return AttributeToString<StaticStringList>("horizontal_justification", R"(horizontal_justification={"%s",true})", iExpected);
  else
    return AttributeToString<StaticStringList>("horizontal_justification", R"(horizontal_justification={"center",false})");
};

//! HasIncreasing | increasing
inline constexpr auto HasIncreasing = [](std::optional<bool> iExpected = std::nullopt) {
  if(iExpected)
    return AttributeToString<Bool>("increasing", R"(increasing={%s,true})", toString(*iExpected));
  else
    return AttributeToString<Bool>("increasing", R"(increasing={true,false})");
};


//! HasValues | values
inline constexpr auto HasValues = [](const std::vector<char const *>& iExpectedValues) {
  return AttributeToString<PropertyPathList>("values", R"(values={%s,true})", toString(iExpectedValues));
};

//! HasBool | bool value
inline constexpr auto HasBool = [](char const *iName, std::optional<bool> iExpectedValue = std::nullopt) {
  if(iExpectedValue)
    return AttributeToString<Bool>(iName, R"(%s={%s,true})", iName, *iExpectedValue ? "true" : "false");
  else
    return AttributeToString<Bool>(iName, R"(%s={false,false})", iName);
};

//! HasInteger | integer value
inline constexpr auto HasInteger = [](char const *iName, std::optional<int> iExpectedValue = std::nullopt) {
  if(iExpectedValue)
    return AttributeToString<Integer>(iName, R"(%s={%d,true})", iName, *iExpectedValue);
  else
    return AttributeToString<Integer>(iName, R"(%s={0,false})", iName);
};

//! HasIndex | index value
inline constexpr auto HasIndex = [](int iExpectedValue) {
  return AttributeToString<Index>("index", R"(index={%d,true})", iExpectedValue);
};

//! HasUserSampleIndex | user_sample_index value
inline constexpr auto HasUserSampleIndex = [](int iExpectedValue) {
  return AttributeToString<UserSampleIndex>("user_sample_index", R"(user_sample_index={%d,true})", iExpectedValue);
};

//! HasString | string value
inline constexpr auto HasString = [](char const *iName, char const * iExpectedValue = nullptr) {
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

//! HasValueTemplates | value_templates
inline constexpr auto HasValueTemplates = [](std::optional<std::vector<char const *>> const &iExpected = std::nullopt) {
  if(iExpected)
    return AttributeToString<ValueTemplates>("value_templates", R"(value_templates={%s,true})", toUITexts(*iExpected));
  else
    return AttributeToString<ValueTemplates>("value_templates", R"(value_templates={{},false})");
};

class MockTexture : public Texture
{

};

class MockTextureManager : public TextureManager
{
protected:
  std::unique_ptr<Texture> createTexture() const override
  {
    return std::make_unique<MockTexture>();
  }

  void populateTexture(std::shared_ptr<Texture> const &iTexture) const override
  {

  }
};

class MockContext : public Application::Context
{
public:
  MockContext() : Application::Context(true) {}

  std::shared_ptr<TextureManager> newTextureManager() const override
  {
    return std::make_shared<MockTextureManager>();
  }

  std::shared_ptr<NativeFontManager> newNativeFontManager() const override
  {
    return nullptr;
  }

  ImVec4 getWindowPositionAndSize() const override
  {
    return ImVec4();
  }

  void setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const override
  {

  }

  void centerWindow() const override
  {

  }

  void setWindowTitle(std::string const &iTitle) const override
  {

  }
};

TEST(HDGui2D, All)
{
  re::edit::Application app{std::make_shared<MockContext>()};
  app.loadProject(getResourceFile("."));

  auto hdg = HDGui2D::fromFile(getResourceFile("all-hdgui_2D.lua"));

  auto front = hdg->front();
  ASSERT_EQ(37, front->fWidgets.size());
  ASSERT_EQ("Panel_front_bg", front->fGraphicsNode);
  ASSERT_EQ(std::nullopt, front->fCableOrigin);
  ASSERT_EQ(std::vector<std::string>{"disable_sample_drop_on_panel"}, front->fOptions);

  int id = 0;

  // ak1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kAnalogKnob, w->fWidget->getType());
    ASSERT_EQ("ak1_node", w->fGraphics.fNode);
    ASSERT_EQ(std::nullopt, w->fGraphics.fHitBoundaries);
    ASSERT_THAT(w, HasValueNoSwitch("/ak1"));
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
    ASSERT_THAT(w, HasValueNoSwitch("/ak3"));
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
    ASSERT_THAT(w, HasInteger("display_width_pixels", 30));
    ASSERT_THAT(w, HasInteger("display_height_pixels", 10));
    ASSERT_THAT(w, HasValues({"/cd1"}));
    ASSERT_THAT(w, HasString("invalidate_function"));
    ASSERT_THAT(w, HasString("draw_function", "draw_cd1"));
    ASSERT_THAT(w, HasString("gesture_function"));
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
    ASSERT_THAT(w, HasInteger("display_width_pixels", 35));
    ASSERT_THAT(w, HasInteger("display_height_pixels", 40));
    ASSERT_THAT(w, HasString("invalidate_function", "invalidate_cd2"));
    ASSERT_THAT(w, HasString("draw_function", "draw_cd2"));
    ASSERT_THAT(w, HasString("gesture_function", "gesture_cd2"));
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
    ASSERT_THAT(w, HasValueNoSwitch("/sf1"));
    ASSERT_THAT(w, HasOrientation());
    ASSERT_THAT(w, HasBool("inverted"));
    ASSERT_THAT(w, HasInteger("inset1"));
    ASSERT_THAT(w, HasInteger("inset2"));
    ASSERT_THAT(w, HasInteger("handle_size"));
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
    ASSERT_THAT(w, HasBool("inverted", true));
    ASSERT_THAT(w, HasInteger("inset1", 10));
    ASSERT_THAT(w, HasInteger("inset2", 20));
    ASSERT_THAT(w, HasInteger("handle_size", 30));
    ASSERT_THAT(w, HasVisibility("/sf2_switch", {3, 1}));
    ASSERT_THAT(w, HasTooltipPosition("top"));
    ASSERT_THAT(w, HasTooltipTemplate("sf2_tooltip_template"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
  }

  // mb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kMomentaryButton, w->fWidget->getType());
    ASSERT_EQ("mb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/mb1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // mb2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kMomentaryButton, w->fWidget->getType());
    ASSERT_EQ("mb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/mb2"));
    ASSERT_THAT(w, HasVisibility("/mb2_switch", {7, 2}));
    ASSERT_THAT(w, HasTooltipPosition("center"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("mb2_tooltip_template"));
  }

  // tb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kToggleButton, w->fWidget->getType());
    ASSERT_EQ("tb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/tb1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // tb2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kToggleButton, w->fWidget->getType());
    ASSERT_EQ("tb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/tb2"));
    ASSERT_THAT(w, HasVisibility("/tb2_switch", {5, 0}));
    ASSERT_THAT(w, HasTooltipPosition("top_right"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("tb2_tooltip_template"));
  }

  // sb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kStepButton, w->fWidget->getType());
    ASSERT_EQ("sb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/sb1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasIncreasing());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // sb2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kStepButton, w->fWidget->getType());
    ASSERT_EQ("sb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/sb2"));
    ASSERT_THAT(w, HasVisibility("/sb2_switch", {6, 5}));
    ASSERT_THAT(w, HasTooltipPosition("no_tooltip"));
    ASSERT_THAT(w, HasIncreasing(false));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("sb2_tooltip_template"));
  }

  // udb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kUpDownButton, w->fWidget->getType());
    ASSERT_EQ("udb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/udb1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasBool("inverted"));
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // udb2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kUpDownButton, w->fWidget->getType());
    ASSERT_EQ("udb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/udb2"));
    ASSERT_THAT(w, HasVisibility("/udb2_switch", {0, 9}));
    ASSERT_THAT(w, HasTooltipPosition("top"));
    ASSERT_THAT(w, HasBool("inverted", true));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("udb2_tooltip_template"));
  }

  // sm1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSequenceMeter, w->fWidget->getType());
    ASSERT_EQ("sm1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/sm1"));
    ASSERT_THAT(w, HasNoVisibility());
  }

  // sm2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSequenceMeter, w->fWidget->getType());
    ASSERT_EQ("sm2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/sm2"));
    ASSERT_THAT(w, HasVisibility("/sm2_switch", {8}));
  }

  // pbg1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPatchBrowseGroup, w->fWidget->getType());
    ASSERT_EQ("pbg1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasBool("fx_patch"));
    ASSERT_THAT(w, HasTooltipPosition());
  }

  // pbg2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPatchBrowseGroup, w->fWidget->getType());
    ASSERT_EQ("pbg2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasBool("fx_patch", true));
    ASSERT_THAT(w, HasTooltipPosition("top"));
  }

  // pw1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPitchWheel, w->fWidget->getType());
    ASSERT_EQ("pw1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/pw1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // pw2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPitchWheel, w->fWidget->getType());
    ASSERT_EQ("pw2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/pw2"));
    ASSERT_THAT(w, HasVisibility("/pw2_switch", {5, 0}));
    ASSERT_THAT(w, HasTooltipPosition("top_right"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("pw2_tooltip_template"));
  }

  // ppb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPopupButton, w->fWidget->getType());
    ASSERT_EQ("ppb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/ppb1"));
    ASSERT_THAT(w, HasTextStyle("Label font"));
    ASSERT_THAT(w, HasTextColor({10, 20, 30}));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
  }

  // ppb2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPopupButton, w->fWidget->getType());
    ASSERT_EQ("ppb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/ppb2"));
    ASSERT_THAT(w, HasTextStyle("Arial medium large bold font"));
    ASSERT_THAT(w, HasTextColor({100, 200, 40}));
    ASSERT_THAT(w, HasVisibility("/ppb2_switch", {7}));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
  }

  // vd1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kValueDisplay, w->fWidget->getType());
    ASSERT_EQ("vd1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValueNoSwitch("/vd1"));
    ASSERT_THAT(w, HasValueTemplates());
    ASSERT_THAT(w, HasTextStyle("Small LCD font"));
    ASSERT_THAT(w, HasTextColor({11, 21, 31}));
    ASSERT_THAT(w, HasHorizontalJustification());
    ASSERT_THAT(w, HasBool("read_only"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // vd2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kValueDisplay, w->fWidget->getType());
    ASSERT_EQ("vd2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValueSwitch("/vd2_switch", {"/vd2_v1", "/vd2_v2"}));
    auto value_templates = std::vector<char const *>{"vd2_vt_1", "vd2_vt_2"};
    ASSERT_THAT(w, HasValueTemplates(value_templates));
    ASSERT_THAT(w, HasTextStyle("Arial medium font"));
    ASSERT_THAT(w, HasTextColor({101, 201, 41}));
    ASSERT_THAT(w, HasHorizontalJustification("right"));
    ASSERT_THAT(w, HasBool("read_only", true));
    ASSERT_THAT(w, HasVisibility("/vd2_visibility_switch", {9}));
    ASSERT_THAT(w, HasTooltipPosition("left"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("vd2_tooltip_template"));
  }

  // rb1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kRadioButton, w->fWidget->getType());
    ASSERT_EQ("rb1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/rb1"));
    ASSERT_THAT(w, HasIndex(3));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // pw2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kRadioButton, w->fWidget->getType());
    ASSERT_EQ("rb2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValue("/rb2"));
    ASSERT_THAT(w, HasIndex(5));
    ASSERT_THAT(w, HasVisibility("/rb2_switch", {6}));
    ASSERT_THAT(w, HasTooltipPosition("right"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("rb2_tooltip_template"));
  }

  // pn1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPatchName, w->fWidget->getType());
    ASSERT_EQ("pn1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasTextStyle("Small LCD font"));
    ASSERT_THAT(w, HasColor("fg_color", {100, 98, 45}));
    ASSERT_THAT(w, HasColor("loader_alt_color", {76, 23, 12}));
    ASSERT_THAT(w, HasBool("center"));
  }

  // pn2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kPatchName, w->fWidget->getType());
    ASSERT_EQ("pn2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasTextStyle("Huge bold LCD font"));
    ASSERT_THAT(w, HasColor("fg_color", {101, 97, 44}));
    ASSERT_THAT(w, HasColor("loader_alt_color", {70, 11, 7}));
    ASSERT_THAT(w, HasBool("center", true));
  }

  // zsk1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kZeroSnapKnob, w->fWidget->getType());
    ASSERT_EQ("zsk1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValueNoSwitch("/zsk1"));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
    ASSERT_THAT(w, HasShowRemoteBox());
    ASSERT_THAT(w, HasShowAutomationRect());
    ASSERT_THAT(w, HasTooltipTemplate());
  }

  // ak2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kZeroSnapKnob, w->fWidget->getType());
    ASSERT_EQ("zsk2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasValueSwitch("/zsk2_switch", {"/zsk2_v1"}));
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition("top"));
    ASSERT_THAT(w, HasShowRemoteBox(false));
    ASSERT_THAT(w, HasShowAutomationRect(false));
    ASSERT_THAT(w, HasTooltipTemplate("zsk2_tooltip_template"));
  }

  // sbg1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSampleBrowseGroup, w->fWidget->getType());
    ASSERT_EQ("sbg1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasNoVisibility());
    ASSERT_THAT(w, HasTooltipPosition());
  }

  // sbg2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSampleBrowseGroup, w->fWidget->getType());
    ASSERT_EQ("sbg2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasVisibility("/sbg2_switch", {12}));
    ASSERT_THAT(w, HasTooltipPosition("left"));
  }

  // sdz1
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSampleDropZone, w->fWidget->getType());
    ASSERT_EQ("sdz1_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasUserSampleIndex(3));
    ASSERT_THAT(w, HasNoVisibility());
  }

  // sbg2
  {
    auto w = front->fWidgets[id++];
    ASSERT_EQ(WidgetType::kSampleDropZone, w->fWidget->getType());
    ASSERT_EQ("sdz2_node", w->fGraphics.fNode);
    ASSERT_THAT(w, HasUserSampleIndex(2));
    ASSERT_THAT(w, HasVisibility("/sdz2_switch", {5, 15}));
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