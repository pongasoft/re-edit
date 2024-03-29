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

#include "UIContext.h"
#include <imgui.h>
#include <raylib.h>

namespace re::edit {

/**
 * `fragColor` is part of raylib and re-edit uses it only for X-Ray (alpha only)
 * `colTint`, `colBrightness` and `colContrast` are the effects parameters */
static constexpr char const *kFXFragmentShader = R"ogl33(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec4 colTint;
uniform float colBrightness;
uniform float colContrast;
void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 c = texelColor*colTint + vec4(colBrightness, colBrightness, colBrightness, 0);
    if(colContrast != 1.0)
    {
      c = c - vec4(0.5, 0.5, 0.5, 0);
      c = c * vec4(colContrast, colContrast, colContrast, 1.0);
      c = c + vec4(0.5, 0.5, 0.5, 0);
    }
    finalColor = c * fragColor;
}
)ogl33";

//------------------------------------------------------------------------
// UIContext::UIContext
//------------------------------------------------------------------------
UIContext::RLShader::RLShader() : fShader{}
{

}

//------------------------------------------------------------------------
// UIContext::UIContext
//------------------------------------------------------------------------
UIContext::UIContext(int iMaxTextureSize, std::thread::id iUIThreadId) :
  fMaxTextureSize{iMaxTextureSize},
  fUIThreadId{iUIThreadId}
{
}

//------------------------------------------------------------------------
// UIContext::init
//------------------------------------------------------------------------
void UIContext::init()
{
  fFXShader = std::move(RLShader{nullptr, kFXFragmentShader});
  fShaderTintLocation = GetShaderLocation(*fFXShader.getShader(), "colTint");
  fShaderBrightnessLocation = GetShaderLocation(*fFXShader.getShader(), "colBrightness");
  fShaderContrastLocation = GetShaderLocation(*fFXShader.getShader(), "colContrast");
}

//------------------------------------------------------------------------
// UIContext::execute
//------------------------------------------------------------------------
void UIContext::execute(ui_action_t iAction)
{
  if(std::this_thread::get_id() == fUIThreadId)
    iAction();
  else
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fUIActions.emplace_back(std::move(iAction));
  }
}


//------------------------------------------------------------------------
// UIContext::collectUIActions
//------------------------------------------------------------------------
std::vector<UIContext::ui_action_t> UIContext::collectUIActions()
{
  std::unique_lock<std::mutex> lock(fMutex);
  return std::move(fUIActions);
}

//------------------------------------------------------------------------
// UIContext::beginFXShader
//------------------------------------------------------------------------
void UIContext::beginFXShader(ImVec4 const &iTint, float iBrightness, float iContrast)
{
  auto shader = fFXShader.getShader();
  if(shader)
  {
    BeginShaderMode(*shader);
    SetShaderValue(*shader, fShaderTintLocation, &iTint.x, SHADER_UNIFORM_VEC4);
    SetShaderValue(*shader, fShaderBrightnessLocation, &iBrightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(*shader, fShaderContrastLocation, &iContrast, SHADER_UNIFORM_FLOAT);
  }
}

//------------------------------------------------------------------------
// UIContext::endFXShader
//------------------------------------------------------------------------
void UIContext::endFXShader()
{
  auto shader = fFXShader.getShader();
  if(shader)
  {
    EndShaderMode();
  }
}

//------------------------------------------------------------------------
// UIContext::RLShader::RLShader(&&)
//------------------------------------------------------------------------
UIContext::RLShader::RLShader(UIContext::RLShader &&iOther) noexcept: fShader{std::move(iOther.fShader)} {}

//------------------------------------------------------------------------
// UIContext::RLShader::RLShader(char const *)
//------------------------------------------------------------------------
UIContext::RLShader::RLShader(char const *iVertexShader, char const *iFragmentShader) :
  fShader{std::make_unique<::Shader>(LoadShaderFromMemory(iVertexShader, iFragmentShader))}
{
}

//------------------------------------------------------------------------
// UIContext::RLShader::~RLShader
//------------------------------------------------------------------------
UIContext::RLShader::~RLShader()
{
  if(fShader)
    UnloadShader(*fShader);
}

//------------------------------------------------------------------------
// UIContext::RLShader::operator=
//------------------------------------------------------------------------
UIContext::RLShader &UIContext::RLShader::operator=(UIContext::RLShader &&iOther) noexcept
{
  fShader = std::move(iOther.fShader);
  return *this;
}



}