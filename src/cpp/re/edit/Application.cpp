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

#include "Application.h"
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// Application::render
//------------------------------------------------------------------------
void Application::render()
{
  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if(show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  {
    static float zoom = 0.25f;
    static int frame = 0;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("zoom", &zoom, 0.25f, 1.5f);
    ImGui::SliderInt("Frame", &frame, 0, 62);

    ImGui::ColorEdit3("clear color", (float *) &clear_color); // Edit 3 floats representing a color

    if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImVec2 pos;
    if(ImGui::BeginChild("Panel", {0, 200}, true, ImGuiWindowFlags_HorizontalScrollbar))
    {
      pos = ImGui::GetCursorPos();
      Texture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Panel_Front.png", 0, pos, {0, 0}, zoom);
      Texture("/Volumes/Development/github/pongasoft/re-cva-7/GUI2D/Knob_17_matte_63frames.png", frame, pos, {3414, 440}, zoom);
    }
    ImGui::EndChild();

    ImGui::Text("cursor %f x %f", pos.x, pos.y);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    ImGui::End();
  }

  // 3. Show another simple window.
  if(show_another_window)
  {
    ImGui::Begin("Another Window",
                 &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if(ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }
}

//------------------------------------------------------------------------
// Application::Texture
//------------------------------------------------------------------------
bool Application::Texture(std::string const &iName, int iFrameNumber, ImVec2 const &iRoot, ImVec2 const &iPosition, float zoom)
{
  auto texture =
    fTextureManager->getTexture(iName, iFrameNumber);
  if(texture.fFilmStrip->isValid())
  {
    ImGui::SetCursorPos({iRoot.x + (iPosition.x * zoom), iRoot.y + (iPosition.y * zoom)});
    ImGui::Image(texture.fData, {
      static_cast<float>(texture.fFilmStrip->frameWidth()) * zoom,
      static_cast<float>(texture.fFilmStrip->frameHeight()) * zoom,
    });
    return true;
  }
  else
  {
    ImGui::Text("Failed to load texture %s", texture.fFilmStrip->errorMessage().c_str());
    return false;
  }
}

}