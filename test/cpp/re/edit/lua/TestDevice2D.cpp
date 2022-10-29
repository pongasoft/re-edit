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

inline std::string getResourceFile(std::string const &iFilename)
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

  auto front = d2d->front();

  ASSERT_EQ(5, front->fNodes.size());
  ASSERT_EQ(2, front->fDecalNodes.size());

  ImVec2 offset{};

  // bg
  {
    auto &n = front->fNodes.at("bg");
    ASSERT_EQ("bg", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("front_bg", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(1, n.fNumFrames);
  }

  offset += {200, 100};
  offset += {0, -50};

  // Label
  {
    auto &n = front->fNodes.at("Label");
    ASSERT_EQ("Label", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("Label_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(1, n.fNumFrames);
  }

  // Knob1
  {
    auto knob1Offset = offset + ImVec2{10 ,20};
    auto &n = front->fNodes.at("Knob1");
    ASSERT_EQ("Knob1", n.fName);
    ASSERT_TRUE(Eq(knob1Offset, n.fPosition));
    ASSERT_EQ("Knob1_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(64, n.fNumFrames);
  }

  // Knob2
  {
    auto knob2Offset = offset + ImVec2{30 , 40};
    auto &n = front->fNodes.at("Knob2");
    ASSERT_EQ("Knob2", n.fName);
    ASSERT_TRUE(Eq(knob2Offset, n.fPosition));
    ASSERT_EQ("Knob2_path", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(32, n.fNumFrames);
  }

  // Knob3
  {
    auto knob3Offset = offset + ImVec2{50 , 60};
    auto &n = front->fNodes.at("Knob3");
    ASSERT_EQ("Knob3", n.fName);
    ASSERT_TRUE(Eq(knob3Offset, n.fPosition));
    ASSERT_TRUE(Eq(ImVec2{5, 15}, std::get<ImVec2>(n.fKeyOrSize)));
    ASSERT_EQ(1, n.fNumFrames);
  }

  // Decal1_path
  {
    auto &n = front->fDecalNodes.at(0);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("Decal1_path", n.fKey);
    ASSERT_EQ("decal1", *n.fName);
  }

  // Decal2_path
  {
    auto anonymous2Offset = offset + ImVec2{100 , 110};

    auto &n = front->fDecalNodes.at(1);
    ASSERT_TRUE(Eq(anonymous2Offset, n.fPosition));
    ASSERT_EQ("Decal2_path", n.fKey);
    ASSERT_EQ("decal2", *n.fName);
  }

  auto foldedBack = d2d->folded_back();
  offset = {};

  ASSERT_EQ(3, foldedBack->fNodes.size());
  ASSERT_EQ(1, foldedBack->fDecalNodes.size());

  // Panel_folded_back_bg
  {
    auto &n = foldedBack->fNodes.at("Panel_folded_back_bg");
    ASSERT_EQ("Panel_folded_back_bg", n.fName);
    ASSERT_TRUE(Eq(offset, n.fPosition));
    ASSERT_EQ("Panel_folded_back", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(1, n.fNumFrames);
  }

  // DeviceName
  {
    auto deviceNameOffset = offset + ImVec2{1330 , 45};
    auto &n = foldedBack->fNodes.at("DeviceName");
    ASSERT_EQ("DeviceName", n.fName);
    ASSERT_TRUE(Eq(deviceNameOffset, n.fPosition));
    ASSERT_EQ("Tape_Horizontal_1frames", std::get<std::string>(n.fKeyOrSize));
    ASSERT_EQ(1, n.fNumFrames);
  }

  // CableOrigin
  {
    auto cableOriginOffset = offset + ImVec2{695 , 75};
    auto &n = foldedBack->fNodes.at("CableOrigin");
    ASSERT_EQ("CableOrigin", n.fName);
    ASSERT_TRUE(Eq(cableOriginOffset, n.fPosition));
    ASSERT_FALSE(n.hasSize());
    ASSERT_FALSE(n.hasKey());
    ASSERT_EQ(1, n.fNumFrames);
  }

  // Decal_path
  {
    auto &n = foldedBack->fDecalNodes.at(0);
    ASSERT_TRUE(Eq(ImVec2{5, 5}, n.fPosition));
    ASSERT_EQ("Decal_path", n.fKey);
    ASSERT_EQ(std::nullopt, n.fName);
  }


  ASSERT_EQ(d2d->getStackString(), "<empty>");
}

}