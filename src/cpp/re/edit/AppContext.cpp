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

#include "AppContext.h"
#include "Widget.h"
#include "PanelState.h"
#include "imgui_internal.h"
#include "Application.h"
#include "Utils.h"
#include <regex>
#include <efsw/efsw.hpp>
#include <nfd.h>

namespace re::edit {

namespace impl {

class UpdateListener : public efsw::FileWatchListener
{
public:
  UpdateListener(AppContext &iCtx, fs::path const &iRoot) :
    fCtx{iCtx}, fRoot{fs::canonical(iRoot)}
  {
    // empty
  }

  void handleFileAction(efsw::WatchID watchid,
                        const std::string &dir,
                        const std::string &filename,
                        efsw::Action action,
                        std::string oldFilename) override
  {
    processFile(fs::path(dir) / filename);
    if(action == efsw::Actions::Moved)
      processFile(fs::path(dir) / oldFilename);
  }

  void processFile(fs::path const &iFile)
  {
    static const std::regex FILENAME_REGEX{"(([0-9]+)_?frames)?\\.png$", std::regex_constants::icase};

    if(fs::is_directory(iFile))
      return;

    std::error_code errorCode;
    auto file = fs::weakly_canonical(iFile, errorCode);
    if(errorCode)
    {
      RE_EDIT_LOG_WARNING("Cannot convert %s to canonical form", iFile);
      return;
    }

    if(file == fRoot / "motherboard_def.lua" || file == fRoot / "info.lua")
    {
      // trigger maybe reloadDevice
      fCtx.maybeReloadDevice(true);
    }
    else
    {
      if(file.parent_path() == fRoot / "GUI2D")
      {
        std::cmatch m;
        if(std::regex_search(file.filename().u8string().c_str(), m, FILENAME_REGEX))
        {
          // trigger maybe scanDirectory
          fCtx.maybeReloadTextures(true);
        }
      }
    }
  }

private:
  AppContext &fCtx;
  fs::path fRoot;
};

}

//------------------------------------------------------------------------
// AppContext::AppContext
//------------------------------------------------------------------------
AppContext::AppContext(fs::path iRoot) :
  fRoot{fs::canonical(iRoot)},
  fFrontPanel(std::make_unique<PanelState>(PanelType::kFront)),
  fFoldedFrontPanel(std::make_unique<PanelState>(PanelType::kFoldedFront)),
  fBackPanel(std::make_unique<PanelState>(PanelType::kBack)),
  fFoldedBackPanel(std::make_unique<PanelState>(PanelType::kFoldedBack)),
  fRootWatcher{std::make_shared<efsw::FileWatcher>()}
{
  fCurrentPanelState = fFrontPanel.get();
}

//------------------------------------------------------------------------
// AppContext::~AppContext
//------------------------------------------------------------------------
AppContext::~AppContext()
{
  disableFileWatcher();
}

//------------------------------------------------------------------------
// AppContext::initPanels
//------------------------------------------------------------------------
void AppContext::initPanels(fs::path const &iDevice2DFile, fs::path const &iHDGui2DFile)
{
  auto d2d = lua::Device2D::fromFile(iDevice2DFile);
  auto hdg = lua::HDGui2D::fromFile(iHDGui2DFile);
  auto merge = [](std::map<std::string, int> &m1, std::map<std::string, int> const &m2) {
    for(auto &[k, numFrame]: m2)
    {
      auto numFrame2 = m1.find(k);
      if(numFrame2 != m1.end())
      {
        if(numFrame != numFrame2->second)
          RE_EDIT_LOG_WARNING("Inconsistent number of frames for %s : %d and %d", k, numFrame2->second, numFrame);
      }
      else
        m1[k] = numFrame;
    }
  };
  std::map<std::string, int> numFrames{};
  merge(numFrames, fFrontPanel->initPanel(*this, d2d->front(), hdg->front()));
  merge(numFrames, fBackPanel->initPanel(*this, d2d->back(), hdg->back()));
  if(fHasFoldedPanels)
  {
    merge(numFrames, fFoldedFrontPanel->initPanel(*this, d2d->folded_front(), hdg->folded_front()));
    merge(numFrames, fFoldedBackPanel->initPanel(*this, d2d->folded_back(), hdg->folded_back()));
  }
  fTextureManager->overrideNumFrames(numFrames);
  markEdited();
  checkForErrors();
}

//------------------------------------------------------------------------
// AppContext::renderTabs
//------------------------------------------------------------------------
void AppContext::renderTabs()
{
  if(ImGui::BeginTabBar("Panels", ImGuiTabBarFlags_None))
  {
    if(fFrontPanel->renderTab(*this))
      fCurrentPanelState = fFrontPanel.get();
    if(fBackPanel->renderTab(*this))
      fCurrentPanelState = fBackPanel.get();
    if(fHasFoldedPanels)
    {
      if(fFoldedFrontPanel->renderTab(*this))
        fCurrentPanelState = fFoldedFrontPanel.get();
      if(fFoldedBackPanel->renderTab(*this))
        fCurrentPanelState = fFoldedBackPanel.get();
    }

    ImGui::EndTabBar();
  }
}

//------------------------------------------------------------------------
// AppContext::render
//------------------------------------------------------------------------
void AppContext::render()
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);
  fCurrentPanelState->render(*this);
  fPreviousPanelState = fCurrentPanelState;

  int flags = needsSaving() ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

  if(auto l = fMainWindow.begin(flags))
  {
    renderTabs();

    ImGui::PushID("Rendering");

    ImGui::PushID("Widget");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Widget          ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fWidgetRendering, AppContext::EWidgetRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fWidgetRendering, AppContext::EWidgetRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("X-Ray ", &fWidgetRendering, AppContext::EWidgetRendering::kXRay);
    ImGui::PopID();

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Border          ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fBorderRendering, AppContext::EBorderRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fBorderRendering, AppContext::EBorderRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("Hit B.", &fBorderRendering, AppContext::EBorderRendering::kHitBoundaries);
    ImGui::PopID();

    ImGui::PushID("SizeOnly");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("No Graphics     ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Border", &fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kBorder);
    ImGui::SameLine();
    ReGui::TextRadioButton("Fill  ", &fNoGraphicsRendering, AppContext::ENoGraphicsRendering::kFill);
    ImGui::PopID();

    ImGui::PushID("Custom Display");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Custom Display  ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Main  ", &fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kMain);
    ImGui::SameLine();
    ReGui::TextRadioButton("SD Bg.", &fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundSD);
    ImGui::SameLine();
    ReGui::TextRadioButton("HD Bg.", &fCustomDisplayRendering, AppContext::ECustomDisplayRendering::kBackgroundHD);
    ImGui::PopID();

    ImGui::PushID("Sample Drop Zone");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sample Drop Zone");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Fill  ", &fSampleDropZoneRendering, AppContext::ESampleDropZoneRendering::kFill);
    ImGui::PopID();

    ImGui::PopID();
    ImGui::Separator();

    if(maybeReloadTextures())
    {
      ImGui::AlignTextToFramePadding();
      ReGui::TipIcon();ImGui::SameLine();ImGui::TextUnformatted("Detected image changes");
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_RescanImages, "Rescan")))
        fReloadTexturesRequested = true;
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_Reset, "Dismiss")))
        maybeReloadTextures(false);
      ImGui::Separator();
    }

    if(maybeReloadDevice())
    {
      ImGui::AlignTextToFramePadding();
      ReGui::TipIcon();ImGui::SameLine();ImGui::TextUnformatted("Detected device changes");
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_ReloadMotherboard, "Reload")))
        fReloadDeviceRequested = true;
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_Reset, "Dismiss")))
        maybeReloadDevice(false);
      ImGui::Separator();
    }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

  }

}

