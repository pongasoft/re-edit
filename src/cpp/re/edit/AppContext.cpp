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
#include <version.h>

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
AppContext::AppContext(fs::path const &iRoot, std::shared_ptr<TextureManager> iTextureManager) :
  fRoot{fs::canonical(iRoot)},
  fTextureManager{std::move(iTextureManager)},
  fUserPreferences{std::make_shared<UserPreferences>()},
  fPropertyManager{std::make_shared<PropertyManager>()},
  fUndoManager{std::make_shared<UndoManager>()},
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
void AppContext::initPanels(fs::path const &iDevice2DFile,
                            fs::path const &iHDGui2DFile,
                            Utils::CancellableSPtr const &iCancellable)
{
  iCancellable->progress("Loading device_2D.lua...");
  auto d2d = lua::Device2D::fromFile(iDevice2DFile);
  fReEditVersion = d2d->getReEditVersion();

  iCancellable->progress("Loading hdgui_2D.lua...");
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
  iCancellable->progress("Init front panel...");
  merge(numFrames, fFrontPanel->initPanel(*this, d2d->front(), hdg->front()));

  iCancellable->progress("Init back panel...");
  merge(numFrames, fBackPanel->initPanel(*this, d2d->back(), hdg->back()));
  if(fHasFoldedPanels)
  {
    iCancellable->progress("Init folded front panel...");
    merge(numFrames, fFoldedFrontPanel->initPanel(*this, d2d->folded_front(), hdg->folded_front()));

    iCancellable->progress("Init folded back panel...");
    merge(numFrames, fFoldedBackPanel->initPanel(*this, d2d->folded_back(), hdg->folded_back()));
  }
  iCancellable->progress("Computing num frames...");
  fTextureManager->overrideNumFrames(numFrames);
  markEdited();
  iCancellable->progress("Checking for errors...");
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

namespace impl {
//------------------------------------------------------------------------
// impl::hasActiveWidget
//------------------------------------------------------------------------
inline bool hasActiveWidget() { return ImGui::GetCurrentContext()->ActiveId != 0; }
}

//------------------------------------------------------------------------
// AppContext::handleKeyboardShortcuts
//------------------------------------------------------------------------
void AppContext::handleKeyboardShortcuts()
{
  if(ImGui::IsKeyDown(ImGuiMod_Shortcut))
  {
    // handle undo/redo via keyboard
    if(ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
      if(!impl::hasActiveWidget())
      {
        if(ImGui::IsKeyDown(ImGuiMod_Shift))
          redoLastAction();
        else
          undoLastAction();
      }
    }
    // zoom -
    else if(ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false))
    {
      decrementZoom();
    }
    // zoom +
    else if(ImGui::IsKeyPressed(ImGuiKey_Equal, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false))
    {
      incrementZoom();
    }
    // zoom to fit
    else if(ImGui::IsKeyPressed(ImGuiKey_0, false))
    {
      zoomToFit();
    }
    // Save
    else if(ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
      Application::GetCurrent().maybeSaveProject();
    }
    // Quit
    else if(ImGui::IsKeyPressed(ImGuiKey_Q, false))
    {
      Application::GetCurrent().maybeExit();
    }
  }
}

