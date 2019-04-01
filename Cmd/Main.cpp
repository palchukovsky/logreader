//
//    Created: 2019/04/01 01:43
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "LogReader/LogReader.hpp"

namespace {
void PrintHelp(const char *exec) {
  printf(R"(
Usage:
  %s "mask" "log file path"

Accepts string with fixed string blocks and the next mask special symbols:
  ? - Block can have one any symbol or can be empty.
  * - Block can have several any symbols or can be empty.
Use slash before special symbols to find special symbols.

Example to match strings "abcXabc*absX" and "abcabc*abs":
  %s "abc?abc\*abs*" debug.log 

)",
         exec, exec);
}
}  // namespace

int main(const int argc, const char *argv[]) {
  if (argc < 1) {
    return 1;
  }
  const auto exec = argv[0];
  if (argc != 3) {
    PrintHelp(exec);
    return 1;
  }
  const auto mask = argv[1];
  const auto filePath = argv[2];

  LogReader reader;
  if (!reader.Open(filePath)) {
    printf(R"(Filed to open file \"%s\".\n)", filePath);
    return 1;
  }
  if (!reader.SetFilter(mask)) {
    printf(R"(Failed to parse mask "%s".\n)", mask);
    PrintHelp(exec);
    return 1;
  }

  char buffer[1024 * 10];
  while (reader.GetNextLine(&buffer[0], sizeof(buffer))) {
    printf("%s\n", &buffer[0]);
  }

  return 0;
}  // namespace
