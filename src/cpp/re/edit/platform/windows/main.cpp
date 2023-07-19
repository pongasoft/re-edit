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

#include <windows.h>

#include "../main.cpp"

inline void writeToConsoleStdErr(std::string const &iMessage)
{
  WriteConsole(GetStdHandle(STD_ERROR_HANDLE), iMessage.c_str(), iMessage.size(), NULL, NULL);
}

int main(int argc, char **argv)
{
  try
  {
    return doMain(argc, argv);
  }
  catch(...)
  {
    writeToConsoleStdErr(re::edit::fmt::printf("Unrecoverable error detected... aborting: %s", re::edit::Application::what(std::current_exception())));
    return 1;
  }
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, INT nCmdShow)
{
  std::string mainExe("re-edit.exe");

  if(lpCmdLine && !std::string(lpCmdLine).empty())
  {
    std::string args(lpCmdLine);
    char *argv[2];
    argv[0] = mainExe.data();
    argv[1] = args.data();
    return main(2, argv);
  }
  else
  {
    char *argv[1];
    argv[0] = mainExe.data();
    return main(1, argv);
  }

}
