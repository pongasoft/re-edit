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

class Widget;
class RedoAction;
class Panel;

struct MergeKey {
  void *fKey{};

  constexpr void reset() { fKey = nullptr; }
  constexpr bool empty() const { return fKey == nullptr; }

  constexpr bool operator==(MergeKey const &rhs) const { return fKey == rhs.fKey; }
  constexpr bool operator!=(MergeKey const &rhs) const { return !(rhs == *this); }

  constexpr static MergeKey none() { return {}; }
  constexpr static MergeKey from(void *iKey) { return { iKey }; }
};

class Action
{
public:
  virtual ~Action() = default;
  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual std::unique_ptr<Action> merge(std::unique_ptr<Action> iAction);

  inline PanelType getPanelType() const { return fPanelType; }
  inline void setPanelType(PanelType iType) { fPanelType = iType; }

  inline MergeKey getMergeKey() const { return fMergeKey; }
  inline void setMergeKey(MergeKey iMergeKey) { fMergeKey = iMergeKey; }
  inline void resetMergeKey() { fMergeKey.reset(); }

  std::string const &getDescription() const { return fDescription; }
  void setDescription(std::string iDescription) { fDescription = std::move(iDescription); }

protected:
  Panel *getPanel() const;

  virtual bool canMergeWith(Action const *iAction) const { return false; }

  /**
   * Actually processes the merge. Should return:
   * - if no merge possible then `std::move(iAction)`
   * - if merge successful then `nullptr` (iAction is consumed)
   * - if merge leads to a no op, then `NoOpAction::create()` */
  virtual std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) { return std::move(iAction); }

public:
  PanelType fPanelType{PanelType::kUnknown};
  std::string fDescription{};
  MergeKey fMergeKey{};
};

class NoOpAction : public Action
{
public:
  void undo() override {}
  void redo() override {}
  static std::unique_ptr<NoOpAction> create() { return std::make_unique<NoOpAction>(); }
};

class CompositeAction : public Action
{
public:
  void undo() override;
  void redo() override;

  inline bool isEmpty() const { return fActions.empty(); }
  inline auto getSize() const { return fActions.size(); }

  std::vector<std::unique_ptr<Action>> const &getActions() const { return fActions; }

protected:
  std::vector<std::unique_ptr<Action>> fActions{};
};

class UndoTx : public CompositeAction
{
public:
  UndoTx(PanelType iPanelType, std::string iDescription, MergeKey const &iMergeKey);
  std::unique_ptr<Action> single();
  void addAction(std::unique_ptr<Action> iAction);
};

template<typename R>
class ExecutableAction : public Action
{
public:
  using result_t = R;

  virtual result_t execute() = 0;

  inline bool isUndoEnabled() const { return fUndoEnabled; }

  void redo() override
  {
    execute();
  }

protected:
  bool fUndoEnabled{true};
};

template<typename T>
class MergeableUndoValue
{
public:
  T fOldValue{};
  T fNewValue{};
};

class UndoAction
{
public:
  virtual ~UndoAction() = default;
  virtual std::shared_ptr<RedoAction> execute() = 0;
  constexpr void *getMergeKey() const { return fMergeKey; }
  constexpr void resetMergeKey() { fMergeKey = nullptr; }

  /**
   * @tparam UndoLambda must be something that behaves like std::function<std::shared_ptr<RedoAction>(UndoAction *)> */
  template<typename UndoLambda>
  static std::shared_ptr<UndoAction> createFromLambda(UndoLambda u);

public:
  long fFrame{};
  PanelType fPanelType{PanelType::kUnknown};
  std::string fDescription{};
  void *fMergeKey{};
};

/**
 * @tparam T type of the mergeable undo value
 * @tparam UndoLambda must be something that behaves like std::function<void(UndoAction *, T)>
 * @tparam RedoLambda must be something that behaves like std::function<void(RedoAction *, T)>
 */