//------------------------------------------------------------------------
// AppContext::render
//------------------------------------------------------------------------
void AppContext::render()
{
  RE_EDIT_INTERNAL_ASSERT(fCurrentPanelState != nullptr);

  handleKeyboardShortcuts();

  int flags = needsSaving() ?  ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None;

  if(auto l = fMainWindow.begin(flags))
  {
    renderTabs();
    renderZoomSelection();
    renderGridSelection();

    ImGui::Separator();

    ImGui::PushID("Rendering");

    ImGui::PushID("Widgets");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Widgets         ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fWidgetRendering, AppContext::EWidgetRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fWidgetRendering, AppContext::EWidgetRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("X-Ray ", &fWidgetRendering, AppContext::EWidgetRendering::kXRay);
    ImGui::PopID();

    ImGui::PushID("Border");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Widgets Border  ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fBorderRendering, AppContext::EBorderRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fBorderRendering, AppContext::EBorderRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("Hit B.", &fBorderRendering, AppContext::EBorderRendering::kHitBoundaries);
    ImGui::PopID();

    ImGui::PushID("Panel");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Panel           ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fPanelRendering, AppContext::EPanelRendering::kNone);
    ImGui::SameLine();
    ReGui::TextRadioButton("Border", &fPanelRendering, AppContext::EPanelRendering::kBorder);
    ImGui::SameLine();
    ReGui::TextRadioButton("Normal", &fPanelRendering, AppContext::EPanelRendering::kNormal);
    ImGui::SameLine();
    ReGui::TextRadioButton("X-Ray ", &fPanelRendering, AppContext::EPanelRendering::kXRay);
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

    ImGui::PushID("Fold Icon");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Fold Icon       ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fShowFoldButton, false);
    ImGui::SameLine();
    ReGui::TextRadioButton("Show  ", &fShowFoldButton, true);
    ImGui::PopID();

    ImGui::PushID("Rails");
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Rack Rails      ");
    ImGui::SameLine();
    ReGui::TextRadioButton("None  ", &fShowRackRails, false);
    ImGui::SameLine();
    ReGui::TextRadioButton("Show  ", &fShowRackRails, true);
    ImGui::PopID();

    ImGui::PopID();

    if(maybeReloadTextures())
    {
      ImGui::Separator();
      ImGui::AlignTextToFramePadding();
      ReGui::TipIcon();ImGui::SameLine();ImGui::TextUnformatted("Detected image changes");
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_RescanImages, "Rescan")))
        fReloadTexturesRequested = true;
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_Reset, "Dismiss")))
        maybeReloadTextures(false);
    }

    if(maybeReloadDevice())
    {
      ImGui::Separator();
      ImGui::AlignTextToFramePadding();
      ReGui::TipIcon();ImGui::SameLine();ImGui::TextUnformatted("Detected device changes");
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_ReloadMotherboard, "Reload")))
        fReloadDeviceRequested = true;
      ImGui::SameLine();
      if(ImGui::Button(ReGui_Prefix(ReGui_Icon_Reset, "Dismiss")))
        maybeReloadDevice(false);
    }

#ifndef NDEBUG
    ImGui::Separator();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
#endif
  }

  fCurrentPanelState->render(*this);
  fPreviousPanelState = fCurrentPanelState;

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
                             ImVec2 const &iSize,
                             int iFrameNumber,
                             ImU32 iBorderColor,
                             ImU32 iTextureColor) const
{
  iTexture->Item(iPosition, iSize, getZoom(), iFrameNumber, iBorderColor, iTextureColor);
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
  iTexture->draw(iPosition, getZoom(), iFrameNumber, iBorderColor, iTextureColor);
}

