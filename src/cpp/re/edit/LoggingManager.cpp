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

#include "LoggingManager.h"
#include <imgui.h>

namespace re::edit {

//------------------------------------------------------------------------
// LoggingManager::instance
//------------------------------------------------------------------------
LoggingManager *LoggingManager::instance()
{
  static LoggingManager fUniqueInstance{};
  return &fUniqueInstance;
}

//------------------------------------------------------------------------
// LoggingManager::debug
//------------------------------------------------------------------------
void LoggingManager::debug(std::string const &iKey, std::string iMessage)
{
  std::lock_guard<std::mutex> lock(fMutex);

  fDebug[iKey] = std::move(iMessage);
}

//------------------------------------------------------------------------
// LoggingManager::clearDebug
//------------------------------------------------------------------------
void LoggingManager::clearDebug(std::string const &iKey)
{
  std::lock_guard<std::mutex> lock(fMutex);

  fDebug.erase(iKey);
}

//------------------------------------------------------------------------
// LoggingManager::clearDebug
//------------------------------------------------------------------------
void LoggingManager::clearDebug()
{
  std::lock_guard<std::mutex> lock(fMutex);

  fDebug.clear();
}

//------------------------------------------------------------------------
// LoggingManager::clearLog
//------------------------------------------------------------------------
void LoggingManager::clearLog()
{
  std::lock_guard<std::mutex> lock(fMutex);

  fLog.clear();
}

//------------------------------------------------------------------------
// LoggingManager::logEntry
//------------------------------------------------------------------------
void LoggingManager::logEntry(LoggingManager::LogEntry iEntry)
{
  std::lock_guard<std::mutex> lock(fMutex);

  if(fLog.size() > kMaxLogEntries)
    fLog.erase(fLog.begin());
  fLog.emplace_back(std::move(iEntry));
  fScrollLog = true;
}

//------------------------------------------------------------------------
// LoggingManager::renderLog
//------------------------------------------------------------------------
void LoggingManager::renderLog()
{
  static const ImVec4 kInfoColor{1,1,1,1};
  static const ImVec4 kWarningColor{0.98,0.38,0.26,1};
  static const ImVec4 kErrorColor{1,0,0,1};

  if(ImGui::Begin("Log", &fShowLog))
  {
    for(auto const &entry: fLog)
    {
      auto level = entry.fLevel == LogLevel::kInfo ? "INFO" : (entry.fLevel == LogLevel::kWarning ? "WARN" : "ERR ");
      auto &color = entry.fLevel == LogLevel::kInfo ? kInfoColor : (entry.fLevel == LogLevel::kWarning ? kWarningColor : kErrorColor);
      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextWrapped("%s | %s", level, entry.fMessage.c_str());
      ImGui::PopStyleColor();
    }
    ImGui::BeginDisabled(fLog.empty());
    if(ImGui::Button("Clear"))
      clearLog();
    if(fScrollLog)
    {
      ImGui::SetScrollHereY(1.0);
      fScrollLog = false;
    }
    ImGui::EndDisabled();
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// LoggingManager::renderLog
//------------------------------------------------------------------------
void LoggingManager::renderDebug()
{
  if(ImGui::Begin("Debug", &fShowDebug))
  {
    for(auto const &iter: fDebug)
    {
      ImGui::Text("%s | %s", iter.first.c_str(), iter.second.c_str());
    }
  }
  ImGui::End();
}

//------------------------------------------------------------------------
// LoggingManager::render
//------------------------------------------------------------------------
void LoggingManager::render()
{
  std::lock_guard<std::mutex> lock(fMutex);

  if(fShowDebug)
    renderDebug();

  if(fShowLog)
    renderLog();
}

//------------------------------------------------------------------------
// LoggingManager::getLogCount
//------------------------------------------------------------------------
size_t LoggingManager::getLogCount() const
{
  std::lock_guard<std::mutex> lock(fMutex);
  return fLog.size();
}

//------------------------------------------------------------------------
// LoggingManager::isShowDebug
//------------------------------------------------------------------------
bool LoggingManager::isShowDebug() const
{
  std::lock_guard<std::mutex> lock(fMutex);
  return fShowDebug;
}

//------------------------------------------------------------------------
// LoggingManager::setShowDebug
//------------------------------------------------------------------------
void LoggingManager::setShowDebug(bool b)
{
  std::lock_guard<std::mutex> lock(fMutex);
  fShowDebug = b;
}

//------------------------------------------------------------------------
// LoggingManager::isShowLog
//------------------------------------------------------------------------
bool LoggingManager::isShowLog() const
{
  std::lock_guard<std::mutex> lock(fMutex);
  return fShowLog;
}

//------------------------------------------------------------------------
// LoggingManager::isShowLog
//------------------------------------------------------------------------
void LoggingManager::setShowLog(bool b)
{
  std::lock_guard<std::mutex> lock(fMutex);
  fShowLog = b;
}

//------------------------------------------------------------------------
// LoggingManager::showLog
//------------------------------------------------------------------------
void LoggingManager::showLog()
{
  std::lock_guard<std::mutex> lock(fMutex);
  fShowLog = true;
}


}