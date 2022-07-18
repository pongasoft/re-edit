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

#ifndef RE_EDIT_ERRORS_H
#define RE_EDIT_ERRORS_H

#include <re/mock/Errors.h>
#include "LoggingManager.h"

namespace re::edit {

//! log_info
template<typename ... Args>
void log_info(char const *iFile, int iLine, const std::string &format, Args ... args)
{
  re::mock::log_info(iFile, iLine, format, args...);
  LoggingManager::instance()->logInfo(format, std::forward<Args>(args)...);
}

//! log_warning
template<typename ... Args>
void log_warning(char const *iFile, int iLine, const std::string &format, Args ... args)
{
  re::mock::log_warning(iFile, iLine, format, args...);
  LoggingManager::instance()->logWarning(format, std::forward<Args>(args)...);
  LoggingManager::instance()->showLog();
}

//! log_error
template<typename ... Args>
void log_error(char const *iFile, int iLine, const std::string &format, Args ... args)
{
  re::mock::log_error(iFile, iLine, format, args...);
  LoggingManager::instance()->logError(format, std::forward<Args>(args)...);
  LoggingManager::instance()->showLog();
}

}

#define RE_EDIT_ASSERT RE_MOCK_ASSERT
#define RE_EDIT_FAIL RE_MOCK_FAIL
#define RE_EDIT_TBD RE_MOCK_TBD
#define RE_EDIT_INTERNAL_ASSERT RE_MOCK_INTERNAL_ASSERT
#define RE_EDIT_LOG_INFO(...) re::edit::log_info(__FILE__, __LINE__, __VA_ARGS__)
#define RE_EDIT_LOG_WARNING(...) re::edit::log_warning(__FILE__, __LINE__, __VA_ARGS__)
#define RE_EDIT_LOG_ERROR(...) re::edit::log_error(__FILE__, __LINE__, __VA_ARGS__)

#endif //RE_EDIT_ERRORS_H