//------------------------------------------------------------------------
// AppContext::renderAddWidgetMenuView
//------------------------------------------------------------------------
void AppContext::renderAddWidgetMenuView(ImVec2 const &iPosition)
{
  for(auto const &def: fCurrentPanelState->fWidgetDefs)
  {
    if(ImGui::MenuItem(def.fName))
    {
      auto widget = def.fFactory();
      widget->setPosition(iPosition);
      fCurrentPanelState->fPanel.addWidget(*this, std::move(widget));
    }
  }
}

//------------------------------------------------------------------------
// AppContext::getPanelState
//------------------------------------------------------------------------
PanelState *AppContext::getPanelState(PanelType iType) const
{
  switch(iType)
  {
    case PanelType::kFront:
      return fFrontPanel.get();
    case PanelType::kBack:
      return fBackPanel.get();
    case PanelType::kFoldedFront:
      return fFoldedFrontPanel.get();
    case PanelType::kFoldedBack:
      return fFoldedBackPanel.get();

    default:
      RE_EDIT_FAIL("should not be here");
  }
}

//------------------------------------------------------------------------
// AppContext::getPanel
//------------------------------------------------------------------------
Panel *AppContext::getPanel(PanelType iType) const
{
  return &getPanelState(iType)->fPanel;
}

