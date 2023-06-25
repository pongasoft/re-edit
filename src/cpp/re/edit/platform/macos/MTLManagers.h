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

#ifndef RE_EDIT_MTL_TEXTURE_MANAGER_H
#define RE_EDIT_MTL_TEXTURE_MANAGER_H

#include "../../TextureManager.h"
#include "../../FontManager.h"
#include <imgui.h>
#include <Metal/Metal.hpp>

namespace re::edit {

class MTLTexture : public Texture
{
public:
  class MTLData : public Texture::Data
  {
  public:
    MTLData(ImTextureID iImTextureID, float iHeight);
    ~MTLData() override;
    MTL::Texture *getMTLTexture() const { return reinterpret_cast<MTL::Texture *>(fImTextureID); }
  };

public:
  MTLTexture() = default;
  ~MTLTexture() override = default;

  inline static int kMaxTextureHeight = 16384;
};

class MTLTextureManager : public TextureManager
{
public:
  explicit MTLTextureManager(MTL::Device *iDevice);
  ~MTLTextureManager() override = default;

protected:
  std::unique_ptr<Texture> createTexture() const override;
  void populateTexture(std::shared_ptr<Texture> const &iTexture) const override;

private:
  MTL::Device *fDevice;
};

class MTLFontManager : public NativeFontManager
{
public:
  explicit MTLFontManager(MTL::Device *iDevice) : NativeFontManager{}, fDevice{iDevice} {}
  void createFontsTexture() override;
  void destroyFontsTexture() override;

private:
  MTL::Device *fDevice;
};

}

#endif //RE_EDIT_MTL_TEXTURE_MANAGER_H