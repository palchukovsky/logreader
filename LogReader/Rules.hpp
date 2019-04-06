//
//    Created: 2019/04/06 13:07
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

namespace logReader {

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
   * A rule can change own state inside one match operation. Method Reset will
   * be called before the next match operation will be stared.
   *
   * @sa Reset
   *
   * @param[in,out] begin Accepts content begin, returns next field begin.
   *
   * @param[in] strictBegin The last position where actual field can start.
   *
   * @param[in,out] end Content end. Returns current field end.
   */
  virtual Result Check(const char *&begin,
                       const char *strictBegin,
                       const char *&end) = 0;

  //! Reset prepares rule for the next check.
  virtual void Reset(){};
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
  AnySymbolWithLen0OrMoreRule &operator=(const AnySymbolWithLen0OrMoreRule &) =
      default;
  ~AnySymbolWithLen0OrMoreRule() override = default;
  bool HasError() const override;
  Result Check(const char *&, const char *, const char *&) override;
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
  Result Check(const char *&, const char *, const char *&) override;

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
  Result Check(const char *&, const char *, const char *&) override;
  void Reset() override;

 private:
  const size_t m_len;
  char *m_template;
  const char *m_lastPos = nullptr;
  Result m_lastResult = numberOfResults;
};

}  // namespace logReader
