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

#ifndef RE_EDIT_WIDGET_ATTRIBUTE_HPP
#define RE_EDIT_WIDGET_ATTRIBUTE_HPP

#include "Widget.h"
#include "Panel.h"

namespace re::edit::widget {

class AttributeUpdateAction : public ExecutableAction<void, PanelAction>
{
public:
  void init(int iWidgetId,
            std::unique_ptr<widget::Attribute> iValue,
            std::unique_ptr<widget::Attribute> iPreviousValue,
            std::string iDescription,
            MergeKey const &iMergeKey)
  {
    fWidgetId = iWidgetId;
    fValue = std::move(iValue);
    fPreviousValue = std::move(iPreviousValue);
    fDescription = std::move(iDescription);
    fMergeKey = iMergeKey;
  }

  void execute() override
  {
    // action has already taken place
  }

  void undo() override
  {
    getWidget()->copyFromAction(fPreviousValue.get());
  }

  void redo() override
  {
    getWidget()->copyFromAction(fValue.get());
  }

protected:
  Widget *getWidget() const
  {
    return getPanel()->findWidget(fWidgetId);
  }

  bool canMergeWith(Action const *iAction) const override
  {
    if(typeid(*this) != typeid(*iAction))
      return false;
    auto action = dynamic_cast<AttributeUpdateAction const *>(iAction);
    return action && action->fWidgetId == fWidgetId && action->fPreviousValue->eq(fValue.get());
  }

  std::unique_ptr<Action> doMerge(std::unique_ptr<Action> iAction) override
  {
    auto action = dynamic_cast<AttributeUpdateAction *>(iAction.get());
    fValue = std::move(action->fValue);
    if(fValue->eq(fPreviousValue.get()))
      return NoOpAction::create();
    else
      return nullptr;
  }

private:
  int fWidgetId{-1};
  std::unique_ptr<widget::Attribute> fValue{};
  std::unique_ptr<widget::Attribute> fPreviousValue{};
};

//------------------------------------------------------------------------
// Attribute::update
//------------------------------------------------------------------------
template<typename F>
bool Attribute::update(F &&f, std::string const &iDescription, MergeKey const &iMergeKey)
{
  auto &ctx = AppContext::GetCurrent();
  if(!ctx.isUndoEnabled())
  {
    f();
    markEdited();
    return true;
  }
  else
  {
    auto previousValue = clone();
    f();
    auto currentValue = clone();
    if(!currentValue->eq(previousValue.get()))
    {
      markEdited();
      ctx.executeAction<AttributeUpdateAction>(getParent()->getPanelType(),
                                               getParent()->getId(),
                                               std::move(currentValue),
                                               std::move(previousValue),
                                               iDescription,
                                               iMergeKey);
      return true;
    }
    else
      return false;
  }
}

//------------------------------------------------------------------------
// Attribute::resetAttribute
//------------------------------------------------------------------------
template<typename F>
bool Attribute::updateAttribute(F &&f, Attribute *iAttributeForDescription, MergeKey const &iMergeKey)
{
  return update(std::forward<F>(f), computeUpdateAttributeDescription(iAttributeForDescription), iMergeKey);
}

namespace attribute {

//------------------------------------------------------------------------
// Attribute::update
//------------------------------------------------------------------------
template<typename T>
bool SingleAttribute<T>::update(T const &iNewValue)
{
  return updateAttribute([this, &iNewValue] {
    fValue = iNewValue;
    fProvided = true;
  });
}

//------------------------------------------------------------------------
// Attribute::mergeUpdate
//------------------------------------------------------------------------
template<typename T>
bool SingleAttribute<T>::mergeUpdate(T const &iNewValue)
{
  return updateAttribute([this, &iNewValue]{
    fValue = iNewValue;
    fProvided = true;
  }, this, MergeKey::from(this));
}

}


}

#endif // RE_EDIT_WIDGET_ATTRIBUTE_HPP
