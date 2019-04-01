//
//    Created: 2019/03/31 10:57
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

namespace logReader {

//! File provides an access to a file of log.
class File {
 public:
  explicit File(const char *filePath);
  File(File &&) = default;
  File(const File &) = delete;
  File &operator=(File &&) = delete;
  File &operator=(const File &) = delete;
  ~File();

  explicit operator bool() const { return IsOk(); }

  //! Close closes a file.
  /*
   * Does nothing if the file is not opened.
   */
  void Close();

  //! IsOk returns true if the file is opened and reading position is not at the
  //! end.
  bool IsOk() const;

  //! ReadRecord reads the next record.
  /**
   * @param[out] begin At success returns string begin.
   * @param[out] end At success returns string end.
   * @sa IsOk
   * @return True at success, false otherwise.
   */
  bool ReadRecord(const char *&begin, const char *&end);

 private:
  void *m_file;
  void *m_mapping{nullptr};
  const char *m_view{nullptr};
  size_t m_pos = 0;
  size_t m_size;
};

}  // namespace logReader
