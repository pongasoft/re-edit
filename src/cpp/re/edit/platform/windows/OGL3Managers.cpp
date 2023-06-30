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

#include <utility>
#include "OGL3Managers.h"
#include "imgui_impl_opengl3_loader.h"
#include "imgui_impl_opengl3.h"

// hack to remove the definition of min as a "define"
#undef min

namespace re::edit {

//------------------------------------------------------------------------
// OGL3TextureManager::createTexture
//------------------------------------------------------------------------
std::unique_ptr<Texture> OGL3TextureManager::createTexture() const
{
  return std::make_unique<OGL3Texture>();
}

//------------------------------------------------------------------------
// OGL3GPUData::OGL3GPUData
//------------------------------------------------------------------------
OGL3Texture::OGL3GPUData::OGL3GPUData(ImTextureID iImTextureID, float iHeight) : GPUData(iImTextureID, iHeight)
{}

//------------------------------------------------------------------------
// OGL3GPUData::OGL3GPUData
//------------------------------------------------------------------------
OGL3Texture::OGL3GPUData::~OGL3GPUData()
{
  GLuint imageTexture = static_cast<GLuint>(reinterpret_cast<intptr_t>(fImTextureID));
  glDeleteTextures(1, &imageTexture);
}

// YP implementation note: including gl/GL.h for this constant results in many compilation errors
// => redefining here where it is needed
#define GL_MAX_TEXTURE_SIZE               0x0D33

//------------------------------------------------------------------------
// OGL3Texture::doLoadOnGPU
//------------------------------------------------------------------------
void OGL3Texture::doLoadOnGPU(std::shared_ptr<FilmStrip> const &iFilmStrip) const
{
  fGPUData.clear();

  if(iFilmStrip->isValid())
  {
    int glMaxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTextureSize);

    auto const width = iFilmStrip->width();
    RE_EDIT_ASSERT(width < glMaxTextureSize);

    auto height = iFilmStrip->height();

    auto pixels = iFilmStrip->data();

    do
    {
      auto h = std::min(height, glMaxTextureSize);

      GLuint imageTexture;
      glGenTextures(1, &imageTexture);
      glBindTexture(GL_TEXTURE_2D, imageTexture);

      // Setup filtering parameters for display
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

      // Upload pixels into texture
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      glBindTexture(GL_TEXTURE_2D, 0);

      auto textureID = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(imageTexture));

      auto ogl3Data = std::make_unique<OGL3Texture::OGL3GPUData>(textureID, static_cast<float>(h));

      fGPUData.emplace_back(std::move(ogl3Data));

      height -= h;
      pixels += 4 * width * h;
    }
    while(height != 0);
  }

}

//------------------------------------------------------------------------
// OGL3FontManager::createFontsTexture
//------------------------------------------------------------------------
void OGL3FontManager::createFontsTexture()
{
  ImGui_ImplOpenGL3_CreateFontsTexture();
}

//------------------------------------------------------------------------
// OGL3FontManager::destroyFontsTexture
//------------------------------------------------------------------------
void OGL3FontManager::destroyFontsTexture()
{
  ImGui_ImplOpenGL3_DestroyFontsTexture();
}

}