template<typename T, typename UndoLambda, typename RedoLambda>
class LambdaMergeableUndoAction : public UndoAction, public MergeableUndoValue<T>
{
public:
  explicit LambdaMergeableUndoAction(T const &iOldValue, T const &iNewValue, UndoLambda u, RedoLambda r) :
    fUndoLambda{std::move(u)},
    fRedoLambda{std::move(r)}
    {
      MergeableUndoValue<T>::fOldValue = iOldValue;
      MergeableUndoValue<T>::fNewValue = iNewValue;
    }

  std::shared_ptr<RedoAction> execute() override;

private:
  UndoLambda fUndoLambda;
  RedoLambda fRedoLambda;
};

/**
 * @tparam UndoLambda must be something that behaves like std::function<std::shared_ptr<RedoAction>(UndoAction *)> */
template<typename UndoLambda>
class LambdaUndoAction : public UndoAction
{
public:
  explicit LambdaUndoAction(UndoLambda iLambda) : fLambda{std::move(iLambda)} {}

  std::shared_ptr<RedoAction> execute() override { return fLambda(this); }

public:
  UndoLambda fLambda;
};

class WidgetUndoAction : public UndoAction
{
public:
  ~WidgetUndoAction();
  std::shared_ptr<RedoAction> execute() override;

public:
  int fWidgetId{-1};
  std::unique_ptr<Widget> fWidget{};
};

template<typename T>
class MergeableWidgetUndoAction : public WidgetUndoAction, public MergeableUndoValue<T>
{
};

class CompositeUndoAction : public UndoAction
{
public:
  std::shared_ptr<RedoAction> execute() override;
  inline void add(std::shared_ptr<UndoAction> iAction) { fActions.emplace_back(std::move(iAction)); }
  std::shared_ptr<UndoAction> popLastUndoAction();

public:
  std::vector<std::shared_ptr<UndoAction>> fActions{};
};

class RedoAction
{
public:
  virtual ~RedoAction() = default;
  virtual void execute() = 0;

/**
 * @tparam RedoLambda must be something that behaves like std::function<void(RedoAction *)> */
  template<typename RedoLambda>
  static std::shared_ptr<RedoAction> createFromLambda(RedoLambda r);

public:
  std::shared_ptr<UndoAction> fUndoAction{};
};

/**
 * @tparam RedoLambda must be something that behaves like std::function<void(RedoAction *)> */
template<typename RedoLambda>
class LambdaRedoAction : public RedoAction
{
public:
  explicit LambdaRedoAction(RedoLambda iLambda) : fLambda{std::move(iLambda)} {}

  void execute() override { fLambda(this); }

public:
  RedoLambda fLambda;
};

class CompositeRedoAction : public RedoAction
{
public:
  void execute() override;

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
  void undoLastAction();
  void redoLastAction();
  inline bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  inline bool hasRedoHistory() const { return !fRedoHistory.empty(); }
  inline bool hasHistory() const { return hasUndoHistory() || hasRedoHistory(); }
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

//------------------------------------------------------------------------
// UndoAction::createFromLambda
//------------------------------------------------------------------------
template<typename UndoLambda>
std::shared_ptr<UndoAction> UndoAction::createFromLambda(UndoLambda u)
{
  return std::make_shared<LambdaUndoAction<UndoLambda>>(std::move(u));
}

//------------------------------------------------------------------------
// RedoAction::createFromLambda
//------------------------------------------------------------------------
template<typename RedoLambda>
std::shared_ptr<RedoAction> RedoAction::createFromLambda(RedoLambda r)
{
  return std::make_shared<LambdaRedoAction<RedoLambda>>(std::move(r));
}

//------------------------------------------------------------------------
// LambdaMergeableUndoAction<T, UndoLambda, RedoLambda>::execute
//------------------------------------------------------------------------
template<typename T, typename UndoLambda, typename RedoLambda>
std::shared_ptr<RedoAction> LambdaMergeableUndoAction<T, UndoLambda, RedoLambda>::execute()
{
  fUndoLambda(this, MergeableUndoValue<T>::fOldValue);
  return RedoAction::createFromLambda([newValue = MergeableUndoValue<T>::fNewValue, redoLambda = fRedoLambda](RedoAction *iAction) {
    redoLambda(iAction, newValue);
  });
}


}



#endif //RE_EDIT_UNDO_MANAGER_H
