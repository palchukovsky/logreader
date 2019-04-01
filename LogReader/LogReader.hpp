//
//    Created: 2019/04/01 00:47
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#pragma once

//! LogReader implements log records reading.
class LogReader {
 public:
  LogReader();
  LogReader(LogReader &&) = default;
  LogReader(const LogReader &) = delete;
  LogReader &operator=(LogReader &&) = delete;
  LogReader &operator=(const LogReader &) = delete;
  ~LogReader();

  //! Opens file of log. Returns false at error or if file is already opened.
  bool Open(const char *filePath);
  //! Closes file and resets filter. Does nothing if file is not open or filter
  //! is not set.
  void Close();

  //! Sets records filter for log record.
  /**
   * Accepts string with fixed string blocks and the next mask special symbols:
   *   ? - Block can have one any symbol or can be empty.
   *   * - Block can have several any symbols or can be empty.
   * Use slash before special symbols to find special symbols.
   *
   * Example: "abc?abc\*abs*" to match strings "abcXabc*absX" and "abcabc*abs"
   * By default filter is empty and any string will be extracted.
   *
   *  @return True at success, false at error.
   */
  bool SetFilter(const char *);

  //! Returns next (or first) record of log, that corresponds by the provided
  //! filter.
  /**
   * @params[out] buffer Address to a buffer for request error.
   *
   * @param [in] bufferSize Result buffer size. If the buffer is too small - the
   * record will be truncated by provided size.
   *
   * @se SetFilter
   *
   * @return True if record successfully extracted. False if there are no more
   * records or if an error has occurred.
   */
  bool GetNextLine(char *buffer, int bufferSize);

 private:
  class Implementation;
  Implementation *m_pimpl = nullptr;
};
