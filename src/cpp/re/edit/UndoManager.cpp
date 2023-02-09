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

#include "UndoManager.h"
#include "Errors.h"
#include "AppContext.h"
#include "Panel.h"
#include "stl.h"

namespace re::edit {

namespace stl {

inline Action *last(std::vector<std::unique_ptr<Action>> const &v)
{
  if(v.empty())
    return {};

  auto iter = v.end() - 1;
  return iter->get();
}

}

//------------------------------------------------------------------------
// UndoManager::addUndoAction
//------------------------------------------------------------------------
void UndoManager::addUndoAction(std::unique_ptr<Action> iAction)
{
  if(!isEnabled())
    return;

  auto last = stl::last(fUndoHistory);
  if(last)
    last->resetMergeKey();
  fUndoHistory.emplace_back(std::move(iAction));

  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoLastAction
//------------------------------------------------------------------------
void UndoManager::undoLastAction()
{
  auto action = popLastUndoAction();
  if(action)
  {
    action->resetMergeKey();
    action->undo();
    fRedoHistory.emplace_back(std::move(action));
  }
}

//------------------------------------------------------------------------
// UndoManager::redoLastAction
//------------------------------------------------------------------------
void UndoManager::redoLastAction()
{
  auto action = stl::popLastOrDefault(fRedoHistory);
  if(action)
  {
    action->redo();
    fUndoHistory.emplace_back(std::move(action));
  }
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
Action *UndoManager::getLastUndoAction() const
{
  return stl::last(fUndoHistory);
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
Action *UndoManager::getLastRedoAction() const
{
  return stl::last(fRedoHistory);
}

//------------------------------------------------------------------------
// UndoManager::popLastUndoAction
//------------------------------------------------------------------------
std::unique_ptr<Action> UndoManager::popLastUndoAction()
{
  if(!isEnabled())
    return nullptr;

  return stl::popLastOrDefault(fUndoHistory);
}

//------------------------------------------------------------------------
// UndoManager::clear
//------------------------------------------------------------------------
void UndoManager::clear()
{
  fUndoHistory.clear();
  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoUntil
//------------------------------------------------------------------------
void UndoManager::undoUntil(Action const *iAction)
{
  while(!fUndoHistory.empty() && getLastUndoAction() != iAction)
    undoLastAction();
}

//------------------------------------------------------------------------
// UndoManager::undoAll
//------------------------------------------------------------------------
void UndoManager::undoAll()
{
  while(!fUndoHistory.empty())
    undoLastAction();
}

//------------------------------------------------------------------------
// UndoManager::redoUntil
//------------------------------------------------------------------------
void UndoManager::redoUntil(Action const *iAction)
{
  while(!fRedoHistory.empty() && getLastRedoAction() != iAction)
    redoLastAction();
  redoLastAction(); // we need to get one more
}

//------------------------------------------------------------------------
// Action::getPanel
//------------------------------------------------------------------------
Panel *Action::getPanel() const
{
  return AppContext::GetCurrent().getPanel(fPanelType);
}

//------------------------------------------------------------------------
// Action::merge
//------------------------------------------------------------------------
std::unique_ptr<Action> Action::merge(std::unique_ptr<Action> iAction)
{
  if(fMergeKey.empty() ||
     iAction->getMergeKey().empty() ||
     fMergeKey != iAction->getMergeKey() ||
     fPanelType != iAction->getPanelType() ||
     !canMergeWith(iAction.get()))
  {
    return std::move(iAction);
  }

  return doMerge(std::move(iAction));
}

//------------------------------------------------------------------------
// CompositeAction::undo
//------------------------------------------------------------------------
void CompositeAction::undo()
{
  // reverse order!
  for(auto i = fActions.rbegin(); i != fActions.rend(); i++)
  {
    (*i)->undo();
  }
}

//------------------------------------------------------------------------
// CompositeAction::redo
//------------------------------------------------------------------------
void CompositeAction::redo()
{
  for(auto &action: fActions)
    action->redo();
}

//------------------------------------------------------------------------
// UndoTx::UndoTx
//------------------------------------------------------------------------
UndoTx::UndoTx(PanelType iPanelType, std::string iDescription, MergeKey const &iMergeKey)
{
  fPanelType = iPanelType;
  fDescription = std::move(iDescription);
  fMergeKey = iMergeKey;
}

//------------------------------------------------------------------------
// UndoTx::single
//------------------------------------------------------------------------
std::unique_ptr<Action> UndoTx::single()
{
  if(fActions.size() != 1)
    return nullptr;

  auto res = std::move(fActions[0]);
  fActions.clear();
  return res;
}

//------------------------------------------------------------------------
// UndoTx::addAction
//------------------------------------------------------------------------
void UndoTx::addAction(std::unique_ptr<Action> iAction)
{
  RE_EDIT_INTERNAL_ASSERT(fPanelType == iAction->getPanelType()); // no support for tx across panels
  fActions.emplace_back(std::move(iAction));
}

}