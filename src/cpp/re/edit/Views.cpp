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

#include "Views.h"
#include "imgui.h"
#include "Errors.h"
#include "Constants.h"
#include "ReGui.h"
#include <utility>

namespace re::edit::views {

//------------------------------------------------------------------------
// MultiSelectionList::handleClick
//------------------------------------------------------------------------
void MultiSelectionList::handleClick(std::string const &s, bool iRangeSelectKey, bool iMultiSelectKey)
{
  // when iMultiSelectKey is held => multiple selection
  if(iMultiSelectKey)
  {
    if(fSelected.find(s) == fSelected.end())
    {
      fSelected.emplace(s);
      fLastSelected = s;
    }
    else
    {
      fSelected.erase(s);
      fLastSelected = std::nullopt;
    }
    return;
  }

  // when iRangeSelectKey is held => add all properties between fLastSelected and this one
  if(iRangeSelectKey && fLastSelected)
  {
    bool copy = false;
    for(auto &elt: fList)
    {
      if(s != *fLastSelected && (elt == s || elt == *fLastSelected))
      {
        copy = !copy;
        fSelected.emplace(elt);
      }
      else if(copy)
        fSelected.emplace(elt);
    }

    fLastSelected = s;
    return;
  }

  // neither shift nor control is held => single selection (deselect all others)
  if(fSelected.find(s) == fSelected.end() || fSelected.size() > 1)
  {
    fSelected.clear();
    fSelected.emplace(s);
    fLastSelected = s;
  }
  else
  {
    fSelected.clear();
    fLastSelected = std::nullopt;
  }
}

//------------------------------------------------------------------------
// MultiSelectionList::editView
//------------------------------------------------------------------------
void MultiSelectionList::editView()
{
  for(auto &s: fList)
  {
    if(ImGui::Selectable(s.c_str(), fSelected.find(s) != fSelected.end()))
    {
      auto io = ImGui::GetIO();
      handleClick(s, io.KeyShift, io.KeySuper);
    }
  }

}

//------------------------------------------------------------------------
// MultiSelectionList::sort
//------------------------------------------------------------------------
void MultiSelectionList::sort()
{
  if(fSortBy)
    fSortBy(fList, fSortCriteria);
}

//------------------------------------------------------------------------
// MultiSelectionList::moveSelectionTo
//------------------------------------------------------------------------
void MultiSelectionList::moveSelectionTo(MultiSelectionList &ioOther)
{
  if(fSelected.empty())
    return;

  for(auto &s: fList)
  {
    if(fSelected.find(s) != fSelected.end())
      ioOther.fList.emplace_back(s);
  }

  for(auto &s: fSelected)
  {
    fList.erase(std::find(fList.begin(), fList.end(), s));
  }

  fSelected.clear();
}

//------------------------------------------------------------------------
// MultiSelectionList::clearSelection
//------------------------------------------------------------------------
void MultiSelectionList::clearSelection()
{
  fSelected.clear();
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// MultiSelectionList::selectAll
//------------------------------------------------------------------------
void MultiSelectionList::selectAll()
{
  std::copy(fList.begin(), fList.end(), std::inserter(fSelected, fSelected.end()));
  fLastSelected = std::nullopt;
}

//------------------------------------------------------------------------
// MultiSelectionList::moveSelectionUp
//------------------------------------------------------------------------
void MultiSelectionList::moveSelectionUp()
{
  std::vector<std::string> sublist{};
  sublist.reserve(fSelected.size());
  std::copy_if(fList.begin(), fList.end(), std::back_inserter(sublist), [this](auto &s) { return isSelected(s); });

  for(auto &s: sublist)
  {
    auto iter = std::find(fList.begin(), fList.end(), s);

    // already at the top
    if(iter == fList.begin())
      break; // abort loop

    std::swap(*(iter - 1), *iter);
  }
}

//------------------------------------------------------------------------
// MultiSelectionList::moveSelectionDown
//------------------------------------------------------------------------
void MultiSelectionList::moveSelectionDown()
{
  std::vector<std::string> sublist{};
  sublist.reserve(fSelected.size());
  std::copy_if(fList.begin(), fList.end(), std::back_inserter(sublist), [this](auto &s) { return isSelected(s); });

  // we need to iterate backward
  for(auto i = sublist.rbegin(); i != sublist.rend(); i++)
  {
    auto &s = *i;

    auto iter = std::find(fList.begin(), fList.end(), s);

    // already at the bottom
    if(iter + 1 == fList.end())
      break; // abort loop

    std::swap(*(iter + 1), *iter);
  }

}

//------------------------------------------------------------------------
// MultiSelectionList::setupTableHeader
//------------------------------------------------------------------------
void MultiSelectionList::setupTableHeader(int iColumnIndex)
{
  ImGui::TableSetColumnIndex(iColumnIndex);
  ImGui::PushID(iColumnIndex);

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 1));
  if(ImGui::Button(ReGui::kMenuIcon))
    ImGui::OpenPopup("Header Menu");
  ImGui::PopStyleVar();

  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::TableHeader(ImGui::TableGetColumnName(iColumnIndex));

