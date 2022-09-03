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

namespace re::edit {

namespace stl {

template<typename C>
inline typename C::value_type last(C const &v)
{
  if(v.empty())
    return {};

  auto iter = v.end() - 1;
  return *iter;
}

}

//------------------------------------------------------------------------
// UndoManager::addUndoAction
//------------------------------------------------------------------------
void UndoManager::addUndoAction(std::shared_ptr<UndoAction> iAction)
{
  if(!isEnabled())
    return;

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

  auto last = stl::last(fUndoHistory);
  if(last)
    last->resetMergeKey();
  fUndoHistory.emplace_back(std::move(iAction));

  fRedoHistory.clear();
}

//------------------------------------------------------------------------
// UndoManager::undoLastAction
//------------------------------------------------------------------------
void UndoManager::undoLastAction(AppContext &iCtx)
{
  auto undoAction = popLastUndoAction();
  if(undoAction)
  {
    undoAction->resetMergeKey();
    auto redoAction = undoAction->execute(iCtx);
    if(redoAction)
    {
      redoAction->fUndoAction = undoAction;
      fRedoHistory.emplace_back(std::move(redoAction));
    }
  }
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
  return stl::last(fUndoHistory);
}

//------------------------------------------------------------------------
// UndoManager::getLastUndoAction
//------------------------------------------------------------------------
std::shared_ptr<RedoAction> UndoManager::getLastRedoAction() const
{
  return stl::last(fRedoHistory);
}

//------------------------------------------------------------------------
// UndoManager::popLastUndoAction
//------------------------------------------------------------------------
std::shared_ptr<UndoAction> UndoManager::popLastUndoAction()
{
  if(fUndoHistory.empty())
    return nullptr;

  auto iter = fUndoHistory.end() - 1;
  auto undoAction = *iter;
  fUndoHistory.erase(iter);
  return undoAction;
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

//------------------------------------------------------------------------
// WidgetUndoAction::execute
//------------------------------------------------------------------------
std::shared_ptr<RedoAction> WidgetUndoAction::execute(AppContext &iCtx)
{
  auto w = iCtx.getPanel(fPanelType)->replaceWidget(fWidgetId, fWidget);
  return std::make_shared<LambdaRedoAction>([panelType = this->fPanelType, widgetId = this->fWidgetId, w2 = std::move(w)](AppContext &iCtx) {
    iCtx.getPanel(panelType)->replaceWidget(widgetId, w2);
  });
}

}