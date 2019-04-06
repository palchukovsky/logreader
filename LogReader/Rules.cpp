//
//    Created: 2019/04/06 13:07
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "Rules.hpp"

using namespace logReader;

AnySymbolWithLen0OrNRule::AnySymbolWithLen0OrNRule(const size_t maxLen)
    : m_maxLen(maxLen) {}

bool AnySymbolWithLen0OrNRule::HasError() const { return false; }

AnySymbolWithLen0OrNRule::Result AnySymbolWithLen0OrNRule::Check(
    const char *&begin, const char *, const char *&end) {
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

bool AnySymbolWithLen0OrMoreRule::HasError() const { return false; }

AnySymbolWithLen0OrMoreRule::Result AnySymbolWithLen0OrMoreRule::Check(
    const char *&begin, const char *, const char *&end) {
  assert(begin <= end);
  if (begin > end) {
    return RESULT_FAILED;
  }
  if (begin == end) {
    return RESULT_COMPLETED_FULL;
  }
  return RESULT_COMPLETED_GREEDY;
}

FixedStringRule::FixedStringRule(const char *begin, const char *end)
    : m_len(static_cast<size_t>(end - begin)),
      m_template(static_cast<char *>(malloc(m_len * sizeof(char)))) {
  assert(begin <= end);
  if (!m_template) {
    return;
  }
  memcpy(m_template, begin, m_len * sizeof(char));
}
FixedStringRule::~FixedStringRule() { free(m_template); }

bool FixedStringRule::HasError() const { return m_template == nullptr; }

FixedStringRule::Result FixedStringRule::Check(const char *&begin,
                                               const char *strictBegin,
                                               const char *&end) {
  assert(begin <= end);
  if (begin > end) {
    return RESULT_FAILED;
  }

  if (m_lastPos && begin < m_lastPos) {
    if (m_lastResult == RESULT_COMPLETED_FULL) {
      // To don't search position that was already found before in another
      // branch.
      begin = end = m_lastPos;
      return RESULT_COMPLETED_FULL;
    }
    assert(m_lastResult == RESULT_FAILED);
    // To search from place that was not investigated yet.
    begin = m_lastPos;
  }

  for (; static_cast<size_t>(end - begin) >= m_len; ++begin) {
    size_t i = 0;
    do {
      if (begin[i] != m_template[i]) {
        break;
      }
    } while (++i < m_len);
    if (i == m_len) {
      m_lastPos = begin = end = begin + m_len;
      m_lastResult = RESULT_COMPLETED_FULL;
      return RESULT_COMPLETED_FULL;
    }
    if (strictBegin <= begin) {
      break;
    }
  }

  m_lastPos = begin;
  m_lastResult = RESULT_FAILED;
  return RESULT_FAILED;
}

void FixedStringRule::Reset() { m_lastPos = nullptr; }
