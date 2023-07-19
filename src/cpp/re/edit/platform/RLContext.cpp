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

#include "RLContext.h"
#include <GLFW/glfw3.h>
#include <raylib.h>

namespace re::edit::platform {

//static GLFWmonitor* findMonitor(GLFWwindow *iWindow)
//{
//
//  int monitorCount;
//  auto monitors = glfwGetMonitors(&monitorCount);
//  if(monitorCount == 0)
//    return glfwGetPrimaryMonitor();
//
//  if(monitorCount == 1)
//    return monitors[0];
//
//  int x, y;
//  glfwGetWindowPos(iWindow, &x, &y);
//
//    /*
//   * from collections import namedtuple
//Rectangle = namedtuple('Rectangle', 'xmin ymin xmax ymax')
//
//ra = Rectangle(3., 3., 5., 5.)
//rb = Rectangle(1., 1., 4., 3.5)
//# intersection here is (3, 3, 4, 3.5), or an area of 1*.5=.5
//
//def area(a, b):  # returns None if rectangles don't intersect
//    dx = min(a.xmax, b.xmax) - max(a.xmin, b.xmin)
//    dy = min(a.ymax, b.ymax) - max(a.ymin, b.ymin)
//    if (dx>=0) and (dy>=0):
//        return dx*dy
//   */
//
//}

//------------------------------------------------------------------------
// RLContext::RLContext
//------------------------------------------------------------------------
RLContext::RLContext(std::shared_ptr<re::edit::NativePreferencesManager> iPreferencesManager) :
  Context(std::move(iPreferencesManager)),
  fWindow{glfwGetCurrentContext()}
{
  // empty
}

//------------------------------------------------------------------------
// RLContext::getWindowPositionAndSize
//------------------------------------------------------------------------
ImVec4 RLContext::getWindowPositionAndSize() const
{
  auto scale = getScale();
  ImVec4 res{};
  auto pos = GetWindowPosition();
  res.x = static_cast<float>(pos.x) / scale;
  res.y = static_cast<float>(pos.y) / scale;
  res.z = static_cast<float>(GetScreenWidth()) / scale;
  res.w = static_cast<float>(GetScreenHeight()) / scale;
  return res;
}

//------------------------------------------------------------------------
// RLContext::setWindowPositionAndSize
//------------------------------------------------------------------------
void RLContext::setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const
{
  auto scale = getScale();

  SetWindowSize(static_cast<int>(iSize.x * scale), static_cast<int>(iSize.y * scale));
  if(iPosition)
    SetWindowPosition(static_cast<int>(iPosition->x * scale), static_cast<int>(iPosition->y * scale));
  else
    centerWindow();
}

//------------------------------------------------------------------------
// RLContext::centerWindow
//------------------------------------------------------------------------
void RLContext::centerWindow() const
{
  auto windowWidth = GetScreenWidth();
  auto windowHeight = GetScreenHeight();

  auto monitor = glfwGetPrimaryMonitor();

  int xpos, ypos, width, height;
  glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);

  auto availableWidth = width - xpos;
  auto availableHeight = height - ypos;

  auto windowPosX = (availableWidth - windowWidth) / 2 + xpos;
  auto windowPosY = (availableHeight - windowHeight) / 2 + ypos;

  SetWindowPosition(windowPosX, windowPosY);
}


//------------------------------------------------------------------------
// RLContext::setWindowTitle
//------------------------------------------------------------------------
void RLContext::setWindowTitle(std::string const &iTitle) const
{
  SetWindowTitle(iTitle.c_str());
}

//------------------------------------------------------------------------
// RLContext::getFontDpiScale
//------------------------------------------------------------------------
float RLContext::getFontDpiScale(GLFWwindow *iWindow)
{
  float dpiScale{1.0f};

  if(iWindow)
    glfwGetWindowContentScale(iWindow, &dpiScale, nullptr);
  else
  {
    auto monitor = glfwGetPrimaryMonitor();
    if(monitor)
    {
      glfwGetMonitorContentScale(monitor, &dpiScale, nullptr);
    }
  }
  return dpiScale;
}

//------------------------------------------------------------------------
// onWindowContentScaleChange
//------------------------------------------------------------------------
static void onWindowContentScaleChange(GLFWwindow* iWindow, float iXscale, float iYscale)
{
  auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
  application->onNativeWindowFontScaleChange(iXscale);
}

//------------------------------------------------------------------------
// onDropCallback
//------------------------------------------------------------------------
static void onDropCallback(GLFWwindow* iWindow, int iCount, const char** iPaths)
{
  if(iCount > 0)
  {
    std::vector<fs::path> paths{};
    paths.reserve(iCount);
    for(auto i = 0; i < iCount; i++)
      paths.emplace_back(std::filesystem::u8path(iPaths[i]));
    auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
    application->onNativeDropFiles(paths);
  }
}

//------------------------------------------------------------------------
// RLContext::setupCallbacks
//------------------------------------------------------------------------
void RLContext::setupCallbacks(Application *iApplication)
{
  glfwSetWindowUserPointer(fWindow, iApplication);
  glfwSetWindowContentScaleCallback(fWindow, onWindowContentScaleChange);
  // Implementation note: replacing raylib callback as there is no need to go through another layer
  glfwSetDropCallback(fWindow, onDropCallback);
}

//------------------------------------------------------------------------
// RLContext::openURL
//------------------------------------------------------------------------
void RLContext::openURL(std::string const &iURL) const
{
  OpenURL(iURL.c_str());
}


}