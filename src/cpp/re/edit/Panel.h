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
  // Implementation node: using std::function prevents kAllWidgetDefs to be a constexpr
  using factory_t = std::unique_ptr<Widget> (*)();

  WidgetType fType{};
  char const *fName{};
  factory_t fFactory{};
  PanelType fAllowedPanels{};
};

static constexpr WidgetDef kAllWidgetDefs[] {
  { WidgetType::kAnalogKnob,        "analog_knob",         Widget::analog_knob,         kPanelTypeAny },
  { WidgetType::kAudioInputSocket,  "audio_input_socket",  Widget::audio_input_socket,  PanelType::kBack },
  { WidgetType::kAudioOutputSocket, "audio_output_socket", Widget::audio_output_socket, PanelType::kBack },
  { WidgetType::kCustomDisplay,     "custom_display",      Widget::custom_display,      kPanelTypeAnyFront },
  { WidgetType::kCVInputSocket,     "cv_input_socket",     Widget::cv_input_socket,     PanelType::kBack },
  { WidgetType::kCVOutputSocket,    "cv_output_socket",    Widget::cv_output_socket,    PanelType::kBack },
  { WidgetType::kCVTrimKnob,        "cv_trim_knob",        Widget::cv_trim_knob,        PanelType::kBack },
  { WidgetType::kDeviceName,        "device_name",         Widget::device_name,         kPanelTypeAny },
  { WidgetType::kMomentaryButton,   "momentary_button",    Widget::momentary_button,    kPanelTypeAny },
  { WidgetType::kPatchBrowseGroup,  "patch_browse_group",  Widget::patch_browse_group,  kPanelTypeAny },
  { WidgetType::kPatchName,         "patch_name",          Widget::patch_name,          kPanelTypeAny },
  { WidgetType::kPitchWheel,        "pitch_wheel",         Widget::pitch_wheel,         kPanelTypeAny },
  { WidgetType::kPlaceholder,       "placeholder",         Widget::placeholder,         PanelType::kBack },
  { WidgetType::kPopupButton,       "popup_button",        Widget::popup_button,        kPanelTypeAny },
  { WidgetType::kRadioButton,       "radio_button",        Widget::radio_button,        kPanelTypeAny },
  { WidgetType::kSequenceFader,     "sequence_fader",      Widget::sequence_fader,      kPanelTypeAny },
  { WidgetType::kSequenceMeter,     "sequence_meter",      Widget::sequence_meter,      kPanelTypeAny },
  { WidgetType::kStaticDecoration,  "static_decoration",   Widget::static_decoration,   kPanelTypeAny },
  { WidgetType::kStepButton,        "step_button",         Widget::step_button,         kPanelTypeAny },
  { WidgetType::kToggleButton,      "toggle_button",       Widget::toggle_button,       kPanelTypeAny },
  { WidgetType::kUpDownButton,      "up_down_button",      Widget::up_down_button,      kPanelTypeAny },
  { WidgetType::kValueDisplay,      "value_display",       Widget::value_display,       kPanelTypeAny },
  { WidgetType::kZeroSnapKnob,      "zero_snap_knob",      Widget::zero_snap_knob,      kPanelTypeAny },
  { WidgetType::kPanelDecal,        "panel_decal",         Widget::panel_decal,         kPanelTypeAny },
};

class Panel
{
public:
  static char const *toString(PanelType iType);

public:
  explicit Panel(PanelType iType);

  char const *getName() const;
  constexpr std::string const &getNodeName() const { return fNodeName; };
  constexpr ImVec2 getSize() const { return fGraphics.getSize(); }

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

  std::string hdgui2D(EditContext &iCtx) const;
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
  void computeIsHidden(DrawContext &iCtx);
  void checkForWidgetErrors(EditContext &iCtx);
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