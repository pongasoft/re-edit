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

#ifndef RE_EDIT_OGL3_TEXTURE_MANAGER_H
#define RE_EDIT_OGL3_TEXTURE_MANAGER_H

#include "../../TextureManager.h"
#include "../../FontManager.h"

namespace re::edit {

class OGL3Texture : public Texture
{
public:
  class OGL3GPUData : public Texture::GPUData
  {
  public:
    OGL3GPUData(ImTextureID iImTextureID, float iHeight);
    ~OGL3GPUData() override;
//    OGL3::Texture *getOGL3Texture() const { return reinterpret_cast<OGL3::Texture *>(fImTextureID); }
  };

public:
  OGL3Texture() = default;
  ~OGL3Texture() override = default;

protected:
  void doLoadOnGPU(std::shared_ptr<FilmStrip> const &iFilmStrip) const override;
};

class OGL3TextureManager : public TextureManager
{
public:
   OGL3TextureManager() = default;
  ~OGL3TextureManager() override = default;

protected:
  std::unique_ptr<Texture> createTexture() const override;
};

class OGL3FontManager : public NativeFontManager
{
public:
  OGL3FontManager() = default;
  void createFontsTexture() override;
  void destroyFontsTexture() override;
};



}

#endif //RE_EDIT_OGL3_TEXTURE_MANAGER_H
