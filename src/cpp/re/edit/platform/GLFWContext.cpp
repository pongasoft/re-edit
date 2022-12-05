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

#include "GLFWContext.h"

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
// GLFWContext::getWindowPositionAndSize
//------------------------------------------------------------------------
ImVec4 GLFWContext::getWindowPositionAndSize() const
{
  ImVec4 res{};
  int x,y;
  glfwGetWindowPos(fWindow, &x, &y);
  res.x = static_cast<float>(x);
  res.y = static_cast<float>(y);
  glfwGetWindowSize(fWindow, &x, &y);
  res.z = static_cast<float>(x);
  res.w = static_cast<float>(y);
  return res;
}

//------------------------------------------------------------------------
// GLFWContext::setWindowPositionAndSize
//------------------------------------------------------------------------
void GLFWContext::setWindowPositionAndSize(std::optional<ImVec2> const &iPosition, ImVec2 const &iSize) const
{
  glfwSetWindowSize(fWindow, static_cast<int>(iSize.x), static_cast<int>(iSize.y));
  if(iPosition)
    glfwSetWindowPos(fWindow, static_cast<int>(iPosition->x), static_cast<int>(iPosition->y));
  else
    centerWindow();
}

//------------------------------------------------------------------------
// GLFWContext::centerWindow
//------------------------------------------------------------------------
void GLFWContext::centerWindow() const
{
  int windowWidth, windowHeight;
  glfwGetWindowSize(fWindow, &windowWidth, &windowHeight);

  auto monitor = glfwGetPrimaryMonitor();

  int xpos, ypos, width, height;
  glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);

  auto availableWidth = width - xpos;
  auto availableHeight = height - ypos;

  auto windowPosX = (availableWidth - windowWidth) / 2 + xpos;
  auto windowPosY = (availableHeight - windowHeight) / 2 + ypos;

  glfwSetWindowPos(fWindow, windowPosX, windowPosY);
}


//------------------------------------------------------------------------
// GLFWContext::setWindowTitle
//------------------------------------------------------------------------
void GLFWContext::setWindowTitle(std::string const &iTitle) const
{
  glfwSetWindowTitle(fWindow, iTitle.c_str());
}

//------------------------------------------------------------------------
// GLFWContext::getFontDpiScale
//------------------------------------------------------------------------
float GLFWContext::getFontDpiScale()
{
  float dpiScale{1.0f};
  glfwGetWindowContentScale(fWindow, &dpiScale, nullptr);
  return dpiScale;
}


//------------------------------------------------------------------------
// glfw_error_callback
//------------------------------------------------------------------------
static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//------------------------------------------------------------------------
// GLFWContext::initGLFW
//------------------------------------------------------------------------
int GLFWContext::initGLFW()
{
  glfwSetErrorCallback(glfw_error_callback);
  return glfwInit();
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
// onWindowClose
//------------------------------------------------------------------------
static void onWindowClose(GLFWwindow* iWindow)
{
  auto application = reinterpret_cast<re::edit::Application *>(glfwGetWindowUserPointer(iWindow));
  application->maybeExit();
  if(application->running())
    glfwSetWindowShouldClose(iWindow, GLFW_FALSE);
}

//------------------------------------------------------------------------
// GLFWContext::setupCallbacks
//------------------------------------------------------------------------
void GLFWContext::setupCallbacks(Application *iApplication)
{
  glfwSetWindowUserPointer(fWindow, iApplication);
  glfwSetWindowContentScaleCallback(fWindow, onWindowContentScaleChange);
  glfwSetWindowCloseCallback(fWindow, onWindowClose);
}

}