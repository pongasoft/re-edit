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
#include <re/edit/lua/Device2D.h>
#include <re/edit/AppContext.h>
#include <re/mock/fmt.h>

namespace re::edit::lua::Test {

inline fs::path getResourceFile(std::string const &iFilename)
{
  return fs::path(RE_EDIT_PROJECT_DIR) / "test" / "resources" / "re" / "edit" / "lua" / iFilename;
}

inline ::testing::AssertionResult Eq(ImVec2 const &lhs, ImVec2 const &rhs) {
  if (lhs.x == rhs.x && lhs.y == rhs.y) {
    return ::testing::AssertionSuccess();
  } else {
    return ::testing::AssertionFailure() << "{" << lhs.x << ", " << lhs.y << "} not equal to " << "{" << rhs.x << ", " << rhs.y << "}";
  }
}

TEST(Device2D, All)
{
  auto d2d = Device2D::fromFile(getResourceFile("all-device_2D.lua"));

  ASSERT_EQ("1.2.3", d2d->getReEditVersion());

  auto front = d2d->front();

  ASSERT_EQ(10, front->fNodes.size());

  std::set<std::string> widgetNames{};

  ImVec2 offset{};

  // bg
  {
    auto &n = front->fNodes.at("bg");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("bg", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("front_bg", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  offset += {200, 100};
  offset += {0, -50};

  // Label
  {
    auto &n = front->fNodes.at("Label");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("Label", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("Label_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // Knob1
  {
    auto knob1Offset = offset + ImVec2{10 ,20};
    auto &n = front->fNodes.at("Knob1");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("Knob1", n.fName);
    ASSERT_TRUE(Eq(knob1Offset, n.fPosition));
    ASSERT_EQ("Knob1_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(64, n.fNumFrames);
  }

  // Knob2
  {
    auto knob2Offset = offset + ImVec2{30 , 40};
    auto &n = front->fNodes.at("Knob2");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("Knob2", n.fName);
    ASSERT_TRUE(Eq(knob2Offset, n.fPosition));
    ASSERT_EQ("Knob2_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(32, n.fNumFrames);
  }

  // Knob3
  {
    auto knob3Offset = offset + ImVec2{50 , 60};
    auto &n = front->fNodes.at("Knob3");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("Knob3", n.fName);
    ASSERT_TRUE(Eq(knob3Offset, n.fPosition));
    ASSERT_TRUE(Eq(ImVec2{5, 15}, std::get<ImVec2>(n.fKeyOrSize)));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // Decal1_path
  {
    auto &n = front->fNodes.at("decal1");
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("decal1", n.fName);
    ASSERT_EQ("Decal1_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // Decal2_path
  {
    auto anonymous2Offset = offset + ImVec2{100 , 110};

    auto &n = front->fNodes.at("decal2");
    ASSERT_TRUE(Eq(anonymous2Offset, n.fPosition));
    ASSERT_EQ("decal2", n.fName);
    ASSERT_EQ("Decal2_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // new scope
  offset = {100, 310};

  // Knob4
  {
    auto knob4Offset = offset + ImVec2{10, 20};
    auto &n = front->fNodes.at("Knob4");
    widgetNames.emplace(n.fName);
    ASSERT_EQ("Knob4", n.fName);
    ASSERT_TRUE(Eq(knob4Offset, n.fPosition));
    ASSERT_EQ("Knob4_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(4, n.fNumFrames);
  }

  // label_for_Knob4
  {
    auto label_for_Knob4Offset = offset + ImVec2{10, 20} + ImVec2{-110, 105};

    auto &n = front->fNodes.at("label_for_Knob4");
    ASSERT_TRUE(Eq(label_for_Knob4Offset, n.fPosition));
    ASSERT_EQ("label_for_Knob4", n.fName);
    ASSERT_EQ("label_for_Knob4_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(2, *n.fNumFrames);
  }

  // fx
  {
    auto &n = front->fNodes.at("fx");
    widgetNames.emplace(n.fName);
    ASSERT_TRUE(Eq(ImVec2{300, 200}, n.fPosition));
    ASSERT_EQ("fx", n.fName);
    ASSERT_EQ("path_fx_original", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
    ASSERT_EQ(ReGui::GetColorImU32({100, 128, 145}), n.fEffects.fTint);
    ASSERT_EQ(-7, n.fEffects.fBrightness);
    ASSERT_TRUE(Eq(ImVec2{90, 120}, n.fEffects.fSizeOverride.value()));
    ASSERT_TRUE(n.fEffects.fFlipX);
    ASSERT_TRUE(n.fEffects.fFlipY);
  }

  std::vector<std::string> expectedDecalNames{{"decal1", "decal2", "label_for_Knob4"}};
  ASSERT_EQ(expectedDecalNames, front->getDecalNames(widgetNames));

  ////// foldedBack
  auto foldedBack = d2d->folded_back();
  offset = {};

  ASSERT_EQ(4, foldedBack->fNodes.size());

  // Panel_folded_back_bg
  {
    auto &n = foldedBack->fNodes.at("Panel_folded_back_bg");
    ASSERT_EQ("Panel_folded_back_bg", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("Panel_folded_back", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // DeviceName
  {
    auto deviceNameOffset = offset + ImVec2{1330 , 45};
    auto &n = foldedBack->fNodes.at("DeviceName");
    ASSERT_EQ("DeviceName", n.fName);
    ASSERT_TRUE(Eq(deviceNameOffset, n.fPosition));
    ASSERT_EQ("Tape_Horizontal_1frames", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // CableOrigin
  {
    auto cableOriginOffset = offset + ImVec2{695 , 75};
    auto &n = foldedBack->fNodes.at("CableOrigin");
    ASSERT_EQ("CableOrigin", n.fName);
    ASSERT_TRUE(Eq(cableOriginOffset, n.fPosition));
    ASSERT_FALSE(n.hasSize());
    ASSERT_FALSE(n.hasKey());
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }

  // Decal_path
  {
    auto &n = foldedBack->fNodes.at("panel_decal_1");
    ASSERT_TRUE(Eq(ImVec2{5, 5}, n.fPosition));
    ASSERT_EQ("panel_decal_1", n.fName);
    ASSERT_EQ("Decal_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(std::nullopt, n.fNumFrames);
  }


  ASSERT_EQ(d2d->getStackString(), "<empty>");
}

}