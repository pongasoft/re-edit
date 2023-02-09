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


//------------------------------------------------------------------------
// class ValueAction<T>
//------------------------------------------------------------------------
template<typename Target, typename T>
class ValueAction : public ExecutableAction<void>
{
public:
  using update_function_t = std::function<T(Target *, T const &)>;

public:
  void init(update_function_t iUpdateFunction, T iValue, std::string iDescription, MergeKey const &iMergeKey)
  {
    fUpdateFunction = std::move(iUpdateFunction);
    fValue = std::move(iValue);
    this->fDescription = std::move(iDescription);
    this->fMergeKey = iMergeKey;
  }

  virtual Target *getTarget() const = 0;

  void execute() override
  {
    fPreviousValue = fUpdateFunction(getTarget(), fValue);
    this->fUndoEnabled = fPreviousValue != fValue;
  }

  void undo() override
  {
    fUpdateFunction(getTarget(), fPreviousValue);
  }

  T const &getValue() const { return fValue; }
  T const &getPreviousValue() const { return fValue; }

protected:

  virtual void updateDescriptionOnSuccessfulMerge() {}

  bool canMergeWith(Action const *iAction) const override
  {
    if(typeid(*this) != typeid(*iAction))
      return false;
    auto action = dynamic_cast<ValueAction const *>(iAction);
    return action && action->fPreviousValue == fValue;
  }

  std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) override
  {
    auto action = dynamic_cast<ValueAction const *>(iAction.get());
    fValue = action->fValue;
    this->fDescription = action->getDescription();
    if(fValue == fPreviousValue)
      return NoOpAction::create();
    else
    {
      updateDescriptionOnSuccessfulMerge();
      return nullptr;
    }
  }

protected:
  update_function_t fUpdateFunction;
  T fValue;
  T fPreviousValue{};
};

class UndoManager
{
public:
  constexpr bool isEnabled() const { return fEnabled; }
  constexpr void enable() { fEnabled = true; }
  constexpr void disable() { fEnabled = false; }
  void addUndoAction(std::unique_ptr<Action> iAction);
  void undoLastAction();
  void undoUntil(Action const *iAction);
  void undoAll();
  void redoLastAction();
  void redoUntil(Action const *iAction);
  inline bool hasUndoHistory() const { return !fUndoHistory.empty(); }
  inline bool hasRedoHistory() const { return !fRedoHistory.empty(); }
  inline bool hasHistory() const { return hasUndoHistory() || hasRedoHistory(); }
  std::unique_ptr<Action> popLastUndoAction();
  Action *getLastUndoAction() const;
  Action *getLastRedoAction() const;
  std::vector<std::unique_ptr<Action>> const &getUndoHistory() const { return fUndoHistory; }
  std::vector<std::unique_ptr<Action>> const &getRedoHistory() const { return fRedoHistory; }
  void clear();

private:
  bool fEnabled{true};
  std::vector<std::unique_ptr<Action>> fUndoHistory{};
  std::vector<std::unique_ptr<Action>> fRedoHistory{};
};

}



#endif //RE_EDIT_UNDO_MANAGER_H
