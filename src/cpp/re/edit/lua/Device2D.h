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

#ifndef RE_EDIT_DEVICE_2D_H
#define RE_EDIT_DEVICE_2D_H

#include "Base.h"
#include "../Graphics.h"
#include <map>

namespace re::edit::lua {

struct gfx_node
{
  std::string fName{};
  ImVec2 fPosition{};
  std::variant<std::monostate, std::string, ImVec2> fKeyOrSize{};
  std::optional<int> fNumFrames{};

  bool hasKey() const { return std::holds_alternative<std::string>(fKeyOrSize); }
  std::string getKey() const { return std::get<std::string>(fKeyOrSize); }

  bool hasSize() const { return std::holds_alternative<ImVec2>(fKeyOrSize); }
  ImVec2 getSize() const { return std::get<ImVec2>(fKeyOrSize); }
};

struct panel_nodes
{
  explicit panel_nodes(std::vector<std::string> iDecalNames);

  std::map<std::string, gfx_node> fNodes{};

  std::map<std::string, int> getNumFrames() const;

  std::optional<gfx_node> findNodeByName(std::string const &iName) const;

  void addNode(std::optional<std::string> const &iName, gfx_node iNode);
  void maybeAddNode(std::string const &iName, ImVec2 const &iOffset);

  std::vector<std::string> getDecalNames(std::set<std::string> const &iWidgetNames) const;

private:
  std::vector<std::string> fDecalNames{}; // stored in device_2D.lua under re_edit.<panel_type>.decals
  std::vector<std::string> fNodeNames{};  // to keep the order of other names (when reading a project for the first time, orphan nodes are treated as decals)
  int fAnonymousDecalCount{};
};

class Device2D : public Base
{
public:
  Device2D() = default;

  std::shared_ptr<panel_nodes> front() const { return getPanelNodes("front"); }
  std::shared_ptr<panel_nodes> folded_front() const { return getPanelNodes("folded_front"); }
  std::shared_ptr<panel_nodes> back() const { return getPanelNodes("back"); }
  std::shared_ptr<panel_nodes> folded_back() const { return getPanelNodes("folded_back"); }

  static std::unique_ptr<Device2D> fromFile(fs::path const &iLuaFilename);

protected:
  std::shared_ptr<panel_nodes> getPanelNodes(char const *iPanelName) const;
  std::shared_ptr<panel_nodes> createPanelNodes(char const *iPanelName);
  ImVec2 processLuaTable(std::optional<std::string> const &iName, ImVec2 iOffset, panel_nodes &oPanelNodes);
  std::optional<gfx_node> getMaybeNodeOnTopOfStack();
  std::optional<ImVec2> getOptionalOffset();
};


}

#endif //RE_EDIT_DEVICE_2D_H