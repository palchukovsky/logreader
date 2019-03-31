//
//    Created: 2019/03/30 15:51
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

namespace logReader {

//! Matcher describes an interface that answers the question does content meet
//! the interface implementation requirements.
class Matcher {
 public:
  Matcher() = default;
  Matcher(Matcher &&) = default;
  Matcher(const Matcher &) = default;
  Matcher &operator=(Matcher &&) = default;
  Matcher &operator=(const Matcher &) = default;
  virtual ~Matcher() = default;

  //! Match performs matching check for a symbol string.
  /**
   * @retunr True if a string is matched, false otherwise.
   */
  virtual bool Match(const char *begin, const char *end) const = 0;
};

}  // namespace logReader
