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

#ifndef RE_EDIT_UTILS_H
#define RE_EDIT_UTILS_H

#include <memory>

namespace re::edit::Utils {

template<typename F>
class DeferrableAction
{
public:
  explicit DeferrableAction(F iAction) : fAction{std::move(iAction)} {}
  DeferrableAction(DeferrableAction const &) = delete;
  DeferrableAction &operator=(DeferrableAction const &) = delete;
  ~DeferrableAction() { fAction(); }
private:
  F fAction{};
};

template<typename F>
[[nodiscard]] inline std::unique_ptr<DeferrableAction<F>> defer(F iAction) { return std::make_unique<DeferrableAction<F>>(std::move(iAction)); }

template<typename T>
struct StorageRAII
{
  explicit StorageRAII(T **iStorage, T *iValue) : fStorage{iStorage}, fPrevious{*iStorage} { *fStorage = iValue; }
  StorageRAII(StorageRAII &&) = delete;
  StorageRAII(StorageRAII const &) = delete;
  ~StorageRAII() { *fStorage = fPrevious; }
private:
  T **fStorage;
  T *fPrevious;
};

}

#endif //RE_EDIT_UTILS_H