//------------------------------------------------------------------------
// AppContext::drawRect
//------------------------------------------------------------------------
void AppContext::drawRect(ImVec2 const &iPosition, ImVec2 const &iSize, ImU32 iColor) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  ImVec2 pos(cp + iPosition * getZoom());
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(pos, {pos.x + (iSize.x * getZoom()), pos.y + (iSize.y * getZoom())}, iColor);
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
  auto const cp = ImGui::GetCursorScreenPos() + iPosition * getZoom();

  const ImRect rect{cp, cp + iSize * getZoom()};

  ImGui::SetCursorScreenPos(cp);
  ImGui::ItemSize(rect);
  if(!ImGui::ItemAdd(rect, 0))
      return;

  ImGui::SetCursorScreenPos(cp);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(cp, {cp.x + (iSize.x * getZoom()), cp.y + (iSize.y * getZoom())}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// AppContext::RectItem
//------------------------------------------------------------------------
void AppContext::RectItem(ImVec2 const &iPosition,
                          ImVec2 const &iSize,
                          ImU32 iColor,
                          float iRounding,
                          ImDrawFlags iFlags) const
{
  auto const cp = ImGui::GetCursorScreenPos() + iPosition * getZoom();

  const ImRect rect{cp, cp + iSize * getZoom()};

  ImGui::SetCursorScreenPos(cp);
  ImGui::ItemSize(rect);
  if(!ImGui::ItemAdd(rect, 0))
    return;

  ImGui::SetCursorScreenPos(cp);
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRect(cp, {cp.x + (iSize.x * getZoom()), cp.y + (iSize.y * getZoom())}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// AppContext::Dummy
//------------------------------------------------------------------------
void AppContext::Dummy(ImVec2 const &iPosition, ImVec2 const &iSize) const
{
  auto const cp = ImGui::GetCursorScreenPos() + iPosition * getZoom();
  const ImRect rect{cp, cp + iSize * getZoom()};
  ImGui::SetCursorScreenPos(cp);
  ImGui::ItemSize(rect);
  ImGui::ItemAdd(rect, 0);
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
  ImVec2 pos(cp + iPosition * getZoom());
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(pos, {pos.x + (iSize.x * getZoom()), pos.y + (iSize.y * getZoom())}, iColor, iRounding, iFlags);
}

//------------------------------------------------------------------------
// AppContext::drawLine
//------------------------------------------------------------------------
void AppContext::drawLine(const ImVec2& iP1, const ImVec2& iP2, ImU32 iColor, float iThickness) const
{
  auto const cp = ImGui::GetCursorScreenPos();
  auto drawList = ImGui::GetWindowDrawList();
  drawList->AddLine(cp + iP1 * getZoom(), cp + iP2 * getZoom(), iColor, iThickness);
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
void AppContext::init(config::Device const &iConfig)
{
  fPanelWindow.setIsVisible(iConfig.fShowPanel);
  fPanelWidgetsWindow.setIsVisible(iConfig.fShowPanelWidgets);
  fPropertiesWindow.setIsVisible(iConfig.fShowProperties);
  fWidgetsWindow.setIsVisible(iConfig.fShowWidgets);
  fGrid = ImVec2{std::fmax(iConfig.fGrid.x, 1.0f), std::fmax(iConfig.fGrid.y, 1.0f)};
//  fShowBorder = static_cast<ShowBorder>(iConfig.fShowBorder);
//  fShowCustomDisplay = static_cast<ShowCustomDisplay>(iConfig.fShowCustomDisplay);
//  fShowSampleDropZone = static_cast<ShowSampleDropZone>(iConfig.fShowSampleDropZone);
  enableFileWatcher();
}

//------------------------------------------------------------------------
// AppContext::getConfig
//------------------------------------------------------------------------
config::Device AppContext::getConfig() const
{
  auto info = fPropertyManager->getDeviceInfo();

  config::Device c{
    /* .fName             = */ info.fMediumName,
    /* .fPath             = */ fRoot.u8string(),
    /* .fType             = */ deviceTypeToString(info.fDeviceType),
    /* .fShowProperties   = */ fPropertiesWindow.isVisible(),
    /* .fShowPanel        = */ fPanelWindow.isVisible(),
    /* .fShowPanelWidgets = */ fPanelWidgetsWindow.isVisible(),
    /* .fShowWidgets      = */ fWidgetsWindow.isVisible(),
    /* .fGrid             = */ fGrid,
    /* .fImGuiIni         = */ ImGui::SaveIniSettingsToMemory()
  };

  return c;
}

//------------------------------------------------------------------------
// AppContext::getDeviceName
//------------------------------------------------------------------------
std::string AppContext::getDeviceName() const
{
  return fPropertyManager->getDeviceInfo().fMediumName;
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
void AppContext::initGUI2D(Utils::CancellableSPtr const &iCancellable)
{
  auto GUI2D = fRoot / "GUI2D";
  iCancellable->progress("Loading built ins...");
  fTextureManager->init(BuiltIns::kDeviceBuiltIns, GUI2D);

  iCancellable->progress("Scanning GUI2D...");
  fTextureManager->scanDirectory();

  auto device_2D = GUI2D / "device_2D.lua";
  auto hdgui_2D = GUI2D / "hdgui_2D.lua";
  if(fs::exists(device_2D) && fs::exists(hdgui_2D))
    initPanels(device_2D, hdgui_2D, iCancellable);
  else
  {
    markEdited();
    checkForErrors();
  }
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
        static constexpr auto kKeyboardShortcut = ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "Z");
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
          if(ImGui::MenuItem(desc.c_str(), kKeyboardShortcut))
          {
            undoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Undo, "Undo"), kKeyboardShortcut);
          ImGui::EndDisabled();
        }
      }

      // Redo
      {
        static constexpr auto kKeyboardShortcut = ReGui_Menu_Shortcut3(ReGui_Icon_KeySuper, ReGui_Icon_KeyShift, "Z");
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
          if(ImGui::MenuItem(desc.c_str(), kKeyboardShortcut))
          {
            redoLastAction();
          }
        }
        else
        {
          ImGui::BeginDisabled();
          ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Redo, "Redo"), kKeyboardShortcut);
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
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_Save, "Save"), ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "S")))
      {
        Application::GetCurrent().maybeSaveProject();
      }
      if(ImGui::MenuItem("Close"))
      {
        Application::GetCurrent().maybeCloseProject();
      }
      ImGui::Separator();
      if(ImGui::MenuItem(ReGui_Prefix(ReGui_Icon_ImportImages, "Import images")))
      {
        auto numTextures = importTexturesBlocking();
        if(numTextures > 0)
          Application::GetCurrent().newDialog("Import")
            .text(fmt::printf("%ld images imported successfully", numTextures))
            .buttonOk();
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
      if(ImGui::BeginMenu("Zoom"))
      {
        if(ImGui::MenuItem("Zoom +", ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "=")))
          incrementZoom();
        if(ImGui::MenuItem("Zoom -", ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "-")))
          decrementZoom();
        if(ImGui::MenuItem("Zoom to fit", ReGui_Menu_Shortcut2(ReGui_Icon_KeySuper, "0")))
          zoomToFit();
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if(ImGui::MenuItem("Horizontal Layout"))
        fNewLayoutRequested = config::kDefaultHorizontalLayout;
      if(ImGui::MenuItem("Vertical Layout"))
        fNewLayoutRequested = config::kDefaultVerticalLayout;
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