//------------------------------------------------------------------------
// AppContext::getCurrentPanel
//------------------------------------------------------------------------
Panel *AppContext::getCurrentPanel() const
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);
  return &fCurrentPanelState->fPanel;
}


//------------------------------------------------------------------------
// AppContext::getPanelSize
//------------------------------------------------------------------------
ImVec2 AppContext::getCurrentPanelSize() const
{
  return getCurrentPanel()->getSize();
}

//------------------------------------------------------------------------
// AppContext::TextureItem
//------------------------------------------------------------------------
void AppContext::TextureItem(Texture const *iTexture,
                             ImVec2 const &iPosition,
                             int iFrameNumber,
                             ImU32 iBorderColor,
                             ImU32 iTextureColor) const
{
  iTexture->Item(iPosition, fZoom, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// AppContext::drawTexture
//------------------------------------------------------------------------
void AppContext::drawTexture(Texture const *iTexture,
                             ImVec2 const &iPosition,
                             int iFrameNumber,
                             ImU32 iBorderColor,
                             ImU32 iTextureColor) const
{
  iTexture->draw(iPosition, fZoom, iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// AppContext::drawRect
//------------------------------------------------------------------------
void AppContext::drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor);
}

//------------------------------------------------------------------------
// AppContext::RectFilledItem
//------------------------------------------------------------------------
void AppContext::RectFilledItem(ImVec2 const &iPosition,
                                ImVec2 const &iSize,
                                ImU32 iColor,
                                float iRounding,
                                ImDrawFlags iFlags) const
{
  auto const cp = ImGui::GetCursorScreenPos() + iPosition * fZoom;

  const ImRect rect{cp, cp + iSize};

  ImGui::SetCursorScreenPos(cp);
  ImGui::ItemSize(rect);
  if(!ImGui::ItemAdd(rect, 0))
      return;

  ImGui::SetCursorScreenPos(cp);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(cp, {cp.x + (iSize.x * fZoom), cp.y + (iSize.y * fZoom)}, iColor, iRounding, iFlags);
}


//------------------------------------------------------------------------
// AppContext::drawRectFilled
//------------------------------------------------------------------------
void AppContext::drawRectFilled(ImVec2 const &iPosition,
                                 ImVec2 const &iSize,
                                 ImU32 iColor,
                                 float iRounding,
                                 ImDrawFlags iFlags) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * fZoom);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(pos, {pos.x + (iSize.x * fZoom), pos.y + (iSize.y * fZoom)}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// AppContext::drawLine
//------------------------------------------------------------------------
void AppContext::drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddLine(cp + iP1 * fZoom, cp + iP2 * fZoom, iColor, iThickness);
}

//------------------------------------------------------------------------
// AppContext::addUndoAction
//------------------------------------------------------------------------
void AppContext::addUndoAction(std::shared_ptr<UndoAction> iAction)
{
  iAction->fFrame = fCurrentFrame;
  if(fCurrentPanelState)
    iAction->fPanelType = fCurrentPanelState->getType();
  if(fUndoTransaction)
  {
    if(!iAction->fMergeKey)
      iAction->fMergeKey = fUndoTransaction->fMergeKey;
    fUndoTransaction->add(std::move(iAction));
  }
  else
    fUndoManager->addUndoAction(std::move(iAction));
}

//------------------------------------------------------------------------
// AppContext::populateLambdaUndoAction
//------------------------------------------------------------------------
void AppContext::populateWidgetUndoAction(WidgetUndoAction *iAction, Widget const *iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  iAction->fWidget = iWidget->clone();
  iAction->fWidgetId = iWidget->getId();
}

//------------------------------------------------------------------------
// AppContext::computeUpdateDescription
//------------------------------------------------------------------------
std::string AppContext::computeUpdateDescription(Widget const *iWidget, widget::Attribute const *iAttribute)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iAttribute)
    return fmt::printf("Update %s.%s", iWidget->getName(), iAttribute->fName);
  else
    return fmt::printf("Update %s", iWidget->getName());
}

