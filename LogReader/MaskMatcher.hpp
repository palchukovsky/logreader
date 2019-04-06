//
//    Created: 2019/03/30 16:09
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

#include "Rules.hpp"

namespace logReader {

//! MaskMatcher checks a string for a given mask.
/**
 * @sa Compile.
 */
class MaskMatcher {
  struct RuleSet;

 public:
  //! C-tor creates matcher with empty compiled mask (like Compile("")).
  /*
   * @sa Compile.
   */
  MaskMatcher() = default;
  MaskMatcher(MaskMatcher &&) = default;
  MaskMatcher(const MaskMatcher &) = delete;
  MaskMatcher &operator=(MaskMatcher &&) = delete;
  MaskMatcher &operator=(const MaskMatcher &) = delete;
  ~MaskMatcher();

  //! Compile compiles the mask expression and replaces it if previous is
  //! existent.
  /*
   * Accepts string with fixed string blocks and the next mask special symbols:
   *   ? - Block can have one any symbol or can be empty.
   *   * - Block can have several any symbols or can be empty.
   * Use slash before special symbols to find special symbols.
   *
   * Example: "abc?abc\*abs*" to match strings "abcXabc*absX" and "abcabc*abs"
   *
   * @return True at success, false is compilation is failed (previous state
   * still be active).
   */
  bool Compile(const char *mask);

  //! AddRule adds a new rule to the end of the rule sequence.
  template <typename Rule, typename... Args>
  bool AddRule(Args... args) {
    return m_rules.AddRule<Rule>(args...);
  }

  //! Match checks is connect matches to compiled mask or not.
  /**
   * @param[in] begin Content begin.
   * @param[in] end Content end.
   * @return True if content matches, false otherwise.
   */
  bool Match(const char *begin, const char *end) const;

 private:
  //! Match tries to match branch from "begin" for rule set started from "rule".
  /**
   * @param[in,out] rule First rule in sequence. Returns stop-rule, may be out
   * of rule set range if the rule is the last.
   *
   * @param[in,out] begin Branch start. Returns next branch begin.
   *
   * @param[in] strictBegin The last position where actual first field can
   * start.
   *
   * @param[in] end Content end.
   *
   * @return True if content matches, false otherwise.
   */
  bool Match(size_t &rule,
             const char *&begin,
             const char *strictBegin,
             const char *end) const;

  //! Check checks the first rule, continues to check if it is matched.
  /**
   * @param[in] rule First rule in sequence.
   *
   * @param[in,out] begin Branch start. Returns next branch begin.
   *
   * @param[in] strictBegin The last position where actual first field can
   * start.
   *
   * @param[in] end Content end.
   *
   * @return Stop-rule, may be out of rule set range if the rule is the last.
   */
  size_t CheckRule(size_t rule,
                   const char *&begin,
                   const char *strictBegin,
                   const char *end) const;

  struct RuleSet {
    size_t size = 0;
    Rule **set{nullptr};

    template <typename Rule, typename... Args>
    bool AddRule(Args...);
    template <typename Rule, typename... Args>
    bool ReplaceRule(size_t, Args...);
    void CleanUp();
  } m_rules;
};

template <typename RuleImpl, typename... Args>
bool MaskMatcher::RuleSet::AddRule(Args... args) {
  const auto rule = static_cast<Rule *>(malloc(sizeof(RuleImpl)));
  if (!rule) {
    return false;
  }
  new (rule) RuleImpl(args...);

  struct Scope {  // NOLINT
    Rule *rule;
    ~Scope() {
      if (!rule) {
        return;
      }
      rule->~Rule();
      free(rule);
    }
  } scope{rule};

  if (rule->HasError()) {
    return false;
  }

  if (!set) {
    assert(size == 0);
    set = static_cast<Rule **>(malloc(sizeof(Rule *)));
    if (!set) {
      return false;
    }
    size = 1;
  } else {
    assert(size > 0);
    const auto newSet =
        static_cast<Rule **>(realloc(set, (size + 1) * sizeof(Rule **)));
    if (!newSet) {
      return false;
    }
    set = newSet;
    ++size;
  }

  scope.rule = nullptr;
  set[size - 1] = rule;
  return true;
}

template <typename RuleImpl, typename... Args>
bool MaskMatcher::RuleSet::ReplaceRule(const size_t index, Args... args) {
  if (index >= size) {
    return false;
  }
  const auto rule = static_cast<Rule *>(malloc(sizeof(RuleImpl)));
  if (!rule) {
    return false;
  }
  new (rule) RuleImpl(args...);
  if (rule->HasError()) {
    rule->~Rule();
    // ReSharper disable once CppNonConsistentAcquisitionReclaimPair
    free(rule);
    return false;
  }
  set[index]->~Rule();
  set[index] = rule;
  return true;
}

}  // namespace logReader
