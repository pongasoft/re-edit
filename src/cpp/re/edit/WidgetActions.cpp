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

#include "Panel.h"
#include "AppContext.hpp"

namespace re::edit {

//------------------------------------------------------------------------
// Widget::executeAction
//------------------------------------------------------------------------
template<class T, class... Args>
typename T::result_t Widget::executeAction(Args &&... args)
{
  auto &ctx = AppContext::GetCurrent();
  return ctx.executeAction<T>(fPanelType, fId, std::forward<Args>(args)...);
}

//------------------------------------------------------------------------
// class WidgetValueAction<T>
//------------------------------------------------------------------------
template<typename T>
class WidgetValueAction : public ValueAction<Widget, T, PanelAction>
{
public:
  Widget *getTarget() const override
  {
    return this->getPanel()->findWidget(fId);
  }

  void init(int iWidgetId,
            typename ValueAction<Widget, T>::update_function_t iUpdateFunction,
            T iValue,
            std::string iDescription,
            MergeKey const &iMergeKey)
  {
    fId = iWidgetId;
    ValueAction<Widget, T, PanelAction>::init(std::move(iUpdateFunction), std::move(iValue), std::move(iDescription), iMergeKey);
  }

protected:
  bool canMergeWith(Action const *iAction) const override
  {
    if(ValueAction<Widget, T, PanelAction>::canMergeWith(iAction))
    {
      auto action = dynamic_cast<WidgetValueAction const *>(iAction);
      return action->fId == fId;
    }
    return false;
  }

protected:
  int fId{-1};
};

//------------------------------------------------------------------------
// class RenameWidgetAction
//------------------------------------------------------------------------
class RenameWidgetAction : public WidgetValueAction<std::string>
{
protected:
  void updateDescriptionOnSuccessfulMerge() override
  {
    fDescription = fmt::printf("Rename widget %s -> %s", fPreviousValue, fValue);
  }
};

//------------------------------------------------------------------------
// Widget::setName
//------------------------------------------------------------------------
void Widget::setName(const std::string& iName)
{
  executeAction<RenameWidgetAction>([iName](Widget *w, auto value) {
                                      return w->setNameAction(value);
                                    },
                                    iName,
                                    fmt::printf("Rename widget %s -> %s", getName(), iName),
                                    MergeKey::from(&fName));
}

//------------------------------------------------------------------------
// Widget::setPositionAction
//------------------------------------------------------------------------
ImVec2 Widget::setPositionAction(ImVec2 const &iPosition)
{
  auto res = getPosition();
  fGraphics->setPosition(iPosition);
  fEdited |= fGraphics->isEdited();
  return res;
}

//------------------------------------------------------------------------
// Widget::setPosition
//------------------------------------------------------------------------
void Widget::setPosition(ImVec2 const &iPosition)
{
  executeAction<WidgetValueAction<ImVec2>>([](Widget *w, auto value) {
                                             return w->setPositionAction(value);
                                           },
                                           iPosition,
                                           fmt::printf("Move [%s] to %.0fx%0.f", getName(), iPosition.x, iPosition.y),
                                           MergeKey::from(&fName));
}


//------------------------------------------------------------------------
// Widget::setNameAction
//------------------------------------------------------------------------
std::string Widget::setNameAction(std::string iName)
{
  auto res = fName.value();
  fName = StringWithHash(std::move(iName));
  fEdited = true;
  return res;
}

}