//------------------------------------------------------------------------
// AppContext::computeResetDescription
//------------------------------------------------------------------------
std::string AppContext::computeResetDescription(Widget const *iWidget, widget::Attribute const *iAttribute)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iAttribute)
    return fmt::printf("Reset %s.%s", iWidget->getName(), iAttribute->fName);
  else
    return fmt::printf("Reset %s", iWidget->getName());
}

//------------------------------------------------------------------------
// AppContext::addUndoWidgetChange
//------------------------------------------------------------------------
void AppContext::addUndoWidgetChange(Widget const *iWidget, std::string iDescription)
{
  auto action = std::make_unique<WidgetUndoAction>();
  action->fWidgetId = iWidget->getId();
  action->fDescription = std::move(iDescription);
  populateWidgetUndoAction(action.get(), iWidget);
  addUndoAction(std::move(action));
}

//------------------------------------------------------------------------
// AppContext::beginUndoTx
//------------------------------------------------------------------------
bool AppContext::beginUndoTx(std::string const &iDescription, void *iMergeKey)
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction == nullptr); // no support for nested transactions

  auto last = fUndoManager->getLastUndoAction();

  if(iMergeKey != nullptr && last && last->getMergeKey() == iMergeKey)
  {
    return false;
  }
  else
  {
    fUndoTransaction = std::make_unique<CompositeUndoAction>();
    fUndoTransaction->fFrame = fCurrentFrame;
    if(fCurrentPanelState)
      fUndoTransaction->fPanelType = fCurrentPanelState->getType();
    fUndoTransaction->fDescription = iDescription;
    fUndoTransaction->fMergeKey = iMergeKey;
    return true;
  }
}

//------------------------------------------------------------------------
// AppContext::commitUndoTx
//------------------------------------------------------------------------
void AppContext::commitUndoTx()
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction != nullptr);
  fUndoManager->addUndoAction(std::move(fUndoTransaction));
}

//------------------------------------------------------------------------
// AppContext::resetUndoMergeKey
//------------------------------------------------------------------------
void AppContext::resetUndoMergeKey()
{
  auto last = fUndoManager->getLastUndoAction();
  if(last)
    last->resetMergeKey();
}

//------------------------------------------------------------------------
// AppContext::init
//------------------------------------------------------------------------
void AppContext::init(lua::Config const &iConfig)
{
  fNativeWindowWidth = iConfig.fNativeWindowWidth;
  fNativeWindowHeight = iConfig.fNativeWindowHeight;
  fPanelWindow.setIsVisible(iConfig.fShowPanel);
  fPanelWidgetsWindow.setIsVisible(iConfig.fShowPanelWidgets);
  fPropertiesWindow.setIsVisible(iConfig.fShowProperties);
  fWidgetsWindow.setIsVisible(iConfig.fShowWidgets);
  fFontManager->requestNewFont({"JetBrains Mono Regular", BuiltInFont::kJetBrainsMonoRegular, iConfig.fFontSize});
  fGrid = iConfig.fGrid;
//  fShowBorder = static_cast<ShowBorder>(iConfig.fShowBorder);
//  fShowCustomDisplay = static_cast<ShowCustomDisplay>(iConfig.fShowCustomDisplay);
//  fShowSampleDropZone = static_cast<ShowSampleDropZone>(iConfig.fShowSampleDropZone);
  enableFileWatcher();
}

