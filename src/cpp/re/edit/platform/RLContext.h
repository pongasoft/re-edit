/*
 * Copyright (c) 2022-2023 pongasoft
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

#ifndef RE_EDIT_GLFW_CONTEXT_H
#define RE_EDIT_GLFW_CONTEXT_H

#include "../Application.h"

struct GLFWwindow;

namespace re::edit::platform {

class RLContext : public Application::Context
{
public:
  explicit RLContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager);

  ImVec4 getWindowPositionAndSize() const override;
  ImVec2 getRenderScale() const override;
  void setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const override;
  void setWindowTitle(std::string const &iTitle) const override;

  void setWindowIcon(Texture *iIconTexture) const;

  void openURL(std::string const &iURL) const override;

  void centerWindow() const override;

  void setup(Application *iApplication);

  virtual float getScale() const = 0;

  void setTargetFrameRate(int iFrameRate) const override;

  void setVSyncEnabled(bool iEnabled) const override;

  float getFontDpiScale() const { return getFontDpiScale(fWindow); }

  static float getFontDpiScale(GLFWwindow *iWindow);

protected:
  virtual bool isWindowIconAllowed() const = 0;
  virtual void initializeScaling(Application *iApplication) const = 0;
  void setupCallbacks(Application *iApplication);

protected:
  GLFWwindow *fWindow;
};

}



#endif //RE_EDIT_GLFW_CONTEXT_H