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

#include <gtest/gtest.h>
#include <re/edit/FilmStrip.h>
#include <version.h>

namespace re::edit::Test {

TEST(FilmStrip, computeKey) {
  ASSERT_STREQ("file_63frames", FilmStrip::computeKey("file_60frames", 63, {}).c_str());
  ASSERT_STREQ("file_T648003_b7_C60_X_Y_S90x120_63frames", FilmStrip::computeKey("file", 63, {
    ReGui::GetColorImU32({100, 128, 3}),
    -7,
    60,
    true,
    true,
    ImVec2{90, 120}
  }).c_str());
  ASSERT_STREQ("file_S100x90_63frames", FilmStrip::computeKey("file63frames", 63, {
    kDefaultTintColor,
    kDefaultBrightness,
    kDefaultContrast,
    false,
    false,
    ImVec2{100, 90}
  }).c_str());
  ASSERT_STREQ("my_file_TABCDEF_1frames", FilmStrip::computeKey("my_file", 1, {
    ReGui::GetColorImU32({171, 205, 239}),
    kDefaultBrightness,
    kDefaultContrast,
    false,
    false,
    std::nullopt
  }).c_str());
}

TEST(Version, fromGitTag) {
  ASSERT_EQ(std::nullopt, re::edit::Version::fromGitTag("1.2.3.456"));
  ASSERT_EQ("4.6.0.20260711", re::edit::Version::fromGitTag("v4.6.0.20260711")->fFullVersion);
  ASSERT_EQ("4.6.0", re::edit::Version::fromGitTag("v4.6.0.20260711")->fReSDKVersion);
  ASSERT_EQ(460, re::edit::Version::fromGitTag("v4.6.0.20260711")->fReSDKVersionAsNumber);
  ASSERT_EQ(20260711, re::edit::Version::fromGitTag("v4.6.0.20260711")->fReleaseDate);
  ASSERT_EQ(std::nullopt, re::edit::Version::fromGitTag("v4.x.0.20260711"));
}

}