//------------------------------------------------------------------------
// AppContext::getLuaConfig
//------------------------------------------------------------------------
std::string AppContext::getLuaConfig() const
{
  std::stringstream s{};

  s << fmt::printf("re_edit[\"native_window_width\"] = %d\n", fNativeWindowWidth);
  s << fmt::printf("re_edit[\"native_window_height\"] = %d\n", fNativeWindowHeight);
  s << fmt::printf("re_edit[\"show_panel\"] = %s\n", fmt::Bool::to_chars(fPanelWindow.isVisible()));
  s << fmt::printf("re_edit[\"show_panel_widgets\"] = %s\n", fmt::Bool::to_chars(fPanelWidgetsWindow.isVisible()));
  s << fmt::printf("re_edit[\"show_properties\"] = %s\n", fmt::Bool::to_chars(fPropertiesWindow.isVisible()));
  s << fmt::printf("re_edit[\"show_widgets\"] = %s\n", fmt::Bool::to_chars(fWidgetsWindow.isVisible()));
  s << fmt::printf("re_edit[\"font_size\"] = %d\n", static_cast<int>(fFontManager->getCurrentFont().fSize));
  s << fmt::printf("re_edit[\"grid\"] = { %d, %d }\n", static_cast<int>(fGrid.x), static_cast<int>(fGrid.y));
//  s << fmt::printf("re_edit[\"show_border\"] = %d\n", static_cast<int>(fShowBorder));
//  s << fmt::printf("re_edit[\"show_custom_display\"] = %d\n", static_cast<int>(fShowCustomDisplay));
//  s << fmt::printf("re_edit[\"show_sample_drop_zone\"] = %d\n", static_cast<int>(fShowSampleDropZone));

  return s.str();
}

//------------------------------------------------------------------------
// AppContext::onNativeWindowFontDpiScaleChange
//------------------------------------------------------------------------
void AppContext::onNativeWindowFontDpiScaleChange(float iFontDpiScale)
{
  fFontManager->setDpiFontScale(iFontDpiScale);
}

//------------------------------------------------------------------------
// AppContext::onNativeWindowFontScaleChange
//------------------------------------------------------------------------
void AppContext::onNativeWindowFontScaleChange(float iFontScale)
{
  fFontManager->setFontScale(iFontScale);
}

//------------------------------------------------------------------------
// AppContext::reloadTextures
//------------------------------------------------------------------------
void AppContext::reloadTextures()
{
  markEdited();
  checkForErrors();
}

//------------------------------------------------------------------------
// AppContext::initDevice
//------------------------------------------------------------------------
void AppContext::initDevice()
{
  auto propertyManager = std::make_shared<PropertyManager>();
  auto info = propertyManager->init(fRoot);
  fHasFoldedPanels = info.fDeviceType != mock::DeviceType::kNotePlayer;
  fFrontPanel->fPanel.setDeviceHeightRU(info.fDeviceHeightRU);
  fBackPanel->fPanel.setDeviceHeightRU(info.fDeviceHeightRU);
  if(fHasFoldedPanels)
  {
    fFoldedFrontPanel->fPanel.setDeviceHeightRU(info.fDeviceHeightRU);
    fFoldedBackPanel->fPanel.setDeviceHeightRU(info.fDeviceHeightRU);
  }
  fPropertyManager = std::move(propertyManager);
  fMainWindow.setName(info.fMediumName);
}

//------------------------------------------------------------------------
// AppContext::initGUI2D
//------------------------------------------------------------------------
void AppContext::initGUI2D()
{
  fTextureManager->init(fRoot / "GUI2D");
  fTextureManager->scanDirectory();
  initPanels(fRoot / "GUI2D" / "device_2D.lua", fRoot / "GUI2D" / "hdgui_2D.lua");
}

//------------------------------------------------------------------------
// AppContext::reloadDevice
//------------------------------------------------------------------------
void AppContext::reloadDevice()
{
  initDevice();
  checkForErrors();
}

//------------------------------------------------------------------------
// AppContext::markEdited
//------------------------------------------------------------------------
void AppContext::markEdited()
{
  fFrontPanel->fPanel.markEdited();
  fBackPanel->fPanel.markEdited();
  if(fHasFoldedPanels)
  {
    fFoldedFrontPanel->fPanel.markEdited();
    fFoldedBackPanel->fPanel.markEdited();
  }
}

//------------------------------------------------------------------------
// AppContext::checkForErrors
//------------------------------------------------------------------------
bool AppContext::checkForErrors()
{
  auto currentPanel = fCurrentPanelState;
  auto res = false;

  fCurrentPanelState = fFrontPanel.get();
  res |= fFrontPanel->fPanel.checkForErrors(*this);

  fCurrentPanelState = fBackPanel.get();
  res |= fBackPanel->fPanel.checkForErrors(*this);

  if(fHasFoldedPanels)
  {
    fCurrentPanelState = fFoldedFrontPanel.get();
    res |= fFoldedFrontPanel->fPanel.checkForErrors(*this);

    fCurrentPanelState = fFoldedBackPanel.get();
    res |= fFoldedBackPanel->fPanel.checkForErrors(*this);
  }

  fCurrentPanelState = currentPanel;
  return res;
}