  if(ImGui::BeginPopup("Header Menu"))
  {
    if(ImGui::MenuItem("Select All"))
      selectAll();
    if(ImGui::MenuItem("Select None"))
      clearSelection();
    if(!fSortCriteriaList.empty())
    {
      if(ImGui::BeginMenu(ReGui_Icon_Sort " Sort"))
      {
        for(auto &sortCriteria: fSortCriteriaList)
        {
          if(ImGui::MenuItem(fmt::printf("By %s", sortCriteria).c_str(), nullptr, sortCriteria == fSortCriteria))
          {
            fSortCriteria = sortCriteria;
            sort();
          }
        }
        ImGui::EndMenu();
      }
    }
    ImGui::EndPopup();
  }

  ImGui::PopID();
}

//------------------------------------------------------------------------
// StringList::StringList
//------------------------------------------------------------------------
StringListEdit::StringListEdit(std::vector<std::string> iSourceList,
                               std::string iSourceName,
                               MultiSelectionList::sort_by_t iSourceSortBy,
                               std::vector<std::string> iSourceSortCriteriaList,
                               std::string iSourceSortCriteria,
                               std::vector<std::string> iDestinationList,
                               std::string iDestinationName) :
  fSourceName{std::move(iSourceName)},
  fDestinationName{std::move(iDestinationName)}
{
  auto const &style = ImGui::GetStyle();

  // account for scrollbar size
  auto extraWidth = style.ScrollbarSize + style.FramePadding.x;

  std::string max{};

  for(auto &s: iSourceList)
  {
    if(s.size() > max.size())
      max = s;
  }

  auto const size = ImGui::CalcTextSize(max.c_str());

  fSize = ImVec2{size.x + extraWidth, size.y * 25};

  // handle destination first
  fDestination.fList = std::move(iDestinationList);

  fSource.fSortBy = std::move(iSourceSortBy);
  if(fSource.fSortBy)
  {
    RE_EDIT_INTERNAL_ASSERT(!iSourceSortCriteriaList.empty());
    fSource.fSortCriteriaList = std::move(iSourceSortCriteriaList);
    fSource.fSortCriteria = std::move(iSourceSortCriteria);
  }

  if(fDestination.empty())
  {
    fSource.fList = std::move(iSourceList);
  }
  else
  {
    // we need to remove the elements from source that are already in destination
    std::set<std::string> src{iSourceList.begin(), iSourceList.end()};

    for(auto &s: fDestination.fList)
      src.erase(s);

    std::copy_if(iSourceList.begin(), iSourceList.end(), std::back_inserter(fSource.fList), [&src](auto const &s) { return src.find(s) != src.end(); });
  }

  fSource.sort();
}

//------------------------------------------------------------------------
// StringList::editView
//------------------------------------------------------------------------
void StringListEdit::editView()
{
  if(ImGui::BeginTable("StringListEdit", 3, ImGuiTableFlags_Borders))
  {
    ImGui::TableSetupColumn(fSourceName.c_str(), ImGuiTableColumnFlags_WidthFixed, fSize.x);
    ImGui::TableSetupColumn("Action");
    ImGui::TableSetupColumn(fDestinationName.c_str(), ImGuiTableColumnFlags_WidthFixed, fSize.x);

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

    // Column 0 setup
    fSource.setupTableHeader(0);

    // Column 1 setup
    ReGui::DefaultHeaderColumn(1);

    // Column 2 setup
    fDestination.setupTableHeader(2);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    if(ImGui::BeginChild("##Source", fSize))
    {
      fSource.editView();
    }
    ImGui::EndChild();

    auto column0Height = ImGui::GetItemRectSize().y;

    ImGui::TableSetColumnIndex(1);

    const ImVec2 buttonSize{-FLT_MIN, 0.0f};

    // This "complex" logic is so that the 2 buttons are vertically centered in the column :(
    auto const &style = ImGui::GetStyle();
    auto buttonHeight = ImGui::CalcTextSize("->").y + (style.FramePadding.y * 2.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (column0Height / 2.0f) - buttonHeight - (style.ItemSpacing.y / 2.0f));

    ImGui::PushID("Source");
    ImGui::BeginDisabled(fSource.fSelected.empty());
    if(ImGui::Button("->", buttonSize))
      fSource.moveSelectionTo(fDestination);
    ImGui::EndDisabled();
    ImGui::PopID();

    ImGui::PushID("Dest.");
    ImGui::BeginDisabled(fDestination.fSelected.empty());
    if(ImGui::Button("<-", buttonSize))
    {
      fDestination.moveSelectionTo(fSource);
      fSource.sort();
    }
    ImGui::EndDisabled();
    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    if(ImGui::BeginChild("##Destination", fSize))
    {
      fDestination.editView();
    }
    ImGui::EndChild();

    ImGui::EndTable();
  }
}

}