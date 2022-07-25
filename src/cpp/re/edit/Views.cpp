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
#include <utility>

namespace re::edit::views {

//------------------------------------------------------------------------
// MultiSelectionList::handleClick
//------------------------------------------------------------------------
void MultiSelectionList::handleClick(std::string const &s, bool iIsShift, bool iIsControl)
{
  // when control is held => multiple selection
  if(iIsControl)
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

  // when shift is held => add all properties between fLastSelected and this one
  if(iIsShift && fLastSelected)
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

  // neither shift or control is held => single selection (deselect all others)
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
      handleClick(s, io.KeyShift, io.KeyCtrl);
    }
  }

}

//------------------------------------------------------------------------
// MultiSelectionList::sort
//------------------------------------------------------------------------
void MultiSelectionList::sort()
{
  std::sort(fList.begin(), fList.end());
}

//------------------------------------------------------------------------
// MultiSelectionList::moveSelectionTo
//------------------------------------------------------------------------
void MultiSelectionList::moveSelectionTo(MultiSelectionList &ioOther)
{
  if(fSelected.empty())
    return;

  for(auto &s: fSelected)
  {
    ioOther.fList.emplace_back(s);
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
// StringList::StringList
//------------------------------------------------------------------------
StringListEdit::StringListEdit(std::vector<std::string> iSourceList,
                               std::string iSourceName,
                               bool iKeepSourceSorted,
                               std::vector<std::string> iDestinationList,
                               std::string iDestinationName,
                               bool iKeepDestinationSorted) :
  fSourceName{std::move(iSourceName)},
  fKeepSourceSorted{iKeepSourceSorted},
  fDestinationName{std::move(iDestinationName)},
  fKeepDestinationSorted{iKeepDestinationSorted}
{
  std::string max{};

  for(auto &s: iSourceList)
  {
    if(s.size() > max.size())
      max = s;
  }

  auto const size = ImGui::CalcTextSize(max.c_str());

  fSize = ImVec2{size.x, size.y * 25};

  // handle destination first
  fDestination.fList = std::move(iDestinationList);
  if(fKeepDestinationSorted)
    fDestination.sort();

  if(fDestination.empty())
  {
    fSource.fList = std::move(iSourceList);
    if(fKeepSourceSorted)
      fSource.sort();
  }
  else
  {
    // we need to remove the elements from source that are already in destination
    std::set<std::string> src{iSourceList.begin(), iSourceList.end()};

    for(auto &s: fDestination.fList)
      src.erase(s);

    if(fKeepSourceSorted)
      // we can use src because it is already sorted...
      std::copy(src.begin(), src.end(), std::back_inserter(fSource.fList));
    else
    {
      std::copy_if(iSourceList.begin(), iSourceList.end(), std::back_inserter(fSource.fList), [&src](auto const &s) { return src.find(s) != src.end(); });
    }
  }
}

//------------------------------------------------------------------------
// StringList::editView
//------------------------------------------------------------------------
void StringListEdit::editView()
{
  if(ImGui::BeginTable("StringListEdit", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders))
  {
    auto height = fSize.y;

    ImGui::TableSetupColumn(fSourceName.c_str());
    ImGui::TableSetupColumn("Action");
    ImGui::TableSetupColumn(fDestinationName.c_str());
    ImGui::TableHeadersRow();

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    if(ImGui::BeginChild("##Source", fSize))
    {
      fSource.editView();
    }
    ImGui::EndChild();

    ImGui::TableSetColumnIndex(1);

    const ImVec2 buttonSize{-FLT_MIN, 0.0f};

    ImGui::PushID("Source");
    ImGui::Text("Left");
    ImGui::BeginDisabled(fSource.fSelected.empty());
    if(ImGui::Button("->", buttonSize))
      fSource.moveSelectionTo(fDestination);
    if(!fKeepSourceSorted)
    {
      if(ImGui::Button("Up", buttonSize))
        fSource.moveSelectionUp();
      ImGui::SameLine();
      if(ImGui::Button("Down", buttonSize))
        fSource.moveSelectionDown();
    }
    if(ImGui::Button("None", buttonSize))
      fSource.clearSelection();
    ImGui::EndDisabled();
    if(ImGui::Button("All", buttonSize))
      fSource.selectAll();
    ImGui::PopID();

    ImGui::Separator();

    ImGui::PushID("Dest.");
    ImGui::Text("Right");
    ImGui::BeginDisabled(fDestination.fSelected.empty());
    if(ImGui::Button("<-", buttonSize))
    {
      fDestination.moveSelectionTo(fSource);
      if(fKeepSourceSorted)
        fSource.sort();
    }
    if(!fKeepDestinationSorted)
    {
      if(ImGui::Button("Up", buttonSize))
        fDestination.moveSelectionUp();
      if(ImGui::Button("Down", buttonSize))
        fDestination.moveSelectionDown();
    }
    if(ImGui::Button("None", buttonSize))
      fDestination.clearSelection();
    ImGui::EndDisabled();
    if(ImGui::Button("All", buttonSize))
      fDestination.selectAll();
    ImGui::PopID();


    ImGui::TableSetColumnIndex(2);
//    if(ImGui::BeginChild("##Destination", {ImGui::GetColumnWidth(), height}))
    if(ImGui::BeginChild("##Destination", fSize))
    {
      fDestination.editView();
    }
    ImGui::EndChild();

    ImGui::EndTable();
  }
}

}