//------------------------------------------------------------------------
// AppContext::renderMainMenu
//------------------------------------------------------------------------
void AppContext::renderMainMenu()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("Edit"))
    {
      // Undo
      {
        auto const undoAction = fUndoManager->getLastUndoAction();
        if(undoAction)
        {
          resetUndoMergeKey();
          auto desc = re::mock::fmt::printf(ReGui_Prefix(ReGui_Icon_Undo, "Undo %s"), undoAction->fDescription);
          if(fCurrentPanelState && fCurrentPanelState->getType() != undoAction->fPanelType)
          {
            if(undoAction->fPanelType == PanelType::kUnknown)
              RE_EDIT_LOG_WARNING("unknown panel type for %s", undoAction->fDescription);
            else
              desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(undoAction->fPanelType));
          }
          if(ImGui::MenuItem(desc.c_str()))
          {
            undoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Undo, "Undo"));
          ImGui::EndDisabled();
        }
      }

      // Redo
      {
        auto const redoAction = fUndoManager->getLastRedoAction();
        if(redoAction)
        {
          auto const undoAction = redoAction->fUndoAction;
          auto desc = re::mock::fmt::printf(ReGui_Prefix(ReGui_Icon_Redo, "Redo %s"), undoAction->fDescription);
          if(fCurrentPanelState && fCurrentPanelState->getType() != undoAction->fPanelType)
          {
            if(undoAction->fPanelType == PanelType::kUnknown)
              RE_EDIT_LOG_WARNING("unknown panel type for %s", undoAction->fDescription);
            else
              desc = re::mock::fmt::printf("%s (%s)", desc, Panel::toString(undoAction->fPanelType));
          }
          if(ImGui::MenuItem(desc.c_str()))
          {
            redoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Redo, "Redo"));
          ImGui::EndDisabled();
        }
      }

      ImGui::BeginDisabled(!fUndoManager->hasHistory());
      if(ImGui::MenuItem("Clear Undo History"))
      {
        fUndoManager->clear();
        fLastSavedUndoAction = nullptr;
      }
      ImGui::EndDisabled();

      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("File"))
    {
//      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Open, "Load")))
//      {
//        nfdchar_t *outPath;
//        nfdresult_t result = NFD_PickFolder(&outPath, nullptr);
//        if(result == NFD_OKAY)
//        {
//          RE_EDIT_LOG_INFO("Success %s", outPath);
//          NFD_FreePath(outPath);
//        }
//        else if(result == NFD_CANCEL)
//        {
//          RE_EDIT_LOG_INFO("Cancel");
//        }
//        else
//        {
//          RE_EDIT_LOG_ERROR("Error: %s\n", NFD_GetError());
//        }
//      }
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Save, "Save")))
      {
        Application::GetCurrent().newDialog("Save")
          .preContentMessage("!!! Warning !!!")
          .text("This is an experimental build. Saving will override hdgui_2d.lua and device_2d.lua\nAre you sure you want to proceed?")
          .button("Ok", [this] { save(); return ReGui::Dialog::Result::kContinue; })
          .buttonCancel("Cancel", true)
          ;
      }
      ImGui::Separator();
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_RescanImages, "Rescan images")))
      {
        fReloadTexturesRequested = true;
      }
      if(fMaybeReloadTextures)
      {
        ImGui::SameLine();
        ImGui::TextUnformatted("\u00b7");
      }
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ReloadMotherboard, "Reload motherboard")))
      {
        fReloadDeviceRequested = true;
      }
      if(fMaybeReloadDevice)
      {
        ImGui::SameLine();
        ImGui::TextUnformatted("\u00b7");
      }

      ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Window"))
    {
      fPanelWindow.menuItem();
      fPanelWidgetsWindow.menuItem();
      fWidgetsWindow.menuItem();
      fPropertiesWindow.menuItem();
      ImGui::Separator();
      if(ImGui::MenuItem("Horizontal Layout"))
        fNewLayoutRequested = lua::kDefaultHorizontalLayout;
      if(ImGui::MenuItem("Vertical Layout"))
        fNewLayoutRequested = lua::kDefaultVerticalLayout;
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

//------------------------------------------------------------------------
// AppContext::beforeRenderFrame
//------------------------------------------------------------------------
void AppContext::beforeRenderFrame()
{
  fCurrentFrame++;
  fPropertyManager->beforeRenderFrame();

  if(fRecomputeDimensionsRequested)
  {
    fItemWidth = 40 * ImGui::CalcTextSize("W").x;
    fRecomputeDimensionsRequested = false;
  }

  if(fReloadTexturesRequested)
  {
    fReloadTexturesRequested = false;
    fMaybeReloadTextures = false;
    fTextureManager->scanDirectory();
    reloadTextures();
  }

  if(fReloadDeviceRequested)
  {
    fReloadDeviceRequested = false;
    fMaybeReloadDevice = false;
    try
    {
      reloadDevice();
    }
    catch(...)
    {
      Application::GetCurrent().newDialog("Error")
        .preContentMessage("Error while reloading rack extension definition")
        .text(Application::what(std::current_exception()), true)
        .buttonCancel("Ok");
    }
  }

  fNeedsSaving = fUndoManager->getLastUndoAction() != fLastSavedUndoAction;
}

//------------------------------------------------------------------------
// AppContext::newFrame
//------------------------------------------------------------------------
void AppContext::newFrame()
{
  if(fNewLayoutRequested)
  {
    auto newLayoutRequest = *fNewLayoutRequested;
    fNewLayoutRequested = std::nullopt;
    ImGui::LoadIniSettingsFromMemory(newLayoutRequest.c_str(), newLayoutRequest.size());
  }

  if(fFontManager->hasFontChangeRequest())
  {
    auto oldDpiScale = fFontManager->getCurrentFontDpiScale();
    fFontManager->applyFontChangeRequest();
    auto newDpiScale = fFontManager->getCurrentFontDpiScale();

    if(oldDpiScale != newDpiScale)
    {
      auto scaleFactor = newDpiScale;
      ImGuiStyle newStyle{};
      ImGui::StyleColorsDark(&newStyle);
      newStyle.ScaleAllSizes(scaleFactor);
      ImGui::GetStyle() = newStyle;
    }

    fRecomputeDimensionsRequested = true;
  }

}

//------------------------------------------------------------------------
// AppContext::afterRenderFrame
//------------------------------------------------------------------------
void AppContext::afterRenderFrame()
{
  fPropertyManager->afterRenderFrame();
}

//------------------------------------------------------------------------
// AppContext::save
//------------------------------------------------------------------------
void AppContext::save()
{
  disableFileWatcher();
  auto deferred = Utils::defer([this] { enableFileWatcher(); });
  Application::saveFile(fRoot / "GUI2D" / "device_2D.lua", device2D());
  Application::saveFile(fRoot / "GUI2D" / "hdgui_2D.lua", hdgui2D());
  Application::saveFile(fRoot / "GUI2D" / "gui_2D.cmake", cmake());
  saveConfig();
//  fAppContext->fUndoManager->clear();
  fNeedsSaving = false;
  fLastSavedUndoAction = fUndoManager->getLastUndoAction();
  ImGui::GetIO().WantSaveIniSettings = false;
}

//------------------------------------------------------------------------
// AppContext::hdgui2D
//------------------------------------------------------------------------
std::string AppContext::hdgui2D() const
{
  std::stringstream s{};
  s << "format_version = \"2.0\"\n\n";
  s << fFrontPanel->fPanel.hdgui2D();
  s << "\n";
  s << fBackPanel->fPanel.hdgui2D();
  s << "\n";
  if(fHasFoldedPanels)
  {
    s << fFoldedFrontPanel->fPanel.hdgui2D();
    s << "\n";
    s << fFoldedBackPanel->fPanel.hdgui2D();
    s << "\n";
  }
  else
  {
    s << "-- players don't have folded panels\n";
  }

  return s.str();
}

//------------------------------------------------------------------------
// Application::saveConfig
//------------------------------------------------------------------------
void AppContext::saveConfig()
{
  std::stringstream s{};

  s << "format_version = \"1.0\"\n\n";
  s << "re_edit = {}\n";

  s << getLuaConfig() << "\n";

  s << fmt::printf("re_edit[\"imgui.ini\"] = [==[\n%s\n]==]\n", ImGui::SaveIniSettingsToMemory());

  Application::saveFile(fRoot / "re-edit.lua", s.str());
}


//------------------------------------------------------------------------
// AppContext::device2D
//------------------------------------------------------------------------
std::string AppContext::device2D() const
{
  std::stringstream s{};
  s << "format_version = \"2.0\"\n\n";

  if(!fHasFoldedPanels)
  {
    s << "panel_type = \"note_player\"\n";
  }

  s << fFrontPanel->fPanel.device2D();
  s << "\n";
  s << fBackPanel->fPanel.device2D();
  s << "\n";

  if(fHasFoldedPanels)
  {
    s << fFoldedFrontPanel->fPanel.device2D();
    s << "\n";
    s << fFoldedBackPanel->fPanel.device2D();
    s << "\n";
  }
  else
    s << "-- players don't have folded panels\n";


  return s.str();
}

//------------------------------------------------------------------------
// AppContext::cmake
//------------------------------------------------------------------------
std::string AppContext::cmake() const
{
  std::set<fs::path> texturePaths{};
  fFrontPanel->fPanel.getUsedTexturePaths(texturePaths);
  fBackPanel->fPanel.getUsedTexturePaths(texturePaths);
  if(fHasFoldedPanels)
  {
    fFoldedFrontPanel->fPanel.getUsedTexturePaths(texturePaths);
    fFoldedBackPanel->fPanel.getUsedTexturePaths(texturePaths);
  }

  std::stringstream s{};
  s << "set(re_sources_2d\n";
  s << "    # lua files describing the GUI\n";
  s << "    \"${RE_2D_SRC_DIR}/device_2D.lua\"\n";
  s << "    \"${RE_2D_SRC_DIR}/hdgui_2D.lua\"\n";
  s << "    # Images for the device\n";
  for(auto &path: texturePaths)
  {
    s << fmt::printf("    \"${RE_2D_SRC_DIR}/%s\"\n", path.filename().u8string());
  }
  s << "    )";
  return s.str();
}

//------------------------------------------------------------------------
// AppContext::enableFileWatcher
//------------------------------------------------------------------------
void AppContext::enableFileWatcher()
{
  if(!fRootWatchID)
  {
    fRootListener = std::make_shared<impl::UpdateListener>(*this, fRoot);
    fRootWatchID = fRootWatcher->addWatch(fRoot.u8string(), fRootListener.get(), true);
    fRootWatcher->watch();
  }
}

//------------------------------------------------------------------------
// AppContext::disableFileWatcher
//------------------------------------------------------------------------
void AppContext::disableFileWatcher()
{
  if(fRootWatchID)
  {
    fRootWatcher->removeWatch(*fRootWatchID);
    fRootListener = nullptr;
    fRootWatchID = std::nullopt;
  }
}

//------------------------------------------------------------------------
// AppContext::importTextureBlocking
//------------------------------------------------------------------------
std::optional<FilmStrip::key_t> AppContext::importTextureBlocking()
{
  disableFileWatcher();
  auto deferred = Utils::defer([this] { enableFileWatcher(); });

  nfdchar_t *outPath;
  nfdfilteritem_t filterItem[] = { { "Image", "png" } };
  nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
  if(result == NFD_OKAY)
  {
    fs::path texturePath{outPath};
    NFD_FreePath(outPath);
    return fTextureManager->importTexture(texturePath);
  }
  else if(result == NFD_CANCEL)
  {
    return std::nullopt;
  }
  else
  {
    RE_EDIT_LOG_WARNING("Error while importing images: %s", NFD_GetError());
    return std::nullopt;
  }
}

////------------------------------------------------------------------------
//// AppContext::onNativeWindowPositionChange
////------------------------------------------------------------------------
//void AppContext::onNativeWindowPositionChange(int x, int y, float iFontScale, float iFontDpiScale)
//{
//  auto loggingManager = LoggingManager::instance();
//
//  loggingManager->debug("FontScale", "%f", iFontScale);
//  loggingManager->debug("FontDpiScale", "%f", iFontDpiScale);
//
//  fFontManager->setFontScales(iFontScale, iFontDpiScale);
//}

}