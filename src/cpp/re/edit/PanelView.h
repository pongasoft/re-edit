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

#ifndef RE_EDIT_PANELVIEW_H
#define RE_EDIT_PANELVIEW_H

#include "Widget.h"
#include <re/mock/ObjectManager.hpp>

using namespace re::mock;

namespace re::edit {

class PanelView : public View
{
public:
  void draw(DrawContext &iCtx) override;

  inline void setTexture(std::shared_ptr<Texture> iTexture) { fTexture = std::move(iTexture); }
  int addWidget(std::unique_ptr<Widget> iWidget);

private:
  std::shared_ptr<Texture> fTexture{};
  ObjectManager<std::unique_ptr<Widget>> fWidgets{};
};

}

#endif //RE_EDIT_PANELVIEW_H