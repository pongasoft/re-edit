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

#include "Widget.hpp"
#include "Panel.h"

namespace re::edit {

//------------------------------------------------------------------------
// WidgetAction::getWidget
//------------------------------------------------------------------------
template<typename R>
Widget *WidgetAction<R>::getWidget() const
{
  return this->getPanel()->findWidget(fId);
}

//------------------------------------------------------------------------
// class WidgetValueAction<T>
//------------------------------------------------------------------------
template<typename T>
class WidgetValueAction : public WidgetActionVoid
{
public:
  explicit WidgetValueAction(T iValue, void *iMergeKey) : fValue{std::move(iValue)}
  {
    fMergeKey = iMergeKey;
  }

protected:
  bool canMergeWith(Action const *iAction) const override
  {
    if(typeid(*this) != typeid(*iAction))
      return false;
    auto action = dynamic_cast<WidgetValueAction const *>(iAction);
    return action && action->fId == fId && action->fPreviousValue == fValue;
  }

  std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) override
  {
    auto action = dynamic_cast<WidgetValueAction *>(iAction.get());
    fValue = action->fValue;
    if(fValue == fPreviousValue)
      return NoOpAction::create();
    else
      return nullptr;
  }

protected:
  T fValue;
  T fPreviousValue{};
};

//------------------------------------------------------------------------
// class RenameWidgetAction
//------------------------------------------------------------------------
class RenameWidgetAction : public WidgetValueAction<std::string>
{
public:
  explicit RenameWidgetAction(Widget *iWidget, std::string iName, void *iMergeKey) :
    WidgetValueAction(std::move(iName), iMergeKey)
  {
    fDescription = fmt::printf("Rename widget %s -> %s", iWidget->getName(), fValue);
  }

  void execute() override
  {
    fPreviousValue = getWidget()->getName();

    fUndoEnabled = fPreviousValue != fValue;

    if(fUndoEnabled)
      getWidget()->setNameAction(fValue);
  }

  void undo() override
  {
    getWidget()->setNameAction(fPreviousValue);
  }

protected:
  std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) override
  {
    iAction = WidgetValueAction::doMerge(std::move(iAction));
    fDescription = fmt::printf("Rename widget %s -> %s", fPreviousValue, fValue);
    return std::move(iAction);
  }
};

//------------------------------------------------------------------------
// Widget::setName
//------------------------------------------------------------------------
void Widget::setName(std::string iName)
{
  executeAction<RenameWidgetAction>(this, std::move(iName), &fName);
}

//------------------------------------------------------------------------
// Widget::setNameAction
//------------------------------------------------------------------------
void Widget::setNameAction(std::string iName)
{
  fName = StringWithHash(std::move(iName));
  fEdited = true;
}

}