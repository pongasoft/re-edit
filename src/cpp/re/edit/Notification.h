/*
 * Copyright (c) 2023 pongasoft
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

#ifndef RE_EDIT_NOTIFICATION_H
#define RE_EDIT_NOTIFICATION_H

#include <functional>
#include <vector>
#include <string>
#include <chrono>
#include <optional>

namespace re::edit::ReGui {

class Notification
{
public:
  struct Key {
    void *fKey{};

    constexpr void reset() { fKey = nullptr; }
    constexpr bool empty() const { return fKey == nullptr; }

    constexpr bool operator==(Key const &rhs) const { return fKey == rhs.fKey; }
    constexpr bool operator!=(Key const &rhs) const { return !(rhs == *this); }

    constexpr static Key none() { return {}; }
    constexpr static Key from(void *iKey) { return { iKey }; }
  };

  class Content
  {
  public:
    virtual ~Content() = default;
    /**
     * @return `false` when render determines that the notification should be dismissed */
    virtual bool render() = 0;
  };

  class LamdaContent : public Content
  {
  public:
    bool render() override;

  public:
    std::function<bool()> fLambda{};
  };

  void dismiss() { fActive = false; }
  constexpr bool isActive() const { return fActive; }
  constexpr Key const &key() { return fKey; }

public:
  explicit Notification(Key const &iKey = Key::none());

  /**
   * @return `true` if the notification (= ImGui window) was rendered, `false` otherwise */
  bool render();

  Notification &text(std::string iText);
  Notification &lambda(std::function<bool()> iLambda);
  template<class Rep, class Period = std::ratio<1>>
  Notification &dismissAfter(std::chrono::duration<Rep, Period> const &iDuration);

private:
  std::vector<std::unique_ptr<Content>> fContent{};
  bool fActive{true};

  Key fKey;
  std::string fWindowName;
  std::optional<decltype(std::chrono::system_clock::now())> fDismissTime{};
  inline static long kIota = 1;
};

//------------------------------------------------------------------------
// Notification::dismissAfter
//------------------------------------------------------------------------
template<class Rep, class Period>
Notification &Notification::dismissAfter(std::chrono::duration<Rep, Period> const &iDuration)
{
  fDismissTime = std::chrono::system_clock::now() + iDuration;
  return *this;
}

}

#endif //RE_EDIT_NOTIFICATION_H