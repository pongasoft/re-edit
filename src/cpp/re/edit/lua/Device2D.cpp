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
  std::vector<std::optional<std::string>> decalNames{};
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
                            decalNames.emplace_back(std::nullopt);
                        },
                        true, false);
      }
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  auto def = std::make_shared<panel_nodes>();
  if(lua_getglobal(L, iPanelName) == LUA_TTABLE)
  {
    processLuaTable({}, decalNames, *def.get());
  }
  lua_pop(L, 1);
  return def;
}

//------------------------------------------------------------------------
// Device2D::processLuaTable
// Assumes that the top of the stack contains a lua table to process
//------------------------------------------------------------------------
void Device2D::processLuaTable(ImVec2 iOffset, std::vector<std::optional<std::string>> const &iDecalNames, panel_nodes &oPanelNodes)
{
  // we process offset first because the order in which keys are processed is not guaranteed
  auto maybeOffset = getOptionalOffset();
  if(maybeOffset)
    iOffset += *maybeOffset;

  iterateLuaTable([this, &iOffset, &iDecalNames, &oPanelNodes](lua_table_key_t const &key) {
    if(std::holds_alternative<std::string>(key))
    {
      auto name = std::get<std::string>(key);
      if(name == "offset")
      {
        // do nothing (already handled)
      }
      else if(name == "path")
      {
        if(lua_type(L, -1) == LUA_TSTRING)
        {
          auto idx = oPanelNodes.fDecalNodes.size();
          auto decalName = idx < iDecalNames.size() ? iDecalNames[idx] : std::nullopt;
          oPanelNodes.fDecalNodes.emplace_back(gfx_decal_node{iOffset, lua_tostring(L, -1), decalName});
        }
      }
      else
      {
        processGfxNode(name, iOffset, oPanelNodes);
      }
    }
    else
    {
      processLuaTable(iOffset, iDecalNames, oPanelNodes);
    }
  }, true, false);
}

//------------------------------------------------------------------------
// Device2D::processGfxNode
//------------------------------------------------------------------------
void Device2D::processGfxNode(std::string const &iName, ImVec2 iOffset, panel_nodes &oPanelNodes)
{
  auto maybeOffset = getOptionalOffset();
  if(maybeOffset)
    iOffset += *maybeOffset;

  if(lua_type(L, -1) == LUA_TTABLE)
  {
    auto node = gfx_node{
      /* .fName = */ iName,
      /* .fPosition = */ iOffset
    };

    iterateLuaTable([this, &node](lua_table_key_t const &key) {
      if(std::holds_alternative<std::string>(key))
      {
        // do nothing (already handled)
      }
      else
      {
        // make sure that we have a table
        if(lua_type(L, -1) == LUA_TTABLE)
        {
          // check for size
          lua_getfield(L, -1, "size");
          if(lua_type(L, -1) == LUA_TTABLE)
            node.fKeyOrSize = getImVec2();
          lua_pop(L, 1);

          // override with path if provided
          auto path = L.getTableValueAsOptionalString("path");
          if(path)
            node.fKeyOrSize = path.value();


          auto frames = L.getTableValueAsOptionalInteger("frames");
          if(frames)
            node.fNumFrames = static_cast<int>(frames.value());


        }
      }
    }, true, false);

    oPanelNodes.fNodes[iName] = node;
  }
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
    if(lua_type(L, -1) == LUA_TTABLE)
      res = getImVec2();
    lua_pop(L, 1);
  }
  return res;
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
}
