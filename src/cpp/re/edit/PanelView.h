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

#include "ControlView.h"
#include <re/mock/ObjectManager.hpp>
#include <set>
#include <optional>

using namespace re::mock;

namespace re::edit {

struct MouseDrag
{
  ImVec2 fInitialPosition{};
  ImVec2 fCurrentPosition{};
};


class PanelView : public View
{
public:
  void draw(DrawContext &iCtx) override;

  inline void setBackground(std::shared_ptr<Texture> iBackground) { fBackground = std::move(iBackground); }
  int addControl(std::unique_ptr<ControlView> iControl);
  ControlView *getSelectedControl() const;

private:
  void clearSelectedControls();
  void selectControl(ImVec2 const &iPosition, bool iMultiple);
  void moveControls(ImVec2 const &iPosition);
  void endMoveControls(ImVec2 const &iPosition);

private:
  std::shared_ptr<Texture> fBackground{};
  ObjectManager<std::unique_ptr<ControlView>> fControls{};
  std::optional<ImVec2> fLastMovePosition{};
  std::set<ControlView *> fSelectedControls{};
  std::optional<MouseDrag> fMouseDrag{};
};

}

#endif //RE_EDIT_PANELVIEW_H