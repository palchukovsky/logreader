//
//    Created: 2019/03/30 16:09
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

#include "Matcher.hpp"

namespace logReader {

//! MaskMatcher checks a string for a given mask.
/**
 * @sa Compile.
 */
class MaskMatcher final : public Matcher {
  struct RuleSet;

 public:
  //! Rule describes one rule in a expression.
  class Rule {
   public:
    enum Result {
      //! Failed.
      RESULT_FAILED,
      //! Fully completed with success.
      RESULT_COMPLETED_FULL,
      //! Completed with success, but maybe continued, if required (rule is
      //! "greedy" from this place).
      RESULT_COMPLETED_GREEDY,
      numberOfResults
    };

    Rule() = default;
    Rule(Rule &&) = default;
    Rule(const Rule &) = default;
    Rule &operator=(Rule &&) = default;
    Rule &operator=(const Rule &) = default;
    virtual ~Rule() = default;

    //! HasError returns true if rule initialized with error.
    virtual bool HasError() const = 0;

    //! Check checks sequence of symbols.
    /**
     * @param[in,out] begin Content begin. If rule is not failed call will
     * write next field begin.
     *
     * @param[in] strictBegin The last position where actual field can start.
     *
     * @param[in,out] end Content end. If rule is not failed call will
     * write field end.
     */
    virtual Result Check(const char *&begin,
                         const char *strictBegin,
                         const char *&end) const = 0;
  };

  //! AnySymbolWithLen0OrMoreRule implements the rule "block can have several
  //! any symbols, or can be empty".
  class AnySymbolWithLen0OrMoreRule final : public Rule {
   public:
    AnySymbolWithLen0OrMoreRule() = default;
    AnySymbolWithLen0OrMoreRule(AnySymbolWithLen0OrMoreRule &&) = default;
    AnySymbolWithLen0OrMoreRule(const AnySymbolWithLen0OrMoreRule &) = default;
    AnySymbolWithLen0OrMoreRule &operator=(AnySymbolWithLen0OrMoreRule &&) =
        default;
    AnySymbolWithLen0OrMoreRule &operator=(
        const AnySymbolWithLen0OrMoreRule &) = default;
    ~AnySymbolWithLen0OrMoreRule() override = default;
    bool HasError() const override;
    Result Check(const char *&, const char *, const char *&) const override;
  };

  //! AnySymbolWithLen0OrNRule implements the rule "block can have up to N
  //! any symbols, or can be empty".
  class AnySymbolWithLen0OrNRule final : public Rule {
   public:
    explicit AnySymbolWithLen0OrNRule(size_t maxLen);
    AnySymbolWithLen0OrNRule(AnySymbolWithLen0OrNRule &&) = default;
    AnySymbolWithLen0OrNRule(const AnySymbolWithLen0OrNRule &) = default;
    AnySymbolWithLen0OrNRule &operator=(AnySymbolWithLen0OrNRule &&) = default;
    AnySymbolWithLen0OrNRule &operator=(const AnySymbolWithLen0OrNRule &) =
        default;
    ~AnySymbolWithLen0OrNRule() override = default;
    bool HasError() const override;
    Result Check(const char *&, const char *, const char *&) const override;

   private:
    size_t m_maxLen;
  };

  //! FixedStringRule implements the rule "block has to be equal to a fixed
  //! string".
  class FixedStringRule final : public Rule {
   public:
    explicit FixedStringRule(const char *begin, const char *end);
    FixedStringRule(FixedStringRule &&) = default;
    FixedStringRule(const FixedStringRule &) = delete;
    FixedStringRule &operator=(FixedStringRule &&) = delete;
    FixedStringRule &operator=(const FixedStringRule &) = delete;
    ~FixedStringRule() override;
    bool HasError() const override;
    Result Check(const char *&, const char *, const char *&) const override;

   private:
    const size_t m_len;
    char *m_template;
  };

  //! C-tor creates matcher with compiled mask (like Compile("")).
  /*
   * @sa Compile.
   */
  MaskMatcher() = default;
  MaskMatcher(MaskMatcher &&) = default;
  MaskMatcher(const MaskMatcher &) = delete;
  MaskMatcher &operator=(MaskMatcher &&) = delete;
  MaskMatcher &operator=(const MaskMatcher &) = delete;
  ~MaskMatcher() override;

  //! Compile compiles the mask expression and replaces it, if previous is
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

  bool Match(const char *begin, const char *end) const override;

 private:
  bool Match(size_t rule,
             const char *&begin,
             const char *strictBegin,
             const char *end) const;

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
    free(rule);
    return false;
  }
  set[index]->~Rule();
  set[index] = rule;
  return true;
}

}  // namespace logReader
