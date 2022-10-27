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
  ImVec2 fLastUpdatePosition{};
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
  { WidgetType::kSampleBrowseGroup, "sample_browse_group", Widget::sample_browse_group, kPanelTypeAny },
  { WidgetType::kSampleDropZone,    "sample_drop_zone",    Widget::sample_drop_zone,    kPanelTypeAny },
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

class Panel : public Editable
{
public:
  static char const *toString(PanelType iType);

public:
  explicit Panel(PanelType iType);

  char const *getName() const;
  constexpr std::string const &getNodeName() const { return fNodeName; };
  constexpr ImVec2 getSize() const { return fSize; }
  constexpr PanelType getType() const { return fType; }

  void setDeviceHeightRU(int iDeviceHeightRU);

  void draw(AppContext &iCtx);
  void editView(AppContext &iCtx);
  void editOrderView(AppContext &iCtx);
  void markEdited() override;
  void resetEdited() override;

  bool checkForErrors(AppContext &iCtx) override;

  inline void setBackground(std::shared_ptr<Texture> iBackground) { fGraphics.setTexture(std::move(iBackground)); fEdited = true; }
  inline void setBackgroundKey(Texture::key_t const &iTextureKey) { fGraphics.setTextureKey(iTextureKey); fEdited = true; }
  inline void setCableOrigin(ImVec2 const &iPosition) { fCableOrigin = iPosition; fEdited = true; }
  void setOptions(std::vector<std::string> const &iOptions);
  int addWidget(AppContext &iCtx, std::shared_ptr<Widget> iWidget, bool iMakeSelected = true);
  void duplicateWidgets(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iWidgets);
  std::shared_ptr<Widget> replaceWidget(int iWidgetId, std::shared_ptr<Widget> iWidget);
  std::vector<std::shared_ptr<Widget>> getSelectedWidgets() const;
  std::shared_ptr<Widget> getWidget(int id) const;

  void selectWidget(int id, bool iMultiple);
  void toggleWidgetSelection(int id, bool iMultiple);
  void unselectWidget(int id);
  void clearSelection();

  /**
   * @return the deleted widget and its order */
  std::pair<std::shared_ptr<Widget>, int> deleteWidget(AppContext &iCtx, int id);
  void deleteWidgets(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iWidgets);

  std::string hdgui2D(AppContext &iCtx) const;
  std::string device2D() const;

  friend class PanelState;

private:
  struct PanelWidgets
  {
    std::map<int, std::shared_ptr<Widget>> fWidgets{};
    std::vector<int> fWidgetsOrder{};
    std::vector<int> fDecalsOrder{};
  };

protected:
  void editNoSelectionView(AppContext &iCtx);
  void editSingleSelectionView(AppContext &iCtx, std::shared_ptr<Widget> const &iWidget);
  void editMultiSelectionView(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets);
  void reloadTextures();

  std::shared_ptr<PanelWidgets> freezeWidgets() const;
  std::shared_ptr<PanelWidgets> thawWidgets(std::shared_ptr<PanelWidgets> const &iPanelWidgets);

  std::shared_ptr<UndoAction> createWidgetsUndoAction(std::string const &iDescription) const;

private:
  void selectWidget(AppContext &iCtx, ImVec2 const &iPosition, bool iMultiSelectKey);
  void selectWidgets(AppContext &iCtx, ImVec2 const &iPosition1, ImVec2 const &iPosition2);
  std::shared_ptr<Widget> findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const;
  std::shared_ptr<Widget> findWidgetOnTopAt(ImVec2 const &iPosition) const;
  void moveWidgets(AppContext &iCtx, ImVec2 const &iPosition);
  void endMoveWidgets(AppContext &iCtx, ImVec2 const &iPosition);
  void computeIsHidden(AppContext &iCtx);
  void renderAddWidgetMenu(AppContext &iCtx, ImVec2 const &iPosition = {});
  bool renderSelectedWidgetsMenu(AppContext &iCtx,
                                 std::vector<std::shared_ptr<Widget>> const &iSelectedWidgets,
                                 std::optional<ImVec2> iPosition = std::nullopt);
  void renderWidgetMenu(AppContext &iCtx, std::shared_ptr<Widget> const &iWidget);
  void drawWidgets(AppContext &iCtx, std::vector<int> const &iOrder);
  void drawCableOrigin(AppContext &iCtx);

private:
  class MultiSelectionList
  {
  public:
    MultiSelectionList(Panel &iPanel, std::vector<int> &iList) : fPanel{iPanel}, fList{iList} {}
    void handleClick(std::shared_ptr<Widget> const &iWidget, bool iRangeSelectKey, bool iMultiSelectKey);
    void editView(AppContext &iCtx);
    void moveSelectionUp();
    void moveSelectionDown();
    void selectAll();
    void clearSelection();
    void duplicateSelection(AppContext &iCtx);
    void deleteSelection(AppContext &iCtx);
    std::vector<std::shared_ptr<Widget>> getSelectedWidgets() const;

  public:
    Panel &fPanel;
    std::vector<int> &fList;
    std::optional<int> fLastSelected{};
  };

private:
  PanelType fType;
  int fDeviceHeightRU{1};
  ImVec2 fSize{kDevicePixelWidth, toPixelHeight(1)};
  std::string fNodeName;
  re::edit::panel::Graphics fGraphics{};
  std::optional<ImVec2> fCableOrigin;
  std::optional<bool> fDisableSampleDropOnPanel{};
  bool fShowCableOrigin{};
  std::map<int, std::shared_ptr<Widget>> fWidgets{};
  std::vector<int> fWidgetsOrder{};
  std::vector<int> fDecalsOrder{};
  std::optional<ImVec2> fLastMovePosition{};
  std::optional<MouseDrag> fMouseDrag{};
  std::optional<MouseDrag> fShiftMouseDrag{};
  std::optional<ImVec2> fPopupLocation{};
  int fWidgetCounter{1}; // used for unique id
  MultiSelectionList fWidgetsSelectionList{*this, fWidgetsOrder};
  MultiSelectionList fDecalsSelectionList{*this, fDecalsOrder};
  mutable std::optional<std::vector<std::shared_ptr<Widget>>> fSelectedWidgets{};
};

}

#endif //RE_EDIT_PANEL_H