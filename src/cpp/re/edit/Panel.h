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

struct WidgetDef
{
  WidgetType fType{};
  char const *fName{};
  std::function<std::unique_ptr<Widget>()> fFactory{};
  PanelType fAllowedPanels{};
};

static const WidgetDef kAllWidgetDefs[] {
  { WidgetType::kAnalogKnob,        "analog_knob",         Widget::analog_knob,         PanelType::kAnyFront },
  { WidgetType::kAudioInputSocket,  "audio_input_socket",  Widget::audio_input_socket,  PanelType::kBack },
  { WidgetType::kAudioOutputSocket, "audio_output_socket", Widget::audio_output_socket, PanelType::kBack },
  { WidgetType::kCustomDisplay,     "custom_display",      Widget::custom_display,      PanelType::kAnyFront },
  { WidgetType::kCVInputSocket,     "cv_input_socket",     Widget::cv_input_socket,     PanelType::kBack },
  { WidgetType::kCVOutputSocket,    "cv_output_socket",    Widget::cv_output_socket,    PanelType::kBack },
  { WidgetType::kDeviceName,        "device_name",         Widget::device_name,         PanelType::kAny },
  { WidgetType::kPlaceholder,       "placeholder",         Widget::placeholder,         PanelType::kBack },
  { WidgetType::kSequenceFader,     "sequence_fader",      Widget::sequence_fader,      PanelType::kAny },
  { WidgetType::kStaticDecoration,  "static_decoration",   Widget::static_decoration,   PanelType::kAny },
  { WidgetType::kPanelDecal,        "panel_decal",         Widget::panel_decal,         PanelType::kAny },
};

class Panel
{
public:
  static char const *toString(PanelType iType);

public:
  explicit Panel(PanelType iType);

  char const *getName() const;
  constexpr std::string const &getNodeName() const { return fNodeName; };

  void setDeviceHeightRU(int iDeviceHeightRU);

  void draw(DrawContext &iCtx);
  void editView(EditContext &iCtx);
  void editOrderView(EditContext &iCtx);

  inline void setBackground(std::shared_ptr<Texture> iBackground) { fGraphics.setTexture(std::move(iBackground)); }
  inline void setCableOrigin(ImVec2 const &iPosition) { fCableOrigin = iPosition; }
  int addWidget(std::shared_ptr<Widget> iWidget);
  std::vector<std::shared_ptr<Widget>> getSelectedWidgets() const;
  std::vector<int> getWidgetsOrder() const { return fWidgetsOrder; }
  std::vector<int> getDecalsOrder() const { return fDecalsOrder; }
  std::shared_ptr<Widget> getWidget(int id) const;

  void selectWidget(int id, bool iMultiple);
  void unselectWidget(int id);
  void clearSelection();

  /**
   * @return the deleted widget and its order */
  std::pair<std::shared_ptr<Widget>, int> deleteWidget(int id);

  void swapWidgets(int iIndex1, int iIndex2);
  void swapDecals(int iIndex1, int iIndex2);

  std::string hdgui2D() const;
  std::string device2D() const;

  friend class PanelState;

protected:
  template<typename F>
  void editOrderView(std::vector<int> const &iOrder, F iOnSwap);

private:
  void selectWidget(DrawContext &iCtx, ImVec2 const &iPosition, bool iMultiple);
  std::shared_ptr<Widget> findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const;
  std::shared_ptr<Widget> findWidgetOnTopAt(ImVec2 const &iPosition) const;
  void moveWidgets(ImVec2 const &iPosition);
  void endMoveWidgets(ImVec2 const &iPosition);
  void checkWidgetForError(Widget &iWidget);
  void computeIsHidden(DrawContext &iCtx);
  void renderAddWidgetMenu(EditContext &iCtx, ImVec2 const &iPosition = {});
  bool renderSelectedWidgetsMenu(std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets,
                                 std::optional<ImVec2> iPosition = std::nullopt);
  void renderWidgetMenu(std::shared_ptr<Widget> const &iWidget);
  void drawWidgets(DrawContext &iCtx, std::vector<int> const &iOrder);
  void drawCableOrigin(DrawContext &iCtx);

private:
  PanelType fType;
  int fDeviceHeightRU{1};
  std::string fNodeName;
  widget::attribute::Graphics fGraphics{};
  std::optional<ImVec2> fCableOrigin;
  bool fShowCableOrigin{};
  std::map<int, std::shared_ptr<Widget>> fWidgets{};
  std::vector<int> fWidgetsOrder{};
  std::vector<int> fDecalsOrder{};
  std::optional<ImVec2> fLastMovePosition{};
  std::optional<MouseDrag> fMouseDrag{};
  int fWidgetCounter{1}; // used for unique id
};

}

#endif //RE_EDIT_PANEL_H