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

#ifndef RE_EDIT_UNDO_MANAGER_H
#define RE_EDIT_UNDO_MANAGER_H

#include <string>
#include <functional>
#include <vector>
#include "Constants.h"

namespace re::edit {

class AppContext;
class Widget;
class RedoAction;

template<typename T>
class MergeableUndoValue
{
public:
  T fOldValue{};
//  T fNewValue{}; not needed for now
};

class UndoAction
{
public:
  virtual std::shared_ptr<RedoAction> execute(AppContext &iCtx) = 0;
  constexpr void *getMergeKey() const { return fMergeKey; }
  constexpr void resetMergeKey() { fMergeKey = nullptr; }

public:
  long fFrame{};
  PanelType fPanelType{PanelType::kUnknown};
  std::string fDescription{};
  void *fMergeKey{};
};

class LambdaUndoAction : public UndoAction
{
public:
  std::shared_ptr<RedoAction> execute(AppContext &iCtx) override { return fLambda(iCtx); }

public:
  std::function<std::shared_ptr<RedoAction>(AppContext &)> fLambda{};
};

class WidgetUndoAction : public UndoAction
{
public:
  std::shared_ptr<RedoAction> execute(AppContext &iCtx) override;

public:
  int fWidgetId{-1};
  std::shared_ptr<Widget> fWidget{};
};

template<typename T>
class MergeableWidgetUndoAction : public WidgetUndoAction, public MergeableUndoValue<T>
{
};

class CompositeUndoAction : public UndoAction
{
public:
  std::shared_ptr<RedoAction> execute(AppContext &iCtx) override;
  inline void add(std::shared_ptr<UndoAction> iAction) { fActions.emplace_back(std::move(iAction)); }

public:
  std::vector<std::shared_ptr<UndoAction>> fActions{};
};

class RedoAction
{
public:
  virtual void execute(AppContext &iCtx) = 0;

public:
  std::shared_ptr<UndoAction> fUndoAction{};
};

class LambdaRedoAction : public RedoAction
{
  using lambda_t = std::function<void(AppContext &)>;

public:
  LambdaRedoAction() = default;
  explicit LambdaRedoAction(lambda_t iLambda) : fLambda{std::move(iLambda)} {}

  void execute(AppContext &iCtx) override { fLambda(iCtx); }

public:
  lambda_t fLambda{};
};

class CompositeRedoAction : public RedoAction
{
public:
  void execute(AppContext &iCtx) override;

public:
  std::vector<std::shared_ptr<RedoAction>> fActions{};
};

class UndoManager
{
public:
  constexpr bool isEnabled() const { return fEnabled; }
  constexpr void enable() { fEnabled = true; }
  constexpr void disable() { fEnabled = false; }
  void addUndoAction(std::shared_ptr<UndoAction> iAction);
  void undoLastAction(AppContext &iCtx);
  void redoLastAction(AppContext &iCtx);
  bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  std::shared_ptr<UndoAction> popLastUndoAction();
  std::shared_ptr<UndoAction> getLastUndoAction() const;
  std::shared_ptr<RedoAction> getLastRedoAction() const;
  std::vector<std::shared_ptr<UndoAction>> const &getUndoHistory() const { return fUndoHistory; }
  std::vector<std::shared_ptr<RedoAction>> const &getRedoHistory() const { return fRedoHistory; }
  void clear();

private:
  bool fEnabled{true};
  std::vector<std::shared_ptr<UndoAction>> fUndoHistory{};
  std::vector<std::shared_ptr<RedoAction>> fRedoHistory{};
};

}



#endif //RE_EDIT_UNDO_MANAGER_H
