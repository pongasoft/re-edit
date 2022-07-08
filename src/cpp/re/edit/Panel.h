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
#include <vector>
#include <optional>

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
  static char const *toString(Type iType);

public:
  explicit Panel(Type iType);

  char const *getName() const;
  constexpr std::string const &getNodeName() const { return fNodeName; };

  void draw(DrawContext &iCtx);
  void editView(EditContext &iCtx);

  inline void setBackground(std::shared_ptr<Texture> iBackground) { fGraphics.setTexture(std::move(iBackground)); }
  int addWidget(std::shared_ptr<Widget> iWidget);
  std::vector<Widget *> getSelectedWidgets() const;
  std::vector<int> getWidgetOrder() const { return fWidgetOrder; }
  Widget *getWidget(int id) const;

  void selectWidget(int id, bool iMultiple);
  void clearSelection();

  /**
   * @return the deleted widget and its order */
  std::pair<std::shared_ptr<Widget>, int> deleteWidget(int id);

  void swap(int iIndex1, int iIndex2);

  std::string hdgui2D() const;

private:
  std::string getEditViewWindowName() const;

private:
  void selectWidget(ImVec2 const &iPosition, bool iMultiple);
  void moveWidgets(ImVec2 const &iPosition);
  void endMoveWidgets(ImVec2 const &iPosition);
  void checkWidgetForError(Widget &iWidget);

private:
  Type fType;
  std::string fNodeName;
  widget::attribute::Graphics fGraphics{};
  std::map<int, std::shared_ptr<Widget>> fWidgets{};
  std::vector<int> fWidgetOrder{};
  std::optional<ImVec2> fLastMovePosition{};
  std::optional<MouseDrag> fMouseDrag{};
  int fWidgetCounter{1}; // used for unique id
};

}

#endif //RE_EDIT_PANEL_H