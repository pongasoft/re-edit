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

namespace re::edit {

//------------------------------------------------------------------------
// UndoManager::addUndoAction
//------------------------------------------------------------------------
void UndoManager::addUndoAction(std::shared_ptr<UndoAction> iAction)
{
  fUndoHistory.emplace_back(std::move(iAction));
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

}