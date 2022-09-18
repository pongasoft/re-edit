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

#ifndef RE_EDIT_VIEWS_H
#define RE_EDIT_VIEWS_H

#include <vector>
#include <set>
#include <string>
#include "imgui.h"

namespace re::edit::views {

class MultiSelectionList
{
public:
  using sort_by_t = std::function<void(std::vector<std::string> &ioString, std::string const &iSortCriteria)>;

public:
  void handleClick(std::string const &s, bool iRangeSelectKey, bool iMultiSelectKey);
  void editView();
  void moveSelectionTo(MultiSelectionList &ioOther);
  void moveSelectionUp();
  void moveSelectionDown();
  void sort();
  void selectAll();
  void clearSelection();
  void setupTableHeader(int iColumnIndex);
  inline bool isSelected(std::string const &s) const { return fSelected.find(s) != fSelected.end(); }
  constexpr bool empty() const { return fList.empty(); }
  constexpr size_t selectedCount() const { return fSelected.size(); }

public:
  std::vector<std::string> fList{};
  std::optional<std::string> fLastSelected{};
  std::set<std::string> fSelected{};
  std::vector<std::string> fSortCriteriaList{};
  std::string fSortCriteria{};
  sort_by_t fSortBy{};
};

class StringListEdit
{
public:
  StringListEdit(std::vector<std::string> iSourceList,
                 std::string iSourceName,
                 MultiSelectionList::sort_by_t iSourceSortBy,
                 std::vector<std::string> iSourceSortCriteriaList,
                 std::string iSourceSortCriteria,
                 std::vector<std::string> iDestinationList,
                 std::string iDestinationName);
  void editView();

  std::vector<std::string> const &source() const { return fSource.fList; }
  std::vector<std::string> &source() { return fSource.fList; }

  std::vector<std::string> const &destination() const { return fDestination.fList; }
  std::vector<std::string> &destination() { return fDestination.fList; }

private:
  MultiSelectionList fSource{};
  std::string fSourceName;
  MultiSelectionList fDestination{};
  std::string fDestinationName;

  ImVec2 fSize{};
};

}

#endif //RE_EDIT_VIEWS_H
