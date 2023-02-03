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

namespace re::edit {

//------------------------------------------------------------------------
// Panel::initSelected
//------------------------------------------------------------------------
void PanelAction::initSelected(std::vector<Widget *> const &iSelectedWidgets)
{
  if(fSelectedWidgets)
    return;
  
  fSelectedWidgets = std::set<int>{};
  for(auto const w: iSelectedWidgets)
    fSelectedWidgets->emplace(w->getId());
}

//------------------------------------------------------------------------
// Panel::undoSelection
//------------------------------------------------------------------------
void PanelAction::undoSelection(Panel *iPanel)
{
  if(fSelectedWidgets)
    iPanel->selectWidgets(*fSelectedWidgets, false);
}

//------------------------------------------------------------------------
// Panel::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
void Panel::executeAction(AppContext &iCtx, Args &&... args)
{
  executePanelAction(iCtx, std::make_unique<T>(std::forward<Args>(args)...));
}

//------------------------------------------------------------------------
// Panel::executePanelAction
//------------------------------------------------------------------------
void Panel::executePanelAction(AppContext &iCtx, std::unique_ptr<PanelAction> iAction)
{
  iAction->setPanelType(getType());
  if(fPanelTx)
  {
    fPanelTx->addAction(std::move(iAction));
  }
  else
  {
    // selected widgets are only relevant in the case of an undo!
    if(iCtx.isUndoEnabled())
      iAction->initSelected(fComputedSelectedWidgets);

    if(iAction->execute() && iCtx.isUndoEnabled())
      iCtx.addUndo(std::move(iAction));
  }

}

//------------------------------------------------------------------------
// PanelTx::PanelTx
//------------------------------------------------------------------------
PanelTx::PanelTx(std::string iDescription)
{
  fDescription = std::move(iDescription);
}

//------------------------------------------------------------------------
// PanelTx::PanelTx
//------------------------------------------------------------------------
void PanelTx::undo()
{
  CompositeAction::undo();
  undoSelection(getPanel());
}

//------------------------------------------------------------------------
// PanelTx::addAction
//------------------------------------------------------------------------
void PanelTx::addAction(std::unique_ptr<PanelAction> iAction)
{
  fActions.emplace_back(std::move(iAction));
}

//------------------------------------------------------------------------
// PanelTx::first
//------------------------------------------------------------------------
std::unique_ptr<PanelAction> PanelTx::first()
{
  if(getSize() == 1)
  {
    auto singleAction = std::move(fActions[0]);
    fActions.clear();
    return singleAction;
  }
  else
    return nullptr;
}

//------------------------------------------------------------------------
// Panel::beginTx
//------------------------------------------------------------------------
void Panel::beginTx(std::string iDescription)
{
  RE_EDIT_INTERNAL_ASSERT(fPanelTx == nullptr); // no nested transaction
  fPanelTx = std::make_unique<PanelTx>(std::move(iDescription));
  fPanelTx->setPanelType(getType());
  fPanelTx->initSelected(fComputedSelectedWidgets);
}

//------------------------------------------------------------------------
// Panel::commitTx
//------------------------------------------------------------------------
void Panel::commitTx(AppContext &iCtx)
{
  RE_EDIT_INTERNAL_ASSERT(fPanelTx != nullptr); // does not make sense to commit otherwise!

  auto action = std::move(fPanelTx);

  if(action->isEmpty())
    return;

  executePanelAction(iCtx, std::move(action));
}

//------------------------------------------------------------------------
// Panel::addWidgetAction
//------------------------------------------------------------------------
int Panel::addWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget, int order)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);

  if(iWidgetId == -1)
    iWidgetId = fWidgetCounter++;

  iWidget->init(iWidgetId);

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
// class ClearSelectionAction
//------------------------------------------------------------------------
class ClearSelectionAction : public PanelAction
{
public:
  explicit ClearSelectionAction()
  {
    fDescription = "Clear Selection";
  }

  bool execute() override
  {
    getPanel()->clearSelection();
    return true;
  }

  void undo() override
  {
    auto panel = getPanel();
    undoSelection(panel);
  }
};


//------------------------------------------------------------------------
// class AddWidgetAction
//------------------------------------------------------------------------
class AddWidgetAction : public PanelAction
{
public:
  explicit AddWidgetAction(std::unique_ptr<Widget> iWidget, char const *iUndoActionName = "Add") :
    fWidget{std::move(iWidget)}
  {
    fDescription = fmt::printf("%s %s", iUndoActionName, re::edit::toString(fWidget->getType()));
  }

  bool execute() override
  {
    fId = getPanel()->addWidgetAction(fId, fWidget->fullClone(), -1);
    return true;
  }

  void undo() override
  {
    auto panel = getPanel();
    panel->deleteWidgetAction(fId);
    undoSelection(panel);
  }

private:
  std::unique_ptr<Widget> fWidget;
  int fId{-1};
};

//------------------------------------------------------------------------
// Panel::addWidget
//------------------------------------------------------------------------
void Panel::addWidget(AppContext &iCtx, std::unique_ptr<Widget> iWidget, bool iMakeSingleSelected, char const *iUndoActionName)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  if(iMakeSingleSelected)
  {
    beginTx(fmt::printf(fmt::printf("%s %s", iUndoActionName, re::edit::toString(iWidget->getType()))));
    executeAction<ClearSelectionAction>(iCtx);
    iWidget->select();
    executeAction<AddWidgetAction>(iCtx, std::move(iWidget), iUndoActionName);
    commitTx(iCtx);
  }
  else
  {
    executeAction<AddWidgetAction>(iCtx, std::move(iWidget), iUndoActionName);
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
    beginTx(fmt::printf("Paste %s [%s]", iWidget->getName(), re::edit::toString(iWidget->getType())));
    executeAction<ClearSelectionAction>(iCtx);
    auto widget = copy(iWidget);
    widget->setPositionFromCenter(iPosition);
    widget->select();
    addWidget(iCtx, std::move(widget), false, "Paste");
    commitTx(iCtx);
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

  beginTx(fmt::printf("Paste %d widgets", iWidgets.size()));

  executeAction<ClearSelectionAction>(iCtx);

  auto res = false;

  for(auto &w: iWidgets)
  {
    if(iCtx.isWidgetAllowed(fType, w->getType()))
    {
      auto widget = copy(w.get());
      widget->setPosition(iPosition + w->getPosition() - min);
      widget->select();
      addWidget(iCtx, std::move(widget), false, "Paste");
      res = true;
    }
  }

  commitTx(iCtx);

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
class DeleteWidgetAction : public PanelAction
{
public:
  explicit DeleteWidgetAction(Widget *iWidget) : fId{iWidget->getId()}
  {
    fDescription = fmt::printf("Delete %s", iWidget->getName());
  }

  bool execute() override
  {
    fWidgetAndOrder = std::move(getPanel()->deleteWidgetAction(fId));
    return true;
  }

  void undo() override
  {
    auto panel = getPanel();
    fId = panel->addWidgetAction(fId, std::move(fWidgetAndOrder.first), fWidgetAndOrder.second);
    undoSelection(panel);
  }

private:
  int fId;
  std::pair<std::unique_ptr<Widget>, int> fWidgetAndOrder{};
};

//------------------------------------------------------------------------
// Panel::deleteWidgets
//------------------------------------------------------------------------
void Panel::deleteWidgets(AppContext &iCtx, std::vector<Widget *> const &iWidgets)
{
  if(iWidgets.empty())
    return;

  beginTx(iWidgets.size() == 1 ?
          fmt::printf("Delete %s widget", iWidgets[0]->getName()) :
          fmt::printf("Delete %d widgets", iWidgets.size()));
  for(auto const &w: iWidgets)
    executeAction<DeleteWidgetAction>(iCtx, w);
  commitTx(iCtx);
}

//------------------------------------------------------------------------
// class ReplaceWidgetAction
//------------------------------------------------------------------------
class ReplaceWidgetAction : public PanelAction
{
public:
  ReplaceWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget, std::string iDescription) :
    fId{iWidgetId},
    fWidget{std::move(iWidget)}
  {
    fDescription = std::move(iDescription);
  }

  bool execute() override
  {
    fWidget = std::move(getPanel()->replaceWidgetAction(fId, std::move(fWidget)));
    return true;
  }

  void undo() override
  {
    execute(); // same code!
  }

private:
  int fId;
  std::unique_ptr<Widget> fWidget;
};

//------------------------------------------------------------------------
// Panel::transmuteWidget
//------------------------------------------------------------------------
void Panel::transmuteWidget(AppContext &iCtx, Widget const *iWidget, WidgetDef const &iNewDef)
{
  auto newWidget = iNewDef.fFactory(iWidget->getName());
  newWidget->copyFrom(*iWidget);
  newWidget->setPosition(iWidget->getPosition());
  executeAction<ReplaceWidgetAction>(iCtx, iWidget->getId(), std::move(newWidget), fmt::printf("Change %s type", iWidget->getName()));
}

//------------------------------------------------------------------------
// Panel::replaceWidgetAction
//------------------------------------------------------------------------
std::unique_ptr<Widget> Panel::replaceWidgetAction(int iWidgetId, std::unique_ptr<Widget> iWidget)
{
  RE_EDIT_INTERNAL_ASSERT(iWidget != nullptr);
  RE_EDIT_INTERNAL_ASSERT(fWidgets.find(iWidgetId) != fWidgets.end());

  iWidget->init(iWidgetId);
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
class ChangeWidgetsOrderAction : public PanelAction
{
public:
  ChangeWidgetsOrderAction(std::string iDescription,
                           std::set<int> iSelectedWidgets,
                           Panel::WidgetOrDecal iWidgetOrDecal,
                           Panel::Direction iDirection) :
    fSelectedWidgets{std::move(iSelectedWidgets)},
    fWidgetOrDecal{iWidgetOrDecal},
    fDirection{iDirection}
  {
    fDescription = std::move(iDescription);
  }

  bool execute() override
  {
    return getPanel()->changeWidgetsOrderAction(fSelectedWidgets, fWidgetOrDecal, fDirection) > 0;
  }

  void undo() override
  {
    auto panel = getPanel();
    getPanel()->changeWidgetsOrderAction(fSelectedWidgets,
                                         fWidgetOrDecal,
                                         fDirection == Panel::Direction::kUp ? Panel::Direction::kDown : Panel::Direction::kUp);
    undoSelection(panel);
  }

private:
  std::set<int> fSelectedWidgets;
  Panel::WidgetOrDecal fWidgetOrDecal;
  Panel::Direction fDirection;
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
              fmt::printf("Move %s %s", getWidget(*selectedWidgets.begin())->getName(), iDirection == Panel::Direction::kUp ? "Up" : "Down") :
              fmt::printf("Move %ld widgets %s", selectedWidgets.size(), iDirection == Panel::Direction::kUp ? "Up" : "Down");

  executeAction<ChangeWidgetsOrderAction>(iCtx, std::move(desc), std::move(selectedWidgets), iWidgetOrDecal, iDirection);
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
class MoveWidgetsAction : public PanelAction
{
public:
  MoveWidgetsAction(std::set<int> iWidgetsIds, ImVec2 const &iMoveDelta, void *iMergeKey) :
    fWidgetsIds{std::move(iWidgetsIds)},
    fMoveDelta{iMoveDelta}
  {
    fDescription = fmt::printf("Move %d widgets", fWidgetsIds.size());
    fMergeKey = iMergeKey;
  }

public:

  bool execute() override
  {
    if(fMoveDelta.x == 0 && fMoveDelta.y == 0)
      return false;

    getPanel()->moveWidgetsAction(fWidgetsIds, fMoveDelta);
    return true;
  }

  void undo() override
  {
    auto panel = getPanel();
    panel->moveWidgetsAction(fWidgetsIds, {-fMoveDelta.x, -fMoveDelta.y});
    undoSelection(panel);
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
    if(moveWidgets(iCtx, totalDelta - fWidgetMove->fDelta))
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
bool Panel::moveWidgets(AppContext &iCtx, ImVec2 const &iDelta)
{
  if(iDelta.x != 0 || iDelta.y != 0)
  {
    executeAction<MoveWidgetsAction>(iCtx, getSelectedWidgetIds(), iDelta, &fWidgetMove);
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
// class SetWidgetPositionAction
//------------------------------------------------------------------------
class SetWidgetPositionAction : public PanelAction
{
public:
  SetWidgetPositionAction(int iWidgetId, ImVec2 const &iPosition) :
    fId{iWidgetId},
    fPosition{iPosition}
  {
  }

  bool execute() override
  {
    fPreviousPosition = getPanel()->setWidgetPositionAction(fId, fPosition);
    return fPreviousPosition != fPosition;
  }

  void undo() override
  {
    auto panel = getPanel();
    getPanel()->setWidgetPositionAction(fId, fPreviousPosition);
    undoSelection(panel);
  }

private:
  int fId;
  ImVec2 fPosition;
  ImVec2 fPreviousPosition{};
};


//------------------------------------------------------------------------
// Panel::setWidgetPositionAction
//------------------------------------------------------------------------
ImVec2 Panel::setWidgetPositionAction(int iWidgetId, ImVec2 const &iPosition)
{
  auto w = findWidget(iWidgetId);
  if(!w)
    return iPosition;
  auto previousPosition = w->getPosition();
  w->setPosition(iPosition);
  if(w->isEdited())
    fEdited = true;
  return previousPosition;
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

  beginTx(fmt::printf("Align %ld Widgets %s", fComputedSelectedWidgets.size(), alignment));

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
      executeAction<SetWidgetPositionAction>(iCtx, w->getId(), alignedPosition);
  }

  commitTx(iCtx);
}

}