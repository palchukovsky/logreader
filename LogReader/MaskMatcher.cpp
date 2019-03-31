//
//    Created: 2019/03/30 16:14
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "MaskMatcher.hpp"

using namespace logReader;

void MaskMatcher::RuleSet::CleanUp() {
  assert(!set || size > 0);
  assert(set || size == 0);
  for (size_t i = 0; i < size; ++i) {
    set[i]->~Rule();
  }
  free(set);
  set = nullptr;
  size = 0;
}

MaskMatcher::AnySymbolWithLen0OrNRule::AnySymbolWithLen0OrNRule(
    const size_t maxLen)
    : m_maxLen(maxLen) {}

bool MaskMatcher::AnySymbolWithLen0OrNRule::HasError() const { return false; }

MaskMatcher::AnySymbolWithLen0OrNRule::Result
MaskMatcher::AnySymbolWithLen0OrNRule::Check(const char *&begin,
                                             const char *,
                                             const char *&end) const {
  assert(begin <= end);
  if (begin > end) {
    return RESULT_FAILED;
  }
  if (begin == end) {
    return RESULT_COMPLETED_FULL;
  }
  const auto fieldEnd = begin + m_maxLen;
  if (fieldEnd < end) {
    end = fieldEnd;
  }
  return RESULT_COMPLETED_GREEDY;
}

bool MaskMatcher::AnySymbolWithLen0OrMoreRule::HasError() const {
  return false;
}

MaskMatcher::AnySymbolWithLen0OrMoreRule::Result
MaskMatcher::AnySymbolWithLen0OrMoreRule::Check(const char *&begin,
                                                const char *,
                                                const char *&end) const {
  assert(begin <= end);
  if (begin > end) {
    return RESULT_FAILED;
  }
  if (begin == end) {
    return RESULT_COMPLETED_FULL;
  }
  return RESULT_COMPLETED_GREEDY;
}

MaskMatcher::FixedStringRule::FixedStringRule(const char *begin,
                                              const char *end)
    : m_len(static_cast<size_t>(end - begin)),
      m_template(static_cast<char *>(malloc(m_len * sizeof(char)))) {
  assert(begin <= end);
  if (!m_template) {
    return;
  }
  memcpy(m_template, begin, m_len * sizeof(char));
}
MaskMatcher::FixedStringRule::~FixedStringRule() { free(m_template); }

bool MaskMatcher::FixedStringRule::HasError() const {
  return m_template == nullptr;
}

MaskMatcher::FixedStringRule::Result MaskMatcher::FixedStringRule::Check(
    const char *&begin, const char *strictBegin, const char *&end) const {
  assert(begin <= end);
  if (begin > end) {
    return RESULT_FAILED;
  }
  for (auto it = begin; static_cast<size_t>(end - it) >= m_len; ++it) {
    size_t i = 0;
    do {
      if (it[i] != m_template[i]) {
        break;
      }
    } while (++i < m_len);
    if (i == m_len) {
      begin = end = it + m_len;
      return RESULT_COMPLETED_FULL;
    }
    if (strictBegin <= it) {
      break;
    }
  }
  return RESULT_FAILED;
}

