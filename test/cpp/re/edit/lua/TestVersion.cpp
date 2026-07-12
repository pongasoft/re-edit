/*
 * Copyright (c) 2026 pongasoft
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
#include <version.h>
#include <re/edit/NetworkManager.h>

namespace re::edit::Test {
TEST(Version, fromGitTag) {
  ASSERT_EQ(std::nullopt, re::edit::Version::fromGitTag("1.2.3.456"));
  ASSERT_EQ("4.6.0.20260711", re::edit::Version::fromGitTag("v4.6.0.20260711")->fFullVersion);
  ASSERT_EQ("4.6.0", re::edit::Version::fromGitTag("v4.6.0.20260711")->fReSDKVersion);
  ASSERT_EQ(4600, re::edit::Version::fromGitTag("v4.6.0.20260711")->fReSDKVersionAsNumber);
  ASSERT_EQ(20260711, re::edit::Version::fromGitTag("v4.6.0.20260711")->fReleaseDate);
  ASSERT_EQ(std::nullopt, re::edit::Version::fromGitTag("v4.x.0.20260711"));
}


TEST(NetworkManager, getLatestRelease)
{
  auto json = R"([
{
  "tag_name": "v5.0.0.20260601",
  "html_url": "html_url_0",
  "body": "body_0"
},
{
  "tag_name": "v4.6.0.20260202",
  "html_url": "html_url_1",
  "body": "body_1"
},
{
  "tag_name": "v4.6.0.20260303",
  "html_url": "html_url_2",
  "body": "body_2"
}
]
)";

  {
    auto release = re::edit::NetworkManager::getLatestRelease(*Version::fromGitTag("v4.6.0.20260101"), json);
    ASSERT_NE(std::nullopt, release);
    ASSERT_EQ("4.6.0.20260303", release->fVersion.fFullVersion);
    ASSERT_EQ("html_url_2", release->fURL);
    ASSERT_EQ("body_2", release->fReleaseNotes);
  }

  {
    auto release = re::edit::NetworkManager::getLatestRelease(*Version::fromGitTag("v5.0.0.20260101"), json);
    ASSERT_NE(std::nullopt, release);
    ASSERT_EQ("5.0.0.20260601", release->fVersion.fFullVersion);
    ASSERT_EQ("html_url_0", release->fURL);
    ASSERT_EQ("body_0", release->fReleaseNotes);
  }

  {
    auto release = re::edit::NetworkManager::getLatestRelease(*Version::fromGitTag("v4.6.0.20260601"), json);
    ASSERT_EQ(std::nullopt, release);
  }
}

}