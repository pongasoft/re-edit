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

#ifndef RE_EDIT_LOGGING_MANAGER_H
#define RE_EDIT_LOGGING_MANAGER_H

#include <map>
#include <string>
#include <vector>
#include <re/mock/fmt.h>

namespace re::edit {

class LoggingManager
{
public:
  template<typename ... Args>
  inline void debug(std::string const &iKey, const std::string& format, Args ... args);

  void debug(std::string const &iKey, std::string iMessage);
  void clearDebug(std::string const &iKey);
  void clearDebug();

  void logInfo(std::string iMessage) { logEntry({LogLevel::kInfo, std::move(iMessage)}); }
  void logWarning(std::string iMessage) { logEntry({LogLevel::kWarning, std::move(iMessage)}); }
  void logError(std::string iMessage) { logEntry({LogLevel::kError, std::move(iMessage)}); }

  template<typename ... Args>
  void logInfo(const std::string& format, Args ... args);
  template<typename ... Args>
  void logWarning(const std::string& format, Args ... args);
  template<typename ... Args>
  void logError(const std::string& format, Args ... args);

  size_t getLogCount() const { return fLog.size(); }

  void clearLog();

  void clearAll() { clearDebug(); clearLog(); }

  void render();

  bool isShowDebug() const { return fShowDebug; }
  bool isShowLog() const { return fShowLog; }

  bool &getShowDebug() { return fShowDebug; };
  bool &getShowLog() { return fShowLog; };

  static LoggingManager *instance();

private:
  enum class LogLevel { kInfo, kWarning, kError };
  struct LogEntry
  {
    LogLevel fLevel;
    std::string fMessage;
  };
  inline static int kMaxLogEntries = 100;

private:
  LoggingManager() = default;
  void logEntry(LogEntry iEntry);
  void renderLog();
  void renderDebug();

private:
  bool fShowDebug{false};
  bool fShowLog{false};
  bool fScrollLog{false};
  std::map<std::string, std::string> fDebug{};
  std::vector<LogEntry> fLog{};
};

//------------------------------------------------------------------------
// LoggingManager::debug
//------------------------------------------------------------------------
template<typename... Args>
void LoggingManager::debug(std::string const &iKey, std::string const &format, Args... args)
{
  debug(iKey, re::mock::fmt::printf(format, args...));
}

//------------------------------------------------------------------------
// LoggingManager::logInfo
//------------------------------------------------------------------------
template<typename... Args>
void LoggingManager::logInfo(std::string const &format, Args... args)
{
  logInfo(re::mock::fmt::printf(format, args...));
}

//------------------------------------------------------------------------
// LoggingManager::logWarning
//------------------------------------------------------------------------
template<typename... Args>
void LoggingManager::logWarning(std::string const &format, Args... args)
{
  logWarning(re::mock::fmt::printf(format, args...));
}

//------------------------------------------------------------------------
// LoggingManager::logError
//------------------------------------------------------------------------
template<typename... Args>
void LoggingManager::logError(std::string const &format, Args... args)
{
  logError(re::mock::fmt::printf(format, args...));
}


}

#endif //RE_EDIT_LOGGING_MANAGER_H