#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace clang::ast_matchers;

using namespace clang::tooling;
using namespace llvm;

// Example Options /////////////////////////////////////////////////////////////

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

// Example Matcher ////////////////////////////////////////////////////////////

StatementMatcher matcher =
  implicitCastExpr();

// Call-backs //////////////////////////////////////////////////////////////////

class LoopPrinter : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop")) {
      FS->dump();
      printf("\n\n\n");
    }
  }
};

class Foo : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) override {
    }
};

// main ////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv) {
  auto OptionsParser = CommonOptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  Foo callback;
  MatchFinder Finder;
  Finder.addMatcher(matcher, &callback);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}

