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

#include "Widget.hpp"

namespace re::edit::widget {

class AttributeUpdateAction : public WidgetAction<void>
{
public:
  AttributeUpdateAction(std::unique_ptr<widget::Attribute> iValue,
                        std::unique_ptr<widget::Attribute> iPreviousValue,
                        void *iMergeKey,
                        std::string iDescription) :
    fValue{std::move(iValue)},
    fPreviousValue{std::move(iPreviousValue)}
  {
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
  bool canMergeWith(Action const *iAction) const override
  {
    if(typeid(*this) != typeid(*iAction))
      return false;
    auto action = dynamic_cast<AttributeUpdateAction const *>(iAction);
    return action && action->fId == fId && action->fPreviousValue->eq(fValue.get());
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
  std::unique_ptr<widget::Attribute> fValue;
  std::unique_ptr<widget::Attribute> fPreviousValue;
};

//------------------------------------------------------------------------
// Attribute::update
//------------------------------------------------------------------------
template<typename F>
bool Attribute::update(F &&f, void *iMergeKey, std::optional<std::string> const &iDescription)
{
  auto &ctx = AppContext::GetCurrent();
  if(!ctx.isUndoEnabled())
  {
    f();
    return true;
  }
  else
  {
    auto previousValue = clone();
    f();
    auto currentValue = clone();
    if(!currentValue->eq(previousValue.get()))
    {
      auto w = const_cast<Widget *>(ctx.getCurrentWidget()); // TODO hack for now
      auto desc = iDescription ? *iDescription : fmt::printf("Update %s.%s", w->getName(), currentValue->fName);
      w->executeAction<AttributeUpdateAction>(std::move(currentValue), std::move(previousValue), iMergeKey, std::move(desc));
      return true;
    }
    else
      return false;
  }
}

}

#endif // RE_EDIT_WIDGET_ATTRIBUTE_HPP
