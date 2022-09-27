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

#include "OGL3TextureManager.h"
#include "../Errors.h"
#include "imgui_impl_opengl3_loader.h"

namespace re::edit {

//------------------------------------------------------------------------
// OGL3TextureManager::OGL3TextureManager
//------------------------------------------------------------------------
OGL3TextureManager::OGL3TextureManager(float iScreenScale) : TextureManager(iScreenScale)
{

}

//------------------------------------------------------------------------
// OGL3TextureManager::createTexture
//------------------------------------------------------------------------
std::unique_ptr<Texture> OGL3TextureManager::createTexture(std::shared_ptr<FilmStrip> const &iFilmStrip) const
{
  RE_EDIT_ASSERT(iFilmStrip->isValid());

  auto texture = std::make_unique<OGL3Texture>(iFilmStrip);

  auto const width = iFilmStrip->width();
  auto const height = iFilmStrip->height();

  auto pixels = iFilmStrip->data();

  GLuint image_texture;
  glGenTextures(1, &image_texture);
  glBindTexture(GL_TEXTURE_2D, image_texture);

  // Setup filtering parameters for display
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

  // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  auto textureID = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(image_texture));

  auto ogl3Data = std::make_unique<OGL3Texture::OGL3Data>(textureID, static_cast<float>(height));

  texture->addData(std::move(ogl3Data));

  return texture;
}

//------------------------------------------------------------------------
// OGL3Data::OGL3Data
//------------------------------------------------------------------------
OGL3Texture::OGL3Data::OGL3Data(ImTextureID iImTextureID, float iHeight) : Data(iImTextureID, iHeight)
{}

//------------------------------------------------------------------------
// OGL3Data::~OGL3Data
//------------------------------------------------------------------------
OGL3Texture::OGL3Data::~OGL3Data()
{
  // TODO HIGH YP: how to unload a texture????
}

}