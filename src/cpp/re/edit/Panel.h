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

#ifndef RE_EDIT_PANEL_H
#define RE_EDIT_PANEL_H

#include "Widget.h"
#include <re/mock/ObjectManager.hpp>
#include <vector>
#include <optional>

using namespace re::mock;

namespace re::edit {

struct MouseDrag
{
  ImVec2 fInitialPosition{};
  ImVec2 fCurrentPosition{};
};

class Panel
{
public:
  enum class Type { kFront, kBack, kFoldedFront, kFoldedBack  };
public:
  explicit Panel(Type iType) : fType{iType} {}

  char const *getName() const;

  void draw(DrawContext &iCtx);
  void editView(EditContext &iCtx);

  inline void setBackground(std::shared_ptr<Texture> iBackground) { fGraphics.fTexture = std::move(iBackground); }
  int addWidget(std::unique_ptr<Widget> iWidget);
  std::vector<Widget *> getSelectedWidgets() const;

private:
  std::string getEditViewWindowName() const;

private:
  void selectWidget(ImVec2 const &iPosition, bool iMultiple);
  void moveWidgets(ImVec2 const &iPosition);
  void endMoveWidgets(ImVec2 const &iPosition);
  void checkWidgetForError(Widget &iWidget);

private:
  Type fType;
  widget::attribute::Graphics fGraphics{};
  ObjectManager<std::unique_ptr<Widget>> fWidgets{};
  std::optional<ImVec2> fLastMovePosition{};
  std::optional<MouseDrag> fMouseDrag{};
};

}

#endif //RE_EDIT_PANEL_H