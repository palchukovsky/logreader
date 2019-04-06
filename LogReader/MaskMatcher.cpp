//
//    Created: 2019/03/30 16:14
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "MaskMatcher.hpp"
#include "Rules.hpp"

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
      // borders. Starting to check next rule from this field end (rule's
      // written it in begin).
      assert(begin <= fieldEnd);
      return rule + 1;

    case Rule::RESULT_COMPLETED_GREEDY: {
      // The current rule is greedy so it has to check each possible branch to
      // check with requirements of this greedy rule.
      if (rule + 1 >= m_rules.size) {
        // As this is greedy and last rule - it passed for sure.
        begin = fieldEnd;
        return rule + 1;
      }
      for (;;) {
        const auto it = begin;
        auto nextRuleId = rule + 1;
        if (Match(nextRuleId, begin, fieldEnd, end)) {
          begin = end;
          return m_rules.size;
        }
        if (it == begin || begin >= fieldEnd) {
          // No one next rule has found something in this branch from the start.
          // Negative results too. It means there are no chances to find
          // something in the rest.
          return false;
        }
        // It has completed branch, but the rule set is over not at the symbol
        // sequence end.It means one or more greedy rules don't eat enough.
        // Checking each branch that starts in the current field "strict begin".
        // New begin has begin for the next field after the last successful.
        // Continue.
      }
    }

    default:
      assert(false);
    case Rule::RESULT_FAILED:
      // Branch is completed with error.
      return 0;
  }
}

bool MaskMatcher::Match(size_t &rule,
                        const char *&begin,
                        const char *strictBegin,
                        const char *end) const {
  assert(rule < m_rules.size);
  assert(begin <= strictBegin);
  assert(strictBegin <= end);
  for (;;) {
    rule = CheckRule(rule, begin, strictBegin, end);
    if (rule >= m_rules.size) {
      assert(begin <= end);
      return begin == end;
    }
    if (!rule) {
      return false;
    }
    strictBegin = begin;
  }
}

bool MaskMatcher::Match(const char *begin, const char *end) const {
  assert(!m_rules.set || m_rules.size > 0);
  assert(m_rules.set || m_rules.size == 0);
  if (!m_rules.set) {
    // Empty rule set (like mask with empty string) means "only empty
    // string matches".
    return begin == end;
  }
  for (size_t i = 0; i < m_rules.size; ++i) {
    m_rules.set[i]->Reset();
  }
  size_t rule = 0;
  return Match(rule, begin, begin, end);
}