//------------------------------------------------------------------------
// AppContext::setUserZoom
//------------------------------------------------------------------------
void AppContext::setUserZoom(float iZoom)
{
  // we make sure that the zoom factor is within reasonable range
  iZoom = Utils::clamp(iZoom, 0.1f, 5.0f);
  fUserZoom = iZoom;
  fDpiAdjustedZoom = iZoom * Application::GetCurrent().getCurrentFontDpiScale();
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
    setUserZoom(fUserZoom); // will adjust the zoom if necessary
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
  UserError errors{};
  auto GUI2D = fRoot / "GUI2D";
  importBuiltIns(&errors); // convert built ins into actual images first (so that cmake() can see them)
  Application::saveFile(GUI2D / "device_2D.lua", device2D(), &errors);
  Application::saveFile(GUI2D / "hdgui_2D.lua", hdgui2D(), &errors);
  if(fs::exists(fRoot / "CMakeLists.txt"))
    Application::saveFile(GUI2D / "gui_2D.cmake", cmake(), &errors);
  Application::GetCurrent().savePreferences(&errors);
  if(errors.hasErrors())
    Application::GetCurrent().newDialog("Error")
      .preContentMessage("There were some errors during the save operation")
      .lambda([errors] {
        for(auto const &error: errors.getErrors())
          ImGui::BulletText("%s", error.c_str());
      })
      .buttonOk();
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
  s << fmt::printf("re_edit = { version = \"%s\" }\n\n", kFullVersion);
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
// AppContext::device2D
//------------------------------------------------------------------------
std::string AppContext::device2D() const
{
  std::stringstream s{};
  s << "format_version = \"2.0\"\n\n";
  s << fmt::printf("re_edit = { version = \"%s\" }\n\n", kFullVersion);

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
  fFrontPanel->fPanel.collectUsedTexturePaths(texturePaths);
  fBackPanel->fPanel.collectUsedTexturePaths(texturePaths);
  if(fHasFoldedPanels)
  {
    fFoldedFrontPanel->fPanel.collectUsedTexturePaths(texturePaths);
    fFoldedBackPanel->fPanel.collectUsedTexturePaths(texturePaths);
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

//------------------------------------------------------------------------
// AppContext::importTexturesBlocking
//------------------------------------------------------------------------
std::size_t AppContext::importTexturesBlocking()
{
  disableFileWatcher();
  auto deferred = Utils::defer([this] { enableFileWatcher(); });

  const nfdpathset_t *outPaths;
  nfdfilteritem_t filterItem[] = { { "Image", "png" } };
  nfdresult_t result = NFD_OpenDialogMultiple(&outPaths, filterItem, 1, nullptr);
  if(result == NFD_OKAY)
  {
    nfdpathsetsize_t numPaths;
    NFD_PathSet_GetCount(outPaths, &numPaths);
    std::vector<fs::path> texturePaths{};
    texturePaths.reserve(numPaths);

    for(nfdpathsetsize_t i = 0; i < numPaths; ++i)
    {
      nfdchar_t *path;
      NFD_PathSet_GetPath(outPaths, i, &path);
      texturePaths.emplace_back(path);
      // free individual path
      NFD_PathSet_FreePath(path);
    }

    // free paths (what a shitty api...)
    NFD_PathSet_Free(outPaths);

    for(auto &texturePath: texturePaths)
      fTextureManager->importTexture(texturePath);
    return texturePaths.size();
  }
  else if(result == NFD_CANCEL)
  {
    return 0;
  }
  else
  {
    RE_EDIT_LOG_WARNING("Error while importing images: %s", NFD_GetError());
    return 0;
  }
}

//------------------------------------------------------------------------
// AppContext::importBuiltIns
//------------------------------------------------------------------------
void AppContext::importBuiltIns(UserError *oErrors)
{
  std::set<FilmStrip::key_t> keys{};
  fFrontPanel->fPanel.collectUsedTextureBuiltIns(keys);
  fBackPanel->fPanel.collectUsedTextureBuiltIns(keys);
  if(fHasFoldedPanels)
  {
    fFoldedFrontPanel->fPanel.collectUsedTextureBuiltIns(keys);
    fFoldedBackPanel->fPanel.collectUsedTextureBuiltIns(keys);
  }

  if(!keys.empty())
    fTextureManager->importBuiltIns(keys, oErrors);
}

//------------------------------------------------------------------------
// AppContext::renderZoomSelection
//------------------------------------------------------------------------
void AppContext::renderZoomSelection()
{
  ImGui::PushID("Zoom");
  auto zoomPercent = fUserZoom * 100.0f;
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Zoom");
  ImGui::SameLine();
  ImGui::PushItemWidth(fItemWidth / 2.0f);
  if(ImGui::SliderFloat("##zoomfloat", &zoomPercent, 20.0f, 200.0f, "%3.0f%%"))
  {
    setUserZoom(zoomPercent / 100.0f);
  }
  ImGui::PopItemWidth();

  ImGui::SameLine();

  int zoom = static_cast<int>(fUserZoom * 100);
  int controlZoom = zoom;
  ReGui::TextRadioButton(" 20%", &zoom, 20);
  ImGui::SameLine();
  ReGui::TextRadioButton("100%", &zoom, 100);
  ImGui::SameLine();
  ImGui::BeginDisabled(!fPanelWindow.isVisible());
  if(ImGui::Button("Fit "))
  {
    zoomToFit();
  }
  ImGui::EndDisabled();

  if(controlZoom != zoom)
  {
    setUserZoom(static_cast<float>(zoom) / 100.0f);
  }
  ImGui::PopID();

//  static bool kSlowFrameRate{};
//
//  if(kSlowFrameRate)
//  {
//    std::this_thread::sleep_for(std::chrono::milliseconds(500));
//  }
//
//  ImGui::Checkbox("Slow Frame Rate", &kSlowFrameRate);

}

//------------------------------------------------------------------------
// AppContext::renderGridSelection
//------------------------------------------------------------------------
void AppContext::renderGridSelection()
{
  static bool kSquare = fGrid.x == fGrid.y;
  constexpr auto kGridStep = 5;
  constexpr auto kGridFastStep = 50;

  ImGui::PushID("Grid");

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Grid");
  ImGui::SameLine();

  ImGui::PushItemWidth(fItemWidth / (kSquare ? 2.0f : 3.0f));

  if(kSquare)
  {
    auto size = fGrid.x;
    if(ReGui::InputInt("##grid", &size, kGridStep, kGridFastStep))
    {
      fGrid.x = std::fmax(size, 1.0f);
      fGrid.y = std::fmax(size, 1.0f);
    }
  }
  else
  {
    auto grid = fGrid;
    if(ReGui::InputInt("w", &grid.x, kGridStep, kGridFastStep))
      fGrid.x = std::fmax(grid.x, 1.0f);
    ImGui::SameLine();
    if(ReGui::InputInt("h", &grid.y, kGridStep, kGridFastStep))
      fGrid.y = std::fmax(grid.y, 1.0f);
  }

  ImGui::SameLine();

  if(ImGui::Checkbox("Square", &kSquare))
  {
    if(kSquare)
      fGrid.y = fGrid.x;
  }

  ImGui::PopItemWidth();

  ImGui::PopID();
}

//------------------------------------------------------------------------
// AppContext::zoomToFit
//------------------------------------------------------------------------
void AppContext::zoomToFit()
{
  if(auto l = fPanelWindow.begin())
  {
    // Implementation note: using internal API because the public API GetContentRegionAvail() depends on
    // scrollbars being visible or not and knowing if a scrollbar is visible is an internal api
    // see this thread https://github.com/ocornut/imgui/issues/6060#issuecomment-1375635063
    auto const &style = ImGui::GetStyle();
    auto windowSize = ImGui::GetCurrentWindowRead()->OuterRectClipped.GetSize();
    auto panelSize = fCurrentPanelState->fPanel.getSize();
    auto factor = (windowSize - (style.WindowPadding * 2)) / panelSize / Application::GetCurrent().getCurrentFontDpiScale();
    setUserZoom(std::min(factor.x, factor.y));
  }
}

//------------------------------------------------------------------------
// AppContext::incrementZoom
//------------------------------------------------------------------------
void AppContext::incrementZoom()
{
  setUserZoom(fUserZoom * 1.1f);
}

//------------------------------------------------------------------------
// AppContext::decrementZoom
//------------------------------------------------------------------------
void AppContext::decrementZoom()
{
  setUserZoom(fUserZoom * 0.9f);
}

//------------------------------------------------------------------------
// AppContext::getBuiltInTexture
//------------------------------------------------------------------------
std::shared_ptr<Texture> AppContext::getBuiltInTexture(FilmStrip::key_t const &iKey) const
{
  return Application::GetCurrent().getTexture(iKey);
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