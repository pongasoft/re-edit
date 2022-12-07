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

#ifndef RE_EDIT_GLFW_CONTEXT_H
#define RE_EDIT_GLFW_CONTEXT_H

#include "../Application.h"
#include <GLFW/glfw3.h>

namespace re::edit::platform {

class GLFWContext : public Application::Context
{
public:
  GLFWContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager,
              GLFWwindow *iWindow) :
    Context(std::move(iPreferencesManager)),
    fWindow{iWindow}
    {
      // empty
    }

  ImVec4 getWindowPositionAndSize() const override;
  void setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const override;
  void setWindowTitle(std::string const &iTitle) const override;

  void centerWindow() const override;

  void setupCallbacks(Application *iApplication);

  virtual float getScale() const = 0;

  float getFontDpiScale() const { return getFontDpiScale(fWindow); }

  static int initGLFW();
  static float getFontDpiScale(GLFWwindow *iWindow);

protected:
  GLFWwindow *fWindow;
};

}



#endif //RE_EDIT_GLFW_CONTEXT_H