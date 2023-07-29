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

#include "../Errors.h"
#include "Device2D.h"

namespace re::edit::lua {

//------------------------------------------------------------------------
// Device2D::fromFile
//------------------------------------------------------------------------
std::unique_ptr<Device2D> Device2D::fromFile(fs::path const &iLuaFilename)
{
  auto res = std::make_unique<Device2D>();
  res->loadFile(iLuaFilename);
  return res;
}

//------------------------------------------------------------------------
// Device2D::getPanelNodes
//------------------------------------------------------------------------
std::shared_ptr<panel_nodes> Device2D::getPanelNodes(char const *iPanelName) const
{
  auto nct = const_cast<Device2D *>(this); // remove const
  return nct->createPanelNodes(iPanelName);
}

//------------------------------------------------------------------------
// Device2D::createPanelNodes
//------------------------------------------------------------------------
std::shared_ptr<panel_nodes> Device2D::createPanelNodes(char const *iPanelName)
{
  std::vector<std::string> decalNames{};
  if(lua_getglobal(L, "re_edit") == LUA_TTABLE)
  {
    if(lua_getfield(L, -1, iPanelName) == LUA_TTABLE)
    {
      if(lua_getfield(L, -1, "decals") == LUA_TTABLE)
      {
        iterateLuaArray([&decalNames, this](auto i) {
                          auto s = lua_tostring(L, -1);
                          if(s != nullptr)
                            decalNames.emplace_back(s);
                          else
                            decalNames.emplace_back(fmt::printf("panel_decal_%d", i));
                        },
                        true, false);
      }
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  auto def = std::make_shared<panel_nodes>(decalNames);
  if(lua_getglobal(L, iPanelName) == LUA_TTABLE)
  {
    processLuaTable({}, {}, *def);
  }
  lua_pop(L, 1);
  return def;
}

//------------------------------------------------------------------------
// Device2D::processLuaTable
// Assumes that the top of the stack contains a lua table to process
//------------------------------------------------------------------------
ImVec2 Device2D::processLuaTable(std::optional<std::string> const &iName, ImVec2 iOffset, panel_nodes &oPanelNodes)
{
  // we process offset first because the order in which keys are processed is not guaranteed
  auto maybeOffset = getOptionalOffset();
  if(maybeOffset)
    iOffset += *maybeOffset;

  iterateLuaTable([this, &iName, &iOffset, &oPanelNodes](lua_table_key_t const &key) {
    if(std::holds_alternative<std::string>(key))
    {
      auto name = std::get<std::string>(key);
      if(name == "offset")
      {
        // do nothing (already handled)
      }
      else
      {
        oPanelNodes.maybeAddNode(name, processLuaTable(name, iOffset, oPanelNodes));
      }
    }
    else
    {
      auto node = getMaybeNodeOnTopOfStack();
      if(node)
      {
        node->fPosition = iOffset;
        oPanelNodes.addNode(iName, std::move(*node));
      }
      else
      {
        auto offset = processLuaTable(iName, iOffset, oPanelNodes);
        if(iName)
          oPanelNodes.maybeAddNode(*iName, offset);
      }
    }
  }, true, false);

  return iOffset;
}

//------------------------------------------------------------------------
// Device2D::getOptionalOffset
//------------------------------------------------------------------------
std::optional<ImVec2> Device2D::getOptionalOffset()
{
  std::optional<ImVec2> res{};
  if(lua_type(L, -1) == LUA_TTABLE)
  {
    // check for size
    lua_getfield(L, -1, "offset");
    if(isTableOnTopOfStack())
      res = getImVec2();
    lua_pop(L, 1);
  }
  return res;
}

//------------------------------------------------------------------------
// Device2D::getMaybeNodeOnTopOfStack
//------------------------------------------------------------------------
std::optional<gfx_node> Device2D::getMaybeNodeOnTopOfStack()
{
  if(lua_type(L, -1) == LUA_TTABLE)
  {
    gfx_node node{};

    // check for size
    lua_getfield(L, -1, "size");
    if(lua_type(L, -1) == LUA_TTABLE)
      node.fKeyOrSize = getImVec2();
    lua_pop(L, 1);

    // check for path override (when there are effects)
    auto pathOverride = L.getTableValueAsOptionalString("re_edit_path");
    if(pathOverride)
    {
      node.fKeyOrSize = pathOverride.value();
    }
    else
    {
      // check for path
      auto path = L.getTableValueAsOptionalString("path");
      if(path)
        node.fKeyOrSize = path.value();
    }

    auto frames = L.getTableValueAsOptionalInteger("frames");
    if(frames)
      node.fNumFrames = static_cast<int>(frames.value());

    // re_edit_size
    lua_getfield(L, -1, "re_edit_size");
    if(lua_type(L, -1) == LUA_TTABLE)
      node.fEffects.fSizeOverride = getImVec2();
    lua_pop(L, 1);

    // re_edit_tint
    JboxColor3 tint;
    if(withField(-1, "re_edit_tint", LUA_TTABLE, [this, &tint]() {
      tint.fRed = static_cast<int>(L.getArrayValueAsInteger(1));
      tint.fGreen = static_cast<int>(L.getArrayValueAsInteger(2));
      tint.fBlue = static_cast<int>(L.getArrayValueAsInteger(3));
    }))
    {
      node.fEffects.fTint = ReGui::GetColorImU32(tint);
    }

    // re_edit_brightness
    auto brightness = L.getTableValueAsOptionalNumber("re_edit_brightness");
    if(brightness)
      node.fEffects.fBrightness = static_cast<float>(brightness.value());

    // re_edit_flip_x
    if(L.getTableValueAsBoolean("re_edit_flip_x"))
      node.fEffects.fFlipX = true;

    // re_edit_flip_y
    if(L.getTableValueAsBoolean("re_edit_flip_y"))
      node.fEffects.fFlipY = true;

    if(node.hasKey() || node.hasSize())
    {
      return node;
    }
  }

  return std::nullopt;
}


//------------------------------------------------------------------------
// panel_nodes::panel_nodes
//------------------------------------------------------------------------
panel_nodes::panel_nodes(std::vector<std::string> iDecalNames) :
  fDecalNames{std::move(iDecalNames)}
{
}

//------------------------------------------------------------------------
// panel_nodes::findNodeByName
//------------------------------------------------------------------------
std::optional<gfx_node> panel_nodes::findNodeByName(std::string const &iName) const
{
  auto iter = fNodes.find(iName);
  if(iter != fNodes.end())
    return iter->second;
  else
    return std::nullopt;
}

//------------------------------------------------------------------------
// panel_nodes::addNode
//------------------------------------------------------------------------
void panel_nodes::addNode(std::optional<std::string> const &iName, gfx_node iNode)
{
  if(iName)
  {
    iNode.fName = *iName;
    fNodeNames.emplace_back(iNode.fName);
  }
  else
  {
    if(fAnonymousDecalCount < fDecalNames.size())
    {
      iNode.fName = fDecalNames[fAnonymousDecalCount];
    }
    else
    {
      iNode.fName = fmt::printf("panel_decal_%d", fAnonymousDecalCount + 1);
      fDecalNames.emplace_back(iNode.fName);
    }
    fAnonymousDecalCount++;
  }
  fNodes[iNode.fName] = std::move(iNode);
}

//------------------------------------------------------------------------
// panel_nodes::maybeAddNode
//------------------------------------------------------------------------
void panel_nodes::maybeAddNode(std::string const &iName, ImVec2 const &iOffset)
{
  if(fNodes.find(iName) == fNodes.end())
  {
    gfx_node node{};
    node.fName = iName;
    node.fPosition = iOffset;
    fNodeNames.emplace_back(iName);
    fNodes[iName] = node;
  }
}

//------------------------------------------------------------------------
// panel_nodes::getDecalNames
//------------------------------------------------------------------------
std::vector<std::string> panel_nodes::getDecalNames(std::set<std::string> const &iWidgetNames) const
{
  std::vector<std::string> res{};

  for(auto &name: fDecalNames)
  {
    // in the event that hdgui_2D.lua is modified manually, a widget could be added that is defined as a decal in
    // device_2D.lua, hence the check to be sure
    if(iWidgetNames.find(name) == iWidgetNames.end())
      res.emplace_back(name);
  }

  for(auto &name: fNodeNames)
  {
    // an orphan node name is a decal
    if(iWidgetNames.find(name) == iWidgetNames.end())
      res.emplace_back(name);
  }

  return res;
}


}
