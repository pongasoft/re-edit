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

#include "WindowsMultipleInstanceManager.h"
#include <Windows.h>

namespace re::edit {

//------------------------------------------------------------------------
// WindowsMultipleInstanceManager::isSingleInstance
//------------------------------------------------------------------------
bool WindowsMultipleInstanceManager::isSingleInstance()
{
  auto window = FindWindowExW(nullptr, nullptr, L"com.pongasoft.re-edit", nullptr);
  return window == nullptr;
}

#define MAKEINTATOMW(i)  (LPCWSTR)((ULONG_PTR)((WORD)(i)))

//------------------------------------------------------------------------
// WindowsMultipleInstanceManager::registerInstance
//------------------------------------------------------------------------
bool WindowsMultipleInstanceManager::registerInstance()
{
  WNDCLASSEXW wc;

  // https://github.com/godotengine/godot/blob/9f12e7b52d944281a39b7d3a33de6700c76cc23a/platform/windows/display_server_windows.cpp#LL2479C31-L2479C38

  memset(&wc, 0, sizeof(WNDCLASSEXW));
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = CS_OWNDC | CS_DBLCLKS;
  wc.lpfnWndProc = DefWindowProcW;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = L"com.pongasoft.re-edit";

  auto windowAtom = RegisterClassExW(&wc);
  if (!windowAtom) {
    return false;
  }

  auto windowMessageHandle = CreateWindowExW(0, MAKEINTATOMW(windowAtom), L"Message", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
  if(!windowMessageHandle)
  {
    return false;
  }

  return true;
}

}