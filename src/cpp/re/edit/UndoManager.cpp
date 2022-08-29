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

namespace re::edit {

//------------------------------------------------------------------------
// UndoManager::addUndoAction
//------------------------------------------------------------------------
void UndoManager::addUndoAction(std::shared_ptr<UndoAction> iAction)
{
  auto compositeAction = std::dynamic_pointer_cast<CompositeUndoAction>(iAction);

  if(compositeAction)
  {
    switch(compositeAction->fActions.size())
    {
      case 0:
        return;

      case 1:
        iAction = compositeAction->fActions[0];
        break;

      default:
        // do nothing
        break;
    }
  }

  if(fUndoTransaction)
    fUndoTransaction->fActions.emplace_back(std::move(iAction));
  else
    fUndoHistory.emplace_back(std::move(iAction));

  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoLastAction
//------------------------------------------------------------------------
void UndoManager::undoLastAction(AppContext &iCtx)
{
  if(fUndoHistory.empty())
    return;

  auto iter = fUndoHistory.end() - 1;
  auto undoAction = *iter;
  auto redoAction = undoAction->execute(iCtx);
  if(redoAction)
  {
    redoAction->fUndoAction = undoAction;
    fRedoHistory.emplace_back(std::move(redoAction));
  }
  fUndoHistory.erase(iter);
}

//------------------------------------------------------------------------
// UndoManager::redoLastAction
//------------------------------------------------------------------------
void UndoManager::redoLastAction(AppContext &iCtx)
{
  if(fRedoHistory.empty())
    return;

  auto iter = fRedoHistory.end() - 1;
  auto redoAction = *iter;
  redoAction->execute(iCtx);
  fUndoHistory.emplace_back(redoAction->fUndoAction);
  fRedoHistory.erase(iter);
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
std::shared_ptr<UndoAction> UndoManager::getLastUndoAction() const
{
  if(fUndoHistory.empty())
    return nullptr;
  auto iter = fUndoHistory.end() - 1;
  return *iter;
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
std::shared_ptr<RedoAction> UndoManager::getLastRedoAction() const
{
  if(fRedoHistory.empty())
    return nullptr;
  auto iter = fRedoHistory.end() - 1;
  return *iter;
}

//------------------------------------------------------------------------
// UndoManager::beginUndoTx
//------------------------------------------------------------------------
void UndoManager::beginUndoTx(long iFrame, std::string iDescription)
{
  auto tx = std::make_unique<UndoTransaction>();
  tx->fFrame = iFrame;
  tx->fDescription = std::move(iDescription);
  if(fUndoTransaction)
    tx->fParent = std::move(fUndoTransaction);
  fUndoTransaction = std::move(tx);
}

//------------------------------------------------------------------------
// UndoManager::rollbackUndoTx
//------------------------------------------------------------------------
void UndoManager::rollbackUndoTx()
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction != nullptr);
  fUndoTransaction = std::move(fUndoTransaction->fParent);
}

//------------------------------------------------------------------------
// UndoManager::commitUndoTx
//------------------------------------------------------------------------
void UndoManager::commitUndoTx()
{
  RE_EDIT_INTERNAL_ASSERT(fUndoTransaction != nullptr);

  auto tx = std::move(fUndoTransaction);
  fUndoTransaction = std::move(tx->fParent);
  addUndoAction(std::move(tx));
}

//------------------------------------------------------------------------
// CompositeUndoAction::execute
//------------------------------------------------------------------------
std::shared_ptr<RedoAction> CompositeUndoAction::execute(AppContext &iCtx)
{
  auto redo = std::make_shared<CompositeRedoAction>();

  for(auto &action: fActions)
  {
    redo->fActions.emplace_back(action->execute(iCtx));
  }

  return redo;
}

//------------------------------------------------------------------------
// CompositeUndoAction::execute
//------------------------------------------------------------------------
void CompositeRedoAction::execute(AppContext &iCtx)
{
  // we undo in reverse order
  std::for_each(fActions.rbegin(), fActions.rend(), [&iCtx](auto &action) { action->execute(iCtx); });
}
}