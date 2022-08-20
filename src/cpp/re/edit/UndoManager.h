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

namespace re::edit {

class AppContext;

class RedoAction;

class UndoAction
{
public:
  long fFrame{};
  std::string fDescription{};

public:
  friend class UndoManager;

protected:
  virtual std::shared_ptr<RedoAction> execute(AppContext &iCtx) = 0;
};

class LambdaUndoAction : public UndoAction
{
public:
  std::function<std::shared_ptr<RedoAction>(AppContext &)> fLambda{};

protected:
  std::shared_ptr<RedoAction> execute(AppContext &iCtx) override { return fLambda(iCtx); }
};

class WidgetUndoAction : public LambdaUndoAction
{
public:
  int fWidgetId{-1};
  int fAttributeId{-1};

public:
  friend class UndoManager;
};

class RedoAction
{
public:
  std::shared_ptr<UndoAction> fUndoAction{};

public:
  friend class UndoManager;

protected:
  virtual void execute(AppContext &iCtx) = 0;
};

class LambdaRedoAction : public RedoAction
{
  using lambda_t = std::function<void(AppContext &)>;

public:
  LambdaRedoAction() = default;
  explicit LambdaRedoAction(lambda_t iLambda) : fLambda{std::move(iLambda)} {}

public:
  lambda_t fLambda{};

protected:
  void execute(AppContext &iCtx) override { fLambda(iCtx); }
};

class UndoManager
{
public:
  void addUndoAction(std::shared_ptr<UndoAction> iAction);
  void undoLastAction(AppContext &iCtx);
  void redoLastAction(AppContext &iCtx);
  bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  std::shared_ptr<UndoAction> getLastUndoAction() const;
  std::shared_ptr<RedoAction> getLastRedoAction() const;
  std::vector<std::shared_ptr<UndoAction>> const &getUndoHistory() const { return fUndoHistory; }
  std::vector<std::shared_ptr<RedoAction>> const &getRedoHistory() const { return fRedoHistory; }

private:
  std::vector<std::shared_ptr<UndoAction>> fUndoHistory{};
  std::vector<std::shared_ptr<RedoAction>> fRedoHistory{};
};

}



#endif //RE_EDIT_UNDO_MANAGER_H
