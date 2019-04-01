//
//    Created: 2019/04/01 01:01
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "LogReader.hpp"
#include "File.hpp"
#include "MaskMatcher.hpp"

using namespace logReader;

class LogReader::Implementation {
 public:
  File *m_file = nullptr;
  MaskMatcher *m_matcher = nullptr;

  Implementation() = default;
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = delete;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() {
    if (m_matcher) {
      m_matcher->~MaskMatcher();
      free(m_matcher);
    }
    if (m_file) {
      m_file->~File();
      free(m_file);
    }
  }
};

LogReader::LogReader()
    : m_pimpl(static_cast<Implementation *>(malloc(sizeof(Implementation)))) {
  if (!m_pimpl) {
    return;
  }
  new (m_pimpl) Implementation();
}

LogReader::~LogReader() {
  if (m_pimpl) {
    m_pimpl->~Implementation();
    free(m_pimpl);
  }
}

bool LogReader::Open(const char *filePath) {
  if (!m_pimpl || m_pimpl->m_file) {
    return false;
  }
  auto file = static_cast<File *>(malloc(sizeof(File)));
  if (!file) {
    return false;
  }
  new (file) File(filePath);
  if (!*file) {
    file->~File();
    free(file);
    return false;
  }
  m_pimpl->m_file = file;
  return true;
}

void LogReader::Close() {
  if (!m_pimpl || !m_pimpl->m_file) {
    return;
  }
  m_pimpl->m_file->~File();
  free(m_pimpl->m_file);
  m_pimpl->m_file = nullptr;
}

bool LogReader::SetFilter(const char *filter) {
  if (!m_pimpl) {
    return false;
  }
  const auto has = m_pimpl->m_matcher != nullptr;
  if (!has) {
    m_pimpl->m_matcher =
        static_cast<MaskMatcher *>(malloc(sizeof(MaskMatcher)));
    if (!m_pimpl->m_matcher) {
      return false;
    }
    new (m_pimpl->m_matcher) MaskMatcher();
  }
  if (!m_pimpl->m_matcher->Compile(filter)) {
    if (!has) {
      m_pimpl->m_matcher->~MaskMatcher();
      free(m_pimpl->m_matcher);
      m_pimpl->m_matcher = nullptr;
    }
    return false;
  }
  return true;
}

bool LogReader::GetNextLine(char *buffer, const int bufferSize) {
  if (!m_pimpl || !m_pimpl->m_file || bufferSize < 1) {
    return false;
  }
  for (;;) {
    const char *begin;
    const char *end;
    if (!m_pimpl->m_file->ReadRecord(begin, end)) {
      return false;
    }
    if (m_pimpl->m_matcher && !m_pimpl->m_matcher->Match(begin, end)) {
      continue;
    }
    auto len = end - begin;
    if (len >= bufferSize) {
      len = bufferSize - 1;
    }
    memcpy(buffer, begin, len);
    buffer[len] = 0;
    return true;
  }
}
