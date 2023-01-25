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

#ifndef RE_EDIT_CLIPBOARD_H
#define RE_EDIT_CLIPBOARD_H

#include <memory>
#include <string>

namespace re::edit {

class Widget;

namespace widget { class Attribute; };

class Clipboard
{
public:
  struct Item
  {
    explicit Item(std::unique_ptr<Widget> iWidget, int iAttributeId = -1);
    ~Item();

    constexpr bool isWidgetItem() const { return fAttributeId < 0;}
    constexpr bool isAttributeItem() const { return !isWidgetItem(); }

    inline Widget const *getWidget() const { return fWidget.get(); }
    widget::Attribute const *getAttribute() const;
    inline std::string const &getDescription() const { return fDescription; }

  private:
    // implementation note: should be using unique_ptr but it requires the Widget type to be defined creating a cycle :(
    std::unique_ptr<Widget> fWidget;
    int fAttributeId;

    std::string fDescription;
  };

public:
  constexpr bool isEmpty() const { return fItem == nullptr; }
  Item const *getItem() const { return fItem.get(); }
  void addItem(std::unique_ptr<Widget> iWidget, int iAttributeId = -1);

private:
  std::unique_ptr<Item> fItem{};
};

}

#endif //RE_EDIT_CLIPBOARD_H