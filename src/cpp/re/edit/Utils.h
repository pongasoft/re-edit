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
#include <mutex>
#include <thread>
#include <algorithm>

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
[[nodiscard]] inline DeferrableAction<F> defer(F iAction) { return DeferrableAction<F>(std::move(iAction)); }

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

class Cancellable
{
public:
  class cancelled_t : public std::exception {};

public:
  bool cancelled() const
  {
    std::lock_guard<std::mutex> lock(fMutex);
    return fCancelled;
  }

  void cancel()
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fCancelled = true;
  }

  void progress(std::string const &s)
  {
//    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> lock(fMutex);
    if(fCancelled)
      throw cancelled_t();
    fProgress = s;
    fCount++;
  }

  std::pair<int, std::string> progress() const
  {
    std::lock_guard<std::mutex> lock(fMutex);
    return {fCount, fProgress};
  }

private:
  mutable std::mutex fMutex;
  bool fCancelled{};
  int fCount{};
  std::string fProgress{};
};

using CancellableSPtr = std::shared_ptr<Cancellable>;

//------------------------------------------------------------------------
// str_tolower
//------------------------------------------------------------------------
inline std::string str_tolower(std::string const &s) {
  auto s2 = s;
  std::transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c){ return std::tolower(c); });
  return s2;
}

/**
 * Make sure that the value remains within its bounds
 *
 * @param iValue the value to clamp between `iLower` and `iUpper`
 * @param iLower the lower bound (must be <= iUpper)
 * @param iUpper the upper bound (must be >= iLower)
 */
template <typename T, typename U>
inline static T clamp(const U &iValue, const T &iLower, const T &iUpper)
{
  auto v = static_cast<T>(iValue);
  return v < iLower ? iLower : (v > iUpper ? iUpper : v);
}

}

#endif //RE_EDIT_UTILS_H
