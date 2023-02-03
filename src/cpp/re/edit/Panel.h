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

class PanelAction : public Action
{
public:
  friend class Panel;

protected:
  void initSelected(std::vector<Widget *> const &iSelectedWidgets);
  void undoSelection(Panel *iPanel);

protected:
  std::optional<std::set<int>> fSelectedWidgets{};
};

class PanelTx : public CompositeAction<PanelAction>
{
public:
  explicit PanelTx(std::string iDescription);

  void undo() override;

  friend class Panel;

protected:
  void addAction(std::unique_ptr<PanelAction> iAction);
  std::unique_ptr<PanelAction> first();
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
  void addWidget(AppContext &iCtx, std::unique_ptr<Widget> iWidget, bool iMakeSingleSelected, char const *iUndoActionName = "Add");
  void addWidget(AppContext &iCtx, WidgetDef const &iDef, ImVec2 const &iPosition);
  bool pasteWidget(AppContext &iCtx, Widget const *iWidget, ImVec2 const &iPosition);
  bool pasteWidgets(AppContext &iCtx, std::vector<std::unique_ptr<Widget>> const &iWidgets, ImVec2 const &iPosition);
  void transmuteWidget(AppContext &iCtx, Widget const *iWidget, WidgetDef const &iNewDef);
  Widget *getWidget(int id) const;
  Widget *findWidget(int id) const;

  enum class WidgetOrDecal { kWidget, kDecal };
  enum class Direction { kUp, kDown };
  void changeSelectedWidgetsOrder(AppContext &iCtx, WidgetOrDecal iWidgetOrDecal, Direction iDirection);
  inline std::vector<int> const &getOrder(WidgetOrDecal iType) const {
    return iType == WidgetOrDecal::kWidget ? fWidgetsOrder : fDecalsOrder;
  }

  void selectWidget(int id, bool iMultiple);
  void selectWidgets(std::set<int> const &iWidgetIds, bool iAddToSelection);
  void toggleWidgetSelection(int id, bool iMultiple);
  void unselectWidget(int id);
  void selectAll(bool iIncludeHiddenWidgets = false);
  void toggleSelectAll(bool iIncludeHiddenWidgets = false);
  void selectByType(WidgetType iType, bool iIncludeHiddenWidgets = false);
  void clearSelection();
  std::set<int> getSelectedWidgetIds() const;

  void deleteWidgets(AppContext &iCtx, std::vector<Widget *> const &iWidgets);

  std::string hdgui2D() const;
  std::string device2D() const;
  void collectUsedTexturePaths(std::set<fs::path> &oPaths) const;
  void collectUsedTextureBuiltIns(std::set<FilmStrip::key_t> &oKeys) const;

  friend class PanelState;

  // action implementations (no undo)
  int addWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget, int order);
  std::pair<std::unique_ptr<Widget>, int> deleteWidgetAction(int id);
  std::unique_ptr<Widget> replaceWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget);
  int changeWidgetsOrderAction(std::set<int> const &iWidgetIds, WidgetOrDecal iWidgetOrDecal, Direction iDirection);
  void moveWidgetsAction(std::set<int> const &iWidgetsIds, ImVec2 const &iMoveDelta);
  ImVec2 setWidgetPositionAction(int iWidgetId, ImVec2 const &iPosition);

protected:
  void editNoSelectionView(AppContext &iCtx);
  void editSingleSelectionView(AppContext &iCtx, Widget *iWidget);
  void editMultiSelectionView(AppContext &iCtx);

private:
  bool selectWidget(AppContext &iCtx, ImVec2 const &iPosition, bool iMultiSelectKey);
  void selectWidgets(AppContext &iCtx, ImVec2 const &iPosition1, ImVec2 const &iPosition2);
  Widget *findWidgetOnTopAt(std::vector<int> const &iOrder, ImVec2 const &iPosition) const;
  Widget *findWidgetOnTopAt(ImVec2 const &iPosition) const;
  void moveWidgets(AppContext &iCtx, ImVec2 const &iPosition, ImVec2 const &iGrid);
  void endMoveWidgets(AppContext &iCtx);
  bool moveWidgets(AppContext &iCtx, ImVec2 const &iDelta);
  enum class WidgetAlignment { kTop, kBottom, kLeft, kRight};
  void alignWidgets(AppContext &iCtx, WidgetAlignment iAlignment);
  void beforeEachFrame(AppContext &iCtx);
  bool renderPanelWidgetMenu(AppContext &iCtx, ImVec2 const &iPosition = {});
  bool renderPanelMenus(AppContext &iCtx, std::optional<ImVec2> iPosition = std::nullopt);
  bool renderSelectedWidgetsMenu(AppContext &iCtx);
  bool renderWidgetMenu(AppContext &iCtx, Widget *iWidget);
  void renderWidgetValues(Widget const *iWidget);
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

  template<class T, class... Args > void executeAction(AppContext &iCtx, Args&&... args);
  void executePanelAction(AppContext &iCtx, std::unique_ptr<PanelAction> iPanelAction);
  void beginTx(std::string iDescription);
  void commitTx(AppContext &iCtx);

private:
  class MultiSelectionList
  {
  public:
    MultiSelectionList(Panel &iPanel, Panel::WidgetOrDecal iType) : fPanel{iPanel}, fWidgetOrDecal{iType} {}
    void handleClick(Widget *iWidget, bool iRangeSelectKey, bool iMultiSelectKey);
    void editView(AppContext &iCtx);

  private:
    inline std::vector<int> const &getList() const { return fPanel.getOrder(fWidgetOrDecal); }

  public:
    Panel &fPanel;
    Panel::WidgetOrDecal fWidgetOrDecal;
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
  std::map<int, std::unique_ptr<Widget>> fWidgets{};
  std::vector<int> fWidgetsOrder{};
  std::vector<int> fDecalsOrder{};
  std::set<StringWithHash::hash_t> fWidgetNameHashes{};
  std::optional<WidgetMove> fWidgetMove{};
  std::optional<MouseDrag> fMoveWidgetsAction{};
  std::optional<MouseDrag> fSelectWidgetsAction{};
  std::optional<MouseDrag> fMoveCanvasAction{};
  std::optional<ImVec2> fPopupLocation{};
  int fWidgetCounter{1}; // used for unique id
  std::unique_ptr<PanelTx> fPanelTx{};
  MultiSelectionList fWidgetsSelectionList{*this, Panel::WidgetOrDecal::kWidget};
  MultiSelectionList fDecalsSelectionList{*this, Panel::WidgetOrDecal::kDecal};
  std::vector<Widget *> fComputedSelectedWidgets{};
  std::optional<ReGui::Rect> fComputedSelectedRect{};
};

}

#endif //RE_EDIT_PANEL_H