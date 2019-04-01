//
//    Created: 2019/03/31 11:02
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "File.hpp"

using namespace logReader;

File::File(const char *filePath)
    : m_file(CreateFile(filePath,
                        GENERIC_READ,
                        0,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr)) {
  if (m_file == INVALID_HANDLE_VALUE) {
    return;
  }
  LARGE_INTEGER size;
  if (!GetFileSizeEx(m_file, &size)) {
    Close();
    return;
  }
  m_size = size.QuadPart;
  m_mapping = CreateFileMapping(m_file, nullptr, PAGE_READONLY, size.u.HighPart,
                                size.u.LowPart, nullptr);
  if (!m_mapping) {
    Close();
    return;
  }
  m_view = static_cast<const char *>(
      MapViewOfFile(m_mapping, FILE_MAP_READ, 0, 0, 0));
  if (!m_view) {
    Close();
    return;
  }
}

File::~File() { Close(); }

void File::Close() {
  if (m_view) {
    UnmapViewOfFile(m_view);
    m_view = nullptr;
  }
  if (m_mapping) {
    CloseHandle(m_mapping);
    m_mapping = nullptr;
  }
  if (m_file != INVALID_HANDLE_VALUE) {
    CloseHandle(m_file);
    m_file = INVALID_HANDLE_VALUE;
  }
}

bool File::IsOk() const { return m_file != INVALID_HANDLE_VALUE; }

bool File::ReadRecord(const char *&begin, const char *&end) {
  if (!IsOk()) {
    return false;
  }
  assert(m_pos <= m_size);
  if (m_pos >= m_size) {
    Close();
    return false;
  }

  auto isStarted = false;
  for (; m_pos < m_size; ++m_pos) {
    const auto &ch = m_view[m_pos];
    if (ch == '\r' || ch == '\n') {
      if (!isStarted) {
        continue;
      }
      end = &ch;
      return true;
    }
    if (!isStarted) {
      begin = &ch;
      isStarted = true;
    }
  }
  if (!isStarted) {
    Close();
  }
  return isStarted;
}
