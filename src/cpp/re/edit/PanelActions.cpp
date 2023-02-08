/*
 * Copyright (c) 2023 pongasoft
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

#include "Panel.h"
#include "AppContext.hpp"

namespace re::edit {

using PanelActionVoid = ExecutableAction<void>;

//------------------------------------------------------------------------
// class PanelValueAction<T>
//------------------------------------------------------------------------
template<typename T>
class PanelValueAction : public ValueAction<Panel, T>
{
public:
  Panel *getTarget() const override
  {
    return this->getPanel();
  }
};

//------------------------------------------------------------------------
// Panel::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t Panel::executeAction(Args &&... args)
{
  return AppContext::GetCurrent().executeAction<T>(getType(), std::forward<Args>(args)...);
}

//------------------------------------------------------------------------
// Panel::addWidgetAction
//------------------------------------------------------------------------
int Panel::addWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget, int order)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iWidgetId == -1)
    iWidgetId = fWidgetCounter++;

  iWidget->init(this, iWidgetId);

  auto &list = iWidget->isPanelDecal() ? fDecalsOrder : fWidgetsOrder;

  if(order >= 0 && order < list.size())
    list.insert(list.begin() + order, iWidgetId);
  else
    list.emplace_back(iWidgetId);

  iWidget->markEdited();
  fEdited = true;

  fWidgets[iWidgetId] = std::move(iWidget);

  return iWidgetId;
}

//------------------------------------------------------------------------
// class WidgetSelection
//------------------------------------------------------------------------
class WidgetSelection
{
public:
  void save(Panel *iPanel)
  {
    fSelectedWidgetIds = iPanel->getSelectedWidgetIds();
  }

  void restore(Panel *iPanel)
  {
    iPanel->selectWidgets(fSelectedWidgetIds, false);
  }

  inline bool empty() const { return fSelectedWidgetIds.empty(); }

private:
  std::set<int> fSelectedWidgetIds{};
};


//------------------------------------------------------------------------
// class ClearSelectionAction
//------------------------------------------------------------------------
class ClearSelectionAction : public PanelActionVoid
{
public:
  void init()
  {
    fDescription = "Clear Selection";
  }

  void execute() override
  {
    // implementation note: not setting fUndoEnabled on purpose ("selection" is not fully under undo/redo)
    auto panel = getPanel();
    fWidgetSelection.save(panel);
    panel->clearSelection();
  }

  void undo() override
  {
    fWidgetSelection.restore(getPanel());
  }

private:
  WidgetSelection fWidgetSelection{};
};

//------------------------------------------------------------------------
// class SelectWidgetAction
//------------------------------------------------------------------------
class SelectWidgetAction : public PanelActionVoid
{
public:
  void init(int iWidgetId)
  {
    fWidgetId = iWidgetId;
    fDescription = fmt::printf("Select Widget [#%d]", iWidgetId);
  }

  void execute() override
  {
    // implementation note: not setting fUndoEnabled on purpose ("selection" is not fully under undo/redo)
    fPreviouslySelected = !getPanel()->selectWidgetAction(fWidgetId);
  }

  void undo() override
  {
    if(fPreviouslySelected)
      getPanel()->selectWidgetAction(fWidgetId);
    else
      getPanel()->unselectWidgetAction(fWidgetId);
  }

private:
  int fWidgetId{};
  bool fPreviouslySelected{};
};

//------------------------------------------------------------------------
// class SelectWidgetsAction
//------------------------------------------------------------------------
class SelectWidgetsAction : public PanelActionVoid
{
public:
  void init(std::set<int> iWidgetIds)
  {
    fDescription = fmt::printf("Select Widgets [%d]", iWidgetIds.size());
    fWidgetIds = std::move(iWidgetIds);
  }

  void execute() override
  {
    // implementation note: not setting fUndoEnabled on purpose ("selection" is not fully under undo/redo)
    auto panel = getPanel();
    fWidgetSelection.save(panel);
    panel->selectWidgetsAction(fWidgetIds);
  }

  void undo() override
  {
    fWidgetSelection.restore(getPanel());
  }

private:
  std::set<int> fWidgetIds{};
  WidgetSelection fWidgetSelection{};
};


//------------------------------------------------------------------------
// Panel::selectWidgetAction
//------------------------------------------------------------------------
bool Panel::selectWidgetAction(int id) const
{
  auto widget = findWidget(id);
  if(widget && !widget->isSelected())
  {
    widget->select();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// Panel::selectWidgetsAction
//------------------------------------------------------------------------
bool Panel::selectWidgetsAction(std::set<int> const &iWidgetIds) const
{
  auto res = false;
  for(auto id: iWidgetIds)
    res |= selectWidgetAction(id);
  return res;
}

//------------------------------------------------------------------------
// Panel::unselectWidget
//------------------------------------------------------------------------
bool Panel::unselectWidgetAction(int id) const
{
  auto widget = findWidget(id);
  if(widget && widget->isSelected())
  {
    widget->unselect();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// Panel::unselectWidgetsAction
//------------------------------------------------------------------------
bool Panel::unselectWidgetsAction(std::set<int> const &iWidgetIds)
{
  auto res = false;
  for(auto id: iWidgetIds)
    res |= unselectWidgetAction(id);
  return res;
}

//------------------------------------------------------------------------
// class AddWidgetAction
//------------------------------------------------------------------------
class AddWidgetAction : public ExecutableAction<int>
{
public:
  void init(std::unique_ptr<Widget> iWidget, char const *iUndoActionName = "Add")
  {
    fDescription = fmt::printf("%s %s", iUndoActionName, re::edit::toString(iWidget->getType()));
    fWidget = std::move(iWidget);
  }

  int execute() override
  {
    fId = getPanel()->addWidgetAction(fId, fWidget->fullClone(), -1);
    return fId;
  }

  void undo() override
  {
    getPanel()->deleteWidgetAction(fId);
  }

private:
  std::unique_ptr<Widget> fWidget;
  int fId{-1};
};

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
int Panel::addWidget(AppContext &iCtx, std::unique_ptr<Widget> iWidget, bool iMakeSingleSelected, char const *iUndoActionName)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  if(iMakeSingleSelected)
  {
    iCtx.beginUndoTx(getType(), fmt::printf(fmt::printf("%s %s", iUndoActionName, re::edit::toString(iWidget->getType()))));
    executeAction<ClearSelectionAction>();
    auto id = executeAction<AddWidgetAction>(std::move(iWidget), iUndoActionName);
    executeAction<SelectWidgetAction>(id);
    iCtx.commitUndoTx();
    return id;
  }
  else
  {
    return executeAction<AddWidgetAction>(std::move(iWidget), iUndoActionName);
  }
}

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
void Panel::addWidget(AppContext &iCtx, WidgetDef const &iDef, ImVec2 const &iPosition)
{
  auto widget = iDef.fFactory(std::nullopt);
  widget->setPositionFromCenter(iPosition);
  addWidget(iCtx, std::move(widget), true);
}

//------------------------------------------------------------------------
// Panel::pasteWidget
//------------------------------------------------------------------------
bool Panel::pasteWidget(AppContext &iCtx, Widget const *iWidget, ImVec2 const &iPosition)
{
  if(iCtx.isWidgetAllowed(fType, iWidget->getType()))
  {
    auto widget = copy(iWidget);
    widget->setPositionFromCenter(iPosition);
    addWidget(iCtx, std::move(widget), true, "Paste");
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// Panel::pasteWidgets
//------------------------------------------------------------------------
bool Panel::pasteWidgets(AppContext &iCtx, std::vector<std::unique_ptr<Widget>> const &iWidgets, ImVec2 const &iPosition)
{
  auto count = std::count_if(iWidgets.begin(), iWidgets.end(), [&iCtx, type = fType](auto &w) { return iCtx.isWidgetAllowed(type, w->getType()); });

  if(count == 0)
    return false;


  ImVec2 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  for(auto &w: iWidgets)
  {
    if(iCtx.isWidgetAllowed(fType, w->getType()))
    {
      auto tl = w->getTopLeft();
      min.x = std::min(min.x, tl.x);
      min.y = std::min(min.y, tl.y);
    }
  }

  iCtx.beginUndoTx(getType(), fmt::printf("Paste [%d] widgets", iWidgets.size()));

  executeAction<ClearSelectionAction>();

  auto res = false;

  std::set<int> ids{};

  for(auto &w: iWidgets)
  {
    if(iCtx.isWidgetAllowed(fType, w->getType()))
    {
      auto widget = copy(w.get());
      widget->setPositionAction(iPosition + w->getPosition() - min);
      ids.emplace(addWidget(iCtx, std::move(widget), false, "Paste"));
      res = true;
    }
  }

  if(!ids.empty())
    executeAction<SelectWidgetsAction>(std::move(ids));

  iCtx.commitUndoTx();

  return res;

}

//------------------------------------------------------------------------
// Panel::deleteWidgetAction
//------------------------------------------------------------------------
std::pair<std::unique_ptr<Widget>, int> Panel::deleteWidgetAction(int id)
{
  auto widget = findWidget(id);
  if(widget)
  {
    int order{};
    if(widget->isPanelDecal())
    {
      auto iter = std::find(fDecalsOrder.begin(), fDecalsOrder.end(), id);
      RE_EDIT_INTERNAL_ASSERT(iter != fDecalsOrder.end());
      order = static_cast<int>(iter - fDecalsOrder.begin());
      fDecalsOrder.erase(iter);
    }
    else
    {
      auto iter = std::find(fWidgetsOrder.begin(), fWidgetsOrder.end(), id);
      RE_EDIT_INTERNAL_ASSERT(iter != fWidgetsOrder.end());
      order = static_cast<int>(iter - fWidgetsOrder.begin());
      fWidgetsOrder.erase(iter);
    }

    // make sure that we don't have a dangling pointer
    fComputedSelectedWidgets.clear();

    std::unique_ptr<Widget> deleted{};
    std::swap(fWidgets[id], deleted);
    fWidgets.erase(id);
    fEdited = true;
    return {std::move(deleted), order};
  }
  return {nullptr, 0};
}


//------------------------------------------------------------------------
// class DeleteWidgetAction
//------------------------------------------------------------------------
class DeleteWidgetAction : public PanelActionVoid
{
public:
  void init(Widget *iWidget)
  {
    fDescription = fmt::printf("Delete %s", iWidget->getName());
    fId = iWidget->getId();
  }

  void execute() override
  {
    fWidgetAndOrder = std::move(getPanel()->deleteWidgetAction(fId));
    fUndoEnabled = fWidgetAndOrder.first != nullptr;
  }

  void undo() override
  {
    fId = getPanel()->addWidgetAction(fId, std::move(fWidgetAndOrder.first), fWidgetAndOrder.second);
  }

private:
  int fId{};
  std::pair<std::unique_ptr<Widget>, int> fWidgetAndOrder{};
};

//------------------------------------------------------------------------
// Panel::deleteWidgets
//------------------------------------------------------------------------
void Panel::deleteWidgets(AppContext &iCtx, std::vector<Widget *> const &iWidgets)
{
  if(iWidgets.empty())
    return;

  iCtx.beginUndoTx(getType(),
                   iWidgets.size() == 1 ?
                   fmt::printf("Delete %s widget", iWidgets[0]->getName()) :
                   fmt::printf("Delete %d widgets", iWidgets.size()));
  for(auto const &w: iWidgets)
    executeAction<DeleteWidgetAction>(w);
  iCtx.commitUndoTx();
}

//------------------------------------------------------------------------
// class ReplaceWidgetAction
//------------------------------------------------------------------------
class ReplaceWidgetAction : public PanelActionVoid
{
public:
  void init(int iWidgetId, std::unique_ptr<Widget> iWidget, std::string iDescription)
  {
    fDescription = std::move(iDescription);
    fId = iWidgetId;
    fWidget = std::move(iWidget);
  }

  void execute() override
  {
    fWidget = std::move(getPanel()->replaceWidgetAction(fId, std::move(fWidget)));
  }

  void undo() override
  {
    execute(); // same code!
  }

private:
  int fId{};
  std::unique_ptr<Widget> fWidget{};
};

//------------------------------------------------------------------------
// Panel::transmuteWidget
//------------------------------------------------------------------------
void Panel::transmuteWidget(AppContext &iCtx, Widget const *iWidget, WidgetDef const &iNewDef)
{
  auto newWidget = iNewDef.fFactory(iWidget->getName());
  newWidget->copyFromAction(*iWidget);
  newWidget->setPositionAction(iWidget->getPosition());
  executeAction<ReplaceWidgetAction>(iWidget->getId(), std::move(newWidget), fmt::printf("Change %s type", iWidget->getName()));
}

//------------------------------------------------------------------------
// Panel::replaceWidgetAction
//------------------------------------------------------------------------
std::unique_ptr<Widget> Panel::replaceWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  RE_EDIT_INTERNAL_ASSERT(fWidgets.find(iWidgetId) != fWidgets.end());

  iWidget->init(this, iWidgetId);
  iWidget->fSelected = fWidgets[iWidgetId]->isSelected();
  iWidget->markEdited();
  fEdited = true;
  if(iWidget->isPanelDecal() != fWidgets[iWidgetId]->isPanelDecal())
  {
    if(iWidget->isPanelDecal())
    {
      auto iter = std::find(fWidgetsOrder.begin(), fWidgetsOrder.end(), iWidgetId);
      fWidgetsOrder.erase(iter);
      fDecalsOrder.emplace_back(iWidgetId);
    }
    else
    {
      auto iter = std::find(fDecalsOrder.begin(), fDecalsOrder.end(), iWidgetId);
      fDecalsOrder.erase(iter);
      fWidgetsOrder.emplace_back(iWidgetId);
    }
  }

  // make sure that we don't have a dangling pointer
  fComputedSelectedWidgets.clear();

  std::swap(fWidgets[iWidgetId], iWidget);
  return iWidget;
}

//------------------------------------------------------------------------
// class ChangeWidgetsOrderAction
//------------------------------------------------------------------------
class ChangeWidgetsOrderAction : public PanelActionVoid
{
public:
  void init(std::string iDescription,
            std::set<int> iSelectedWidgets,
            Panel::WidgetOrDecal iWidgetOrDecal,
            Panel::Direction iDirection)
  {
    fDescription = std::move(iDescription);
    fSelectedWidgets = std::move(iSelectedWidgets);
    fWidgetOrDecal = iWidgetOrDecal;
    fDirection = iDirection;
  }

  void execute() override
  {
    fUndoEnabled = getPanel()->changeWidgetsOrderAction(fSelectedWidgets, fWidgetOrDecal, fDirection) > 0;
  }

  void undo() override
  {
    getPanel()->changeWidgetsOrderAction(fSelectedWidgets,
                                         fWidgetOrDecal,
                                         fDirection == Panel::Direction::kUp ? Panel::Direction::kDown : Panel::Direction::kUp);
  }

private:
  std::set<int> fSelectedWidgets{};
  Panel::WidgetOrDecal fWidgetOrDecal{};
  Panel::Direction fDirection{};
};

//------------------------------------------------------------------------
// Panel::changeSelectedWidgetsOrder
//------------------------------------------------------------------------
void Panel::changeSelectedWidgetsOrder(AppContext &iCtx, WidgetOrDecal iWidgetOrDecal, Direction iDirection)
{
  auto &list = iWidgetOrDecal == WidgetOrDecal::kWidget ? fWidgetsOrder : fDecalsOrder;
  std::set<int> selectedWidgets{};

  for(auto const id: list)
  {
    auto widget = findWidget(id);
    if(widget->isSelected())
      selectedWidgets.emplace(id);
  }

  if(selectedWidgets.empty())
    return;

  auto desc = selectedWidgets.size() == 1 ?
              fmt::printf("Move [%s] %s", getWidget(*selectedWidgets.begin())->getName(), iDirection == Panel::Direction::kUp ? "Up" : "Down") :
              fmt::printf("Move [%ld] widgets %s", selectedWidgets.size(), iDirection == Panel::Direction::kUp ? "Up" : "Down");

  executeAction<ChangeWidgetsOrderAction>(std::move(desc), std::move(selectedWidgets), iWidgetOrDecal, iDirection);
}

//------------------------------------------------------------------------
// Panel::changeWidgetsOrderAction
//------------------------------------------------------------------------
int Panel::changeWidgetsOrderAction(std::set<int> const &iWidgetIds, WidgetOrDecal iWidgetOrDecal, Direction iDirection)
{
  auto &list = iWidgetOrDecal == WidgetOrDecal::kWidget ? fWidgetsOrder : fDecalsOrder;

  std::vector<Widget *> selectedWidgets{};
  selectedWidgets.reserve(iWidgetIds.size());

  for(auto const id: list)
  {
    if(iWidgetIds.find(id) != iWidgetIds.end())
    {
      auto widget = findWidget(id);
      if(widget && widget->isSelected())
        selectedWidgets.emplace_back(widget);
    }
  }

  int changesCount = 0;

  if(iDirection == Direction::kDown)
  {
    // we need to iterate backward
    for(auto i = selectedWidgets.rbegin(); i != selectedWidgets.rend(); i++)
    {
      auto widget = *i;

      auto iter = std::find(list.begin(), list.end(), widget->getId());

      // already at the bottom
      if(iter + 1 == list.end())
        break; // abort loop

      std::swap(*(iter + 1), *iter);
      changesCount++;
    }
  }
  else
  {
    for(auto &w: selectedWidgets)
    {
      auto iter = std::find(list.begin(), list.end(), w->getId());

      // already at the top
      if(iter == list.begin())
        break; // abort loop

      std::swap(*(iter - 1), *iter);
      changesCount++;
    }
  }

  return changesCount;
}

//------------------------------------------------------------------------
// class MoveWidgetsAction
//------------------------------------------------------------------------
class MoveWidgetsAction : public PanelActionVoid
{
public:
  void init(std::set<int> iWidgetsIds, ImVec2 const &iMoveDelta, std::string iDescription, MergeKey iMergeKey)
  {
    fDescription = std::move(iDescription);
    fWidgetsIds = std::move(iWidgetsIds);
    fMoveDelta = iMoveDelta;
    fMergeKey = iMergeKey;
  }

public:

  void execute() override
  {
    fUndoEnabled = fMoveDelta.x != 0 || fMoveDelta.y != 0;
    if(fUndoEnabled)
    {
      auto panel = getPanel();
      fWidgetSelection.save(panel);
      panel->moveWidgetsAction(fWidgetsIds, fMoveDelta);
    }
  }

  void undo() override
  {
    auto panel = getPanel();
    panel->moveWidgetsAction(fWidgetsIds, {-fMoveDelta.x, -fMoveDelta.y});
    fWidgetSelection.restore(panel);
  }

protected:
  bool canMergeWith(Action const *iAction) const override
  {
    auto action = dynamic_cast<MoveWidgetsAction const *>(iAction);
    return action && fWidgetsIds == action->fWidgetsIds;
  }

  std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) override
  {
    auto action = dynamic_cast<MoveWidgetsAction const *>(iAction.get());
    fMoveDelta += action->fMoveDelta;
    if(fMoveDelta.x == 0 && fMoveDelta.y == 0)
      return NoOpAction::create();
    else
      return nullptr;
  }

private:
  std::set<int> fWidgetsIds;
  ImVec2 fMoveDelta;
  WidgetSelection fWidgetSelection{};
};

namespace impl {

//------------------------------------------------------------------------
// impl::clampToGrid
//------------------------------------------------------------------------
constexpr float clampToGrid(float v, float g)
{
  RE_EDIT_INTERNAL_ASSERT(g > 0);

  if(v == 0)
    return 0;

  if(v < 0)
    return -clampToGrid(-v, g);

  // This is clearly not the best implementation but can't figure out using
  // fmod (which would not be constexpr)
  float res = 0;
  while(v >= g)
  {
    res += g;
    v -= g;
  }
  return res;
}

//------------------------------------------------------------------------
// impl::clampToGrid
//------------------------------------------------------------------------
constexpr ImVec2 clampToGrid(ImVec2 v, ImVec2 g)
{
  return { clampToGrid(v.x, g.x), clampToGrid(v.y, g.y) };
}

}

//------------------------------------------------------------------------
// Panel::moveWidgets
//------------------------------------------------------------------------
void Panel::moveWidgets(AppContext &iCtx, ImVec2 const &iPosition, ImVec2 const &iGrid)
{
  if(fWidgetMove)
  {
    auto totalDelta = impl::clampToGrid(iPosition - fWidgetMove->fInitialPosition, iGrid);
    if(moveWidgets(totalDelta - fWidgetMove->fDelta))
      fWidgetMove->fDelta = totalDelta;
  }
}

//------------------------------------------------------------------------
// Panel::endMoveWidgets
//------------------------------------------------------------------------
void Panel::endMoveWidgets(AppContext &iCtx)
{
  iCtx.resetUndoMergeKey();
  fWidgetMove = std::nullopt;
}

//------------------------------------------------------------------------
// Panel::moveWidgets
//------------------------------------------------------------------------
bool Panel::moveWidgets(ImVec2 const &iDelta)
{
  if(iDelta.x != 0 || iDelta.y != 0)
  {
    auto selectedWidgets = getSelectedWidgetIds();

    auto desc = selectedWidgets.size() == 1 ?
                fmt::printf("Move [%s]", getWidget(*selectedWidgets.begin())->getName()) :
                fmt::printf("Move [%ld] widgets", selectedWidgets.size());

    executeAction<MoveWidgetsAction>(std::move(selectedWidgets), iDelta, std::move(desc), MergeKey::from(&fWidgetMove));
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
// Panel::moveWidgetsAction
//------------------------------------------------------------------------
void Panel::moveWidgetsAction(std::set<int> const &iWidgetsIds, ImVec2 const &iMoveDelta)
{
  for(auto id: iWidgetsIds)
  {
    auto w = findWidget(id);
    if(w)
    {
      w->moveAction(iMoveDelta);
      if(w->isEdited())
        fEdited = true;
    }
  }
}

//------------------------------------------------------------------------
// Panel::setCableOriginPositionAction
//------------------------------------------------------------------------
ImVec2 Panel::setCableOriginPositionAction(ImVec2 const &iPosition)
{
  RE_EDIT_INTERNAL_ASSERT(fCableOrigin != std::nullopt);

  auto res = *fCableOrigin;
  fCableOrigin = iPosition;
  if(res != iPosition)
    fEdited = true;
  return res;
}

//------------------------------------------------------------------------
// Panel::alignWidgets
//------------------------------------------------------------------------
void Panel::alignWidgets(AppContext &iCtx, Panel::WidgetAlignment iAlignment)
{
  // can only align multiple widgets!
  if(fComputedSelectedWidgets.size() < 2)
    return;

  char const *alignment = "";

  switch(iAlignment)
  {
    case WidgetAlignment::kTop:
      alignment = "Top";
      break;
    case WidgetAlignment::kBottom:
      alignment = "Bottom";
      break;
    case WidgetAlignment::kLeft:
      alignment = "Left";
      break;
    case WidgetAlignment::kRight:
      alignment = "Right";
      break;
  }

  auto const min = fComputedSelectedRect->Min;
  auto const max = fComputedSelectedRect->Max;

  iCtx.beginUndoTx(getType(), fmt::printf("Align [%ld] Widgets %s", fComputedSelectedWidgets.size(), alignment));

  for(auto w: fComputedSelectedWidgets)
  {
    auto position = w->getPosition();
    ImVec2 alignedPosition{};
    switch(iAlignment)
    {
      case WidgetAlignment::kTop:
        alignedPosition = {position.x, min.y};
        break;
      case WidgetAlignment::kBottom:
        alignedPosition = {position.x, max.y - w->getSize().y};
        break;
      case WidgetAlignment::kLeft:
        alignedPosition = {min.x, position.y};
        break;
      case WidgetAlignment::kRight:
        alignedPosition = {max.x - w->getSize().x, position.y};
        break;
    }

    if(alignedPosition != position)
    {
      iCtx.setNextUndoActionDescription(fmt::printf("Align [%s] to %s", w->getName(), alignment));
      w->setPosition(alignedPosition);
      if(w->isEdited())
        fEdited = true;
    }
  }

  iCtx.commitUndoTx();
}

//------------------------------------------------------------------------
// Panel::setCableOrigin
//------------------------------------------------------------------------
void Panel::setCableOrigin(ImVec2 const &iPosition)
{
  RE_EDIT_INTERNAL_ASSERT(fCableOrigin != std::nullopt);

  executeAction<PanelValueAction<ImVec2>>([](Panel *iPanel, auto iValue) {
                                            return iPanel->setCableOriginPositionAction(iValue);
                                          },
                                          iPosition,
                                          "Update cable_origin",
                                          MergeKey::from(&fCableOrigin));
}

//------------------------------------------------------------------------
// Panel::setPanelOptionsAction
//------------------------------------------------------------------------
bool Panel::setPanelOptionsAction(bool iDisableSampleDropOnPanel)
{
  RE_EDIT_INTERNAL_ASSERT(fDisableSampleDropOnPanel != std::nullopt);

  auto res = *fDisableSampleDropOnPanel;
  fDisableSampleDropOnPanel = iDisableSampleDropOnPanel;
  if(res != iDisableSampleDropOnPanel)
    fEdited = true;
  return res;
}

//------------------------------------------------------------------------
// Panel::setPanelOptions
//------------------------------------------------------------------------
void Panel::setPanelOptions(bool iDisableSampleDropOnPanel)
{
  RE_EDIT_INTERNAL_ASSERT(fDisableSampleDropOnPanel != std::nullopt);

  executeAction<PanelValueAction<bool>>([](Panel *iPanel, auto iValue) {
                                          return iPanel->setPanelOptionsAction(iValue);
                                        },
                                        iDisableSampleDropOnPanel,
                                        "Update disable_sample_drop_on_panel",
                                        MergeKey::from(&fDisableSampleDropOnPanel));
}

//------------------------------------------------------------------------
// Panel::setBackgroundKeyAction
//------------------------------------------------------------------------
Texture::key_t Panel::setBackgroundKeyAction(Texture::key_t const &iTextureKey)
{
  auto res = fGraphics.getTextureKey();
  fGraphics.setTextureKey(iTextureKey);
  fEdited = true;
  return res;
}

//------------------------------------------------------------------------
// Panel::setBackgroundKey
//------------------------------------------------------------------------
void Panel::setBackgroundKey(Texture::key_t const &iTextureKey)
{
  executeAction<PanelValueAction<Texture::key_t>>([](Panel *iPanel, auto iValue) {
                                                    return iPanel->setBackgroundKeyAction(iValue);
                                                  },
                                                  iTextureKey,
                                                  "Change background graphics",
                                                  MergeKey::from(&fGraphics.fTextureKey));
}


}