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
#include <set>
#include <string>
#include <optional>

namespace re::edit {

struct MouseDrag
{
  explicit MouseDrag(ImVec2 const &iPos) : fInitialPosition{iPos}, fLastUpdatePosition{iPos}, fCurrentPosition{iPos} {}

  ImVec2 fInitialPosition{};
  ImVec2 fLastUpdatePosition{};
  ImVec2 fCurrentPosition{};
};

struct WidgetMove
{
  ImVec2 fInitialPosition{};
  ImVec2 fDelta{};
};

struct WidgetDef
{
  // Implementation node: using std::function prevents kAllWidgetDefs to be a constexpr
  using factory_t = std::unique_ptr<Widget> (*)(std::optional<std::string> const &iName);

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

  static constexpr auto kZoomMin = 0.1f;
  static constexpr auto kZoomMax = 5.0f;

public:
  explicit Panel(PanelType iType);

  char const *getName() const;
  constexpr std::string const &getNodeName() const { return fNodeName; };
  constexpr ImVec2 getSize() const { return fSize; }
  constexpr ImVec2 getCenter() const { return getSize() / 2.0f; }
  constexpr ImVec2 getTotalSize() const { return fComputedRect.GetSize(); }
  constexpr ImVec2 getTopLeftOffset() const { return {-fComputedRect.Min.x, -fComputedRect.Min.y}; }
  constexpr ReGui::Rect getRect() const { return fComputedRect; }
  constexpr PanelType getType() const { return fType; }

  void setDeviceHeightRU(int iDeviceHeightRU);

  void draw(AppContext &iCtx, ReGui::Canvas &iCanvas, ImVec2 const &iPopupWindowPadding);
  void editView(AppContext &iCtx);
  void editOrderView(AppContext &iCtx);
  void markEdited() override;
  void resetEdited() override;

  bool checkForErrors(AppContext &iCtx) override;

  inline void setBackgroundKey(Texture::key_t const &iTextureKey) { fGraphics.setTextureKey(iTextureKey); fEdited = true; }
  inline void setCableOrigin(ImVec2 const &iPosition) { fCableOrigin = iPosition; fEdited = true; }
  void setOptions(std::vector<std::string> const &iOptions);
  int addWidget(AppContext &iCtx, std::shared_ptr<Widget> iWidget, char const *iUndoActionName = "Add", bool iMakeSingleSelected = true);
  int addWidget(AppContext &iCtx, WidgetDef const &iDef, ImVec2 const &iPosition);
  bool pasteWidget(AppContext &iCtx, Widget const *iWidget, ImVec2 const &iPosition);
  bool pasteWidgets(AppContext &iCtx, std::vector<std::unique_ptr<Widget>> const &iWidgets, ImVec2 const &iPosition);
  std::shared_ptr<Widget> transmuteWidget(AppContext &iCtx, const std::shared_ptr<Widget>& iWidget, WidgetDef const &iNewDef);
  std::shared_ptr<Widget> replaceWidgetNoUndo(int iWidgetId, std::shared_ptr<Widget> iWidget);
  std::shared_ptr<Widget> getWidget(int id) const;
  std::shared_ptr<Widget> findWidget(int id) const;

  void selectWidget(int id, bool iMultiple);
  void toggleWidgetSelection(int id, bool iMultiple);
  void unselectWidget(int id);
  void selectAll(bool iIncludeHiddenWidgets = false);
  void toggleSelectAll(bool iIncludeHiddenWidgets = false);
  void selectByType(WidgetType iType, bool iIncludeHiddenWidgets = false);
  void clearSelection();

  void deleteWidgets(AppContext &iCtx, std::vector<std::shared_ptr<Widget>> const &iWidgets);

  std::string hdgui2D() const;
  std::string device2D() const;
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const;
  void collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const;

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
  void editMultiSelectionView(AppContext &iCtx);

  std::unique_ptr<PanelWidgets> freezeWidgets() const;
  std::unique_ptr<PanelWidgets> thawWidgets(std::shared_ptr<PanelWidgets> const &iPanelWidgets);

  std::shared_ptr<UndoAction> createWidgetsUndoAction(std::string const &iDescription) const;

private:
  bool selectWidget(AppContext &iCtx, ImVec2 const &iPosition, bool iMultiSelectKey);
  void selectWidgets(AppContext &iCtx, ImVec2 const &iPosition1, ImVec2 const &iPosition2);
  std::shared_ptr<Widget> findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const;
  std::shared_ptr<Widget> findWidgetOnTopAt(ImVec2 const &iPosition) const;
  void moveWidgets(AppContext &iCtx, ImVec2 const &iPosition, ImVec2 const &iGrid);
  void endMoveWidgets(AppContext &iCtx, ImVec2 const &iPosition);
  void deleteWidgetNoUndo(AppContext &iCtx, int id);
  void computeEachFrame(AppContext &iCtx);
  bool renderPanelWidgetMenu(AppContext &iCtx, ImVec2 const &iPosition = {});
  bool renderPanelMenus(AppContext &iCtx, std::optional<ImVec2> iPosition = std::nullopt);
  bool renderSelectedWidgetsMenu(AppContext &iCtx);
  bool renderWidgetMenu(AppContext &iCtx, std::shared_ptr<Widget> const &iWidget);
  void renderWidgetValues(std::shared_ptr<Widget> const &iWidget);
  void drawWidgets(AppContext &iCtx, ReGui::Canvas &iCanvas, std::vector<int> const &iOrder);
  void drawCableOrigin(AppContext &iCtx, ReGui::Canvas &iCanvas);
  void drawRails(AppContext const &iCtx, ReGui::Canvas const &iCanvas) const;
  void drawPanel(AppContext const &iCtx, ReGui::Canvas const &iCanvas) const;

  void handleLeftMouseClick(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos);
  void handleSelectWidgetsAction(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos);
  void handleMoveWidgetsAction(AppContext &iCtx, ReGui::Canvas::canvas_pos_t const &iMousePos);
  void handleMoveCanvasAction(AppContext &iCtx, ReGui::Canvas &iCanvas);
  void handleCanvasInputs(AppContext &iCtx, ReGui::Canvas &iCanvas);
  std::string computeUniqueWidgetNameForCopy(std::string const &iOriginalName) const;
  inline std::unique_ptr<Widget> copy(Widget const *iWidget) const { return iWidget->copy(computeUniqueWidgetNameForCopy(iWidget->getName())); }

private:
  class MultiSelectionList
  {
  public:
    MultiSelectionList(Panel &iPanel, char const *iType, std::vector<int> &iList) : fPanel{iPanel}, fType{iType}, fList{iList} {}
    void handleClick(std::shared_ptr<Widget> const &iWidget, bool iRangeSelectKey, bool iMultiSelectKey);
    void editView(AppContext &iCtx);
    void moveSelectionUp();
    void moveSelectionDown();

    std::vector<std::shared_ptr<Widget>> getSelectedWidgets() const;

  public:
    Panel &fPanel;
    char const *fType;
    std::vector<int> &fList;
    std::optional<int> fLastSelected{};
  };

private:
  PanelType fType;
  int fDeviceHeightRU{1};
  ImVec2 fSize{kDevicePixelWidth, toPixelHeight(1)};
  ReGui::Rect fComputedRect{{}, fSize};
  std::string fNodeName;
  re::edit::panel::Graphics fGraphics{};
  std::optional<ImVec2> fCableOrigin;
  std::optional<bool> fDisableSampleDropOnPanel{};
  bool fShowCableOrigin{};
  std::map<int, std::shared_ptr<Widget>> fWidgets{};
  std::set<StringWithHash::hash_t> fWidgetNameHashes{};
  std::vector<int> fWidgetsOrder{};
  std::vector<int> fDecalsOrder{};
  std::optional<WidgetMove> fWidgetMove{};
  std::optional<MouseDrag> fMoveWidgetsAction{};
  std::optional<MouseDrag> fSelectWidgetsAction{};
  std::optional<MouseDrag> fMoveCanvasAction{};
  std::optional<ImVec2> fPopupLocation{};
  int fWidgetCounter{1}; // used for unique id
  MultiSelectionList fWidgetsSelectionList{*this, "widget", fWidgetsOrder};
  MultiSelectionList fDecalsSelectionList{*this, "decal", fDecalsOrder};
  std::vector<std::shared_ptr<Widget>> fComputedSelectedWidgets{};
  std::optional<ReGui::Rect> fComputedSelectedRect{};
};

}

#endif //RE_EDIT_PANEL_H