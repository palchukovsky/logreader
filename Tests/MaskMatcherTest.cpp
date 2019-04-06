//
//    Created: 2019/03/30 16:16
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
//

#include "Prec.hpp"
#include "LogReader/MaskMatcher.hpp"

using namespace logReader;
using namespace testing;

namespace {
void TestMatch(const MaskMatcher &matcher, const char *string, bool result) {
  EXPECT_EQ(result, matcher.Match(string, string + strlen(string)));
}

}  // namespace

TEST(MaskMatcher, GeneralTaskRequirements1) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("*abc*"));
  TestMatch(matcher, "Abc", false);
  TestMatch(matcher, "xAbcx", false);
  TestMatch(matcher, "abc", true);
  TestMatch(matcher, "xabcx", true);
  TestMatch(matcher, "xabc", true);
  TestMatch(matcher, "abcx", true);
  TestMatch(matcher, "xxxabcxxx", true);
  TestMatch(matcher, "xxxabc", true);
  TestMatch(matcher, "abcxxx", true);
  TestMatch(matcher, "1abc1", true);
  TestMatch(matcher, "1abc", true);
  TestMatch(matcher, "abc1", true);
  TestMatch(matcher, "-abc-", true);
  TestMatch(matcher, "-abc", true);
  TestMatch(matcher, "abc-", true);
  TestMatch(matcher, " abc ", true);
  TestMatch(matcher, " abc", true);
  TestMatch(matcher, "abc ", true);
}
TEST(MaskMatcher, GeneralTaskRequirements2) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("abc*"));
  TestMatch(matcher, "Abc", false);
  TestMatch(matcher, "xAbcx", false);
  TestMatch(matcher, "abc", true);
  TestMatch(matcher, "xabcx", false);
  TestMatch(matcher, "xabc", false);
  TestMatch(matcher, "abcx", true);
  TestMatch(matcher, "xxxabcxxx", false);
  TestMatch(matcher, "xxxabc", false);
  TestMatch(matcher, "abcxxx", true);
  TestMatch(matcher, "1abc1", false);
  TestMatch(matcher, "1abc", false);
  TestMatch(matcher, "abc1", true);
  TestMatch(matcher, "-abc-", false);
  TestMatch(matcher, "-abc", false);
  TestMatch(matcher, "abc-", true);
  TestMatch(matcher, " abc ", false);
  TestMatch(matcher, " abc", false);
  TestMatch(matcher, "abc ", true);
}
TEST(MaskMatcher, GeneralTaskRequirements3) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("abc?"));
  TestMatch(matcher, "Abc", false);
  TestMatch(matcher, "xAbcx", false);
  TestMatch(matcher, "abc", true);
  TestMatch(matcher, "xabcx", false);
  TestMatch(matcher, "xabc", false);
  TestMatch(matcher, "abcx", true);
  TestMatch(matcher, "xxxabcxxx", false);
  TestMatch(matcher, "xxxabc", false);
  TestMatch(matcher, "abcxxx", false);
  TestMatch(matcher, "1abc1", false);
  TestMatch(matcher, "1abc", false);
  TestMatch(matcher, "abc1", true);
  TestMatch(matcher, "-abc-", false);
  TestMatch(matcher, "-abc", false);
  TestMatch(matcher, "abc-", true);
  TestMatch(matcher, " abc ", false);
  TestMatch(matcher, " abc", false);
  TestMatch(matcher, "abc ", true);
}
TEST(MaskMatcher, GeneralTaskRequirements4) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("abc"));
  TestMatch(matcher, "Abc", false);
  TestMatch(matcher, "xAbcx", false);
  TestMatch(matcher, "abc", true);
  TestMatch(matcher, "xabcx", false);
  TestMatch(matcher, "xabc", false);
  TestMatch(matcher, "abcx", false);
  TestMatch(matcher, "xxxabcxxx", false);
  TestMatch(matcher, "xxxabc", false);
  TestMatch(matcher, "abcxxx", false);
  TestMatch(matcher, "1abc1", false);
  TestMatch(matcher, "1abc", false);
  TestMatch(matcher, "abc1", false);
  TestMatch(matcher, "-abc-", false);
  TestMatch(matcher, "-abc", false);
  TestMatch(matcher, "abc-", false);
  TestMatch(matcher, " abc ", false);
  TestMatch(matcher, " abc", false);
  TestMatch(matcher, "abc ", false);
}
TEST(MaskMatcher, GeneralTaskRequirementsResult) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("order*closed"));
  TestMatch(matcher, "Orderclosed", false);
  TestMatch(matcher, "OrderXclosed", false);
  TestMatch(matcher, "orderclosed", true);
  TestMatch(matcher, "orderXclosed", true);
  TestMatch(matcher, "orderXXXclosed", true);
  TestMatch(matcher, "XorderXclosedX", false);
}

TEST(MaskMatcher, MagicAsterisk) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile(R"(test1\*test2*test3)"));
  TestMatch(matcher, "test1*test2Xtest3", true);
  TestMatch(matcher, "test1test2Xtest3", false);
  TestMatch(matcher, "test1*test2test3", true);

  ASSERT_TRUE(matcher.Compile(R"(test1\\\*test2\\*test3)"));
  TestMatch(matcher, R"(test1\*test2\Xtest3)", true);
  TestMatch(matcher, R"(test1test2\Xtest3)", false);
  TestMatch(matcher, R"(test1\*test2\test3)", true);
}

TEST(MaskMatcher, MagicQuestion) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile(R"(test1\?test2?test3)"));
  TestMatch(matcher, "test1?test2Xtest3", true);
  TestMatch(matcher, "test1test2Xtest3", false);
  TestMatch(matcher, "test1?test2test3", true);

  ASSERT_TRUE(matcher.Compile(R"(test1\\\?test2\\?test3)"));
  TestMatch(matcher, R"(test1\?test2\Xtest3)", true);
  TestMatch(matcher, R"(test1\test2\Xtest3)", false);
  TestMatch(matcher, R"(test1\?test2\test3)", true);
}

TEST(MaskMatcher, Multiple) {
  MaskMatcher matcher;
  ASSERT_TRUE(matcher.Compile("test****test"));
  TestMatch(matcher, "testtest", true);
  TestMatch(matcher, "testXtest", true);
  TestMatch(matcher, "testXXtest", true);
  TestMatch(matcher, "testXXXtest", true);
  TestMatch(matcher, "testXXXXtest", true);
  ASSERT_TRUE(matcher.Compile("test???test"));
  TestMatch(matcher, "testtest", true);
  TestMatch(matcher, "testXtest", true);
  TestMatch(matcher, "testXXtest", true);
  TestMatch(matcher, "testXXXtest", true);
  TestMatch(matcher, "testXXXXtest", false);
}

TEST(MaskMatcher, Compile) {
  MaskMatcher matcher;
  EXPECT_FALSE(matcher.Compile(nullptr));
}

TEST(MaskMatcher, Empty) {
  MaskMatcher matcher;
  TestMatch(matcher, "test", false);
  TestMatch(matcher, "", true);
  EXPECT_TRUE(matcher.Compile(""));
  TestMatch(matcher, "test", false);
  TestMatch(matcher, "", true);
}

TEST(MaskMatcher, CompileHelpExample) {
  MaskMatcher matcher;
  EXPECT_TRUE(matcher.Compile(R"(abc?abc\*abs*)"));
  TestMatch(matcher, "abcXabc*absX", true);
  TestMatch(matcher, "abcabc*abs", true);
}

TEST(MaskMatcher, Regress) {
  MaskMatcher matcher;
  EXPECT_TRUE(matcher.Compile("* who *."));
  TestMatch(matcher,
            "wimin who wear him next their harts.  But I do not weep for him.",
            true);
  TestMatch(matcher,
            "Men who have made money and do not yet know that money has made "
            "them....",
            true);
  TestMatch(matcher, " who they are....", true);
  TestMatch(
      matcher,
      "found in different forms amon....  who was really a daughter o....",
      true);
  EXPECT_TRUE(matcher.Compile("* th?m, *."));
  TestMatch(matcher,
            "-he never used them, by the way--and his mind is perfectly clear.",
            true);
}