bool MaskMatcher::Compile(const char *mask) {
  if (!mask) {
    return false;
  }

  const auto maskLen = strlen(mask);
  if (maskLen == 0) {
    m_rules.CleanUp();
    return true;
  }

  struct Scope {  // NOLINT
    char *stingBuffer;
    RuleSet rules{};
    ~Scope() {
      rules.CleanUp();
      free(stingBuffer);
    }
    // ReSharper disable once CppInitializedValueIsAlwaysRewritten
  } scope{static_cast<char *>(malloc(maskLen * sizeof(char)))};

  if (!scope.stingBuffer) {
    return false;
  }
  size_t stingSize = 0;
  const auto &continueString = [&scope, &stingSize](const char *it) {
    scope.stingBuffer[stingSize++] = *it;
  };
  const auto &completePrev = [&scope, &stingSize]() {
    if (!stingSize) {
      return true;
    }
    if (!scope.rules.AddRule<FixedStringRule>(scope.stingBuffer,
                                              scope.stingBuffer + stingSize)) {
      return false;
    }
    stingSize = 0;
    return true;
  };

  auto isDisabled = false;
  char prev = 0;
  size_t len = 0;
  for (auto it = mask; *it; ++it) {
    const auto isDisabledIt = isDisabled;
    isDisabled = false;
    switch (*it) {
      case '\\':
        len = 0;
        if (!isDisabledIt) {
          isDisabled = true;
        } else {
          continueString(it);
        }
        break;

      case '?':
        if (isDisabledIt) {
          len = 0;
          continueString(it);
        } else if (prev != '*') {
          if (!completePrev()) {
            return false;
          }
          ++len;
          if (prev == *it) {
            assert(scope.rules.size > 0);
            if (!scope.rules.ReplaceRule<AnySymbolWithLen0OrNRule>(
                    scope.rules.size - 1, len)) {
              return false;
            }
          } else if (!scope.rules.AddRule<AnySymbolWithLen0OrNRule>(1)) {
            return false;
          }
        }
        break;

      case '*':
        len = 0;
        if (isDisabledIt) {
          continueString(it);
        } else if (prev == '?') {
          assert(scope.rules.size > 0);
          if (!scope.rules.ReplaceRule<AnySymbolWithLen0OrMoreRule>(
                  scope.rules.size - 1)) {
            return false;
          }
        } else if (prev != *it &&
                   (!completePrev() ||
                    !scope.rules.AddRule<AnySymbolWithLen0OrMoreRule>())) {
          return false;
        }
        break;

      default:
        continueString(it);
        break;
    }
    prev = isDisabledIt ? 0 : *it;
  }
  if (!completePrev()) {
    return false;
  }

  const auto tmp = m_rules;
  m_rules = scope.rules;
  scope.rules = tmp;
  return true;
}

MaskMatcher::~MaskMatcher() { m_rules.CleanUp(); }

size_t MaskMatcher::CheckRule(const size_t rule,
                              const char *&begin,
                              const char *strictBegin,
                              const char *end) const {
  auto fieldEnd = end;
  assert(rule < m_rules.size);
  static_assert(Rule::numberOfResults == 3, "List changed.");
  switch (m_rules.set[rule]->Check(begin, strictBegin, fieldEnd)) {
    case Rule::RESULT_COMPLETED_FULL:
      // The current rule is very simple and it completed with the fixed field
      // borders. Starting to check next rule from this field end.
      assert(begin <= fieldEnd);
      assert(fieldEnd <= end);
      begin = fieldEnd;
      return rule + 1;

    case Rule::RESULT_COMPLETED_GREEDY: {
      // The current rule is greedy so it has to check each possible branch to
      // specify requirements of this greedy rule.
      const auto nextRuleId = rule + 1;
      if (nextRuleId >= m_rules.size) {
        // As this is greedy and last rule - it passed for sure.
        begin = fieldEnd;
        return nextRuleId;
      }
      for (;;) {
        auto it = begin;
        if (Match(nextRuleId, it, fieldEnd, end)) {
          begin = end;
          return m_rules.size;
        }
        if (it == begin || ++begin > fieldEnd) {
          return false;
        }
        // Means match completed successfully, but it's not the end of a
        // sequence. It has to continue tests in this branch.
      }
    }

    default:
      assert(false);
    case Rule::RESULT_FAILED:
      // Branch is completed with error.
      return 0;
  }
}

bool MaskMatcher::Match(size_t rule,
                        const char *&begin,
                        const char *strictBegin,
                        const char *end) const {
  assert(rule < m_rules.size);
  do {
    rule = CheckRule(rule, begin, strictBegin, end);
    if (!rule) {
      return false;
    }
    strictBegin = begin;
  } while (rule < m_rules.size);
  assert(begin <= end);
  return begin == end;
}

bool MaskMatcher::Match(const char *begin, const char *end) const {
  assert(!m_rules.set || m_rules.size > 0);
  assert(m_rules.set || m_rules.size == 0);
  if (!m_rules.set) {
    // Empty rule set (like mask with empty string) means "only empty
    // string matches".
    return begin == end;
  }
  return Match(0, begin, begin, end);
}
