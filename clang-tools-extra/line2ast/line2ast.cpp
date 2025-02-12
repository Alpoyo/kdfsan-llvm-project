#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <map>

using namespace clang;

// STYLES //////////////////////////////////////////////////////////////////////
#define STYLE_RESET "\033[0m"
#define STYLE_BOLD "\033[1m"
#define STYLE_RED "\033[38;2;255;0;0m"
#define STYLE_ORANGE "\033[38;2;255;205;0m"
#define STYLE_YELLOW "\033[38;2;255;255;0m"
#define STYLE_GREEN "\033[38;2;0;205;0m"
#define STYLE_CYAN "\033[38;2;0;255;255m"
#define STYLE_BLUE "\033[38;2;0;150;255m"
#define STYLE_PURPLE "\033[38;2;255;0;255m"

// Ugly global variables ///////////////////////////////////////////////////////
std::string main_file;
int line_number_mode_;

// Utility /////////////////////////////////////////////////////////////////////
std::string abspath(std::string relpath) {
    char abs[PATH_MAX];
    char* ret = realpath(relpath.data(), abs);
    if (ret == NULL) {
        return "";
    }

    return std::string(abs, abs + strlen(abs));
}
enum line_number_mode {
    LNM_EXPANSION,
    LNM_PRESUMED,
    LNM_SPELLING,
};

// Options /////////////////////////////////////////////////////////////////////

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

static llvm::cl::opt<int> LineOpt(
    "ast-line",
    llvm::cl::desc("The line to dump the ast for"),
    llvm::cl::init(-1),
    llvm::cl::cat(MyToolCategory)
);
static llvm::cl::opt<int> LineOptEnd(
    "ast-line-end",
    llvm::cl::desc("The last (inclusive) line for the range of lines to dump the\n"
                   "ast for. By default equal to **ast-line**"),
    llvm::cl::init(-1),
    llvm::cl::cat(MyToolCategory)
);
static llvm::cl::opt<std::string> LineNumberMode(
    "ast-line-mode",
    llvm::cl::desc("'expansion', 'presumed', or 'spelling'"),
    llvm::cl::init("presumed"),
    llvm::cl::cat(MyToolCategory)
);

static llvm::cl::extrahelp CommonHelp(tooling::CommonOptionsParser::HelpMessage);

static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");

// Example code ////////////////////////////////////////////////////////////////
class FindLineVisitor
  : public RecursiveASTVisitor<FindLineVisitor> {
public:
  explicit FindLineVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitStmt(Stmt *stmt) {
    SourceManager& sm = Context->getSourceManager();
    bool invalid_s = false;
    bool invalid_t = false;
    // unsigned s = sm.getPresumedLineNumber(stmt->getBeginLoc(), &invalid_s);
    // unsigned t = sm.getPresumedLineNumber(stmt->getEndLoc(), &invalid_t);
    unsigned s, t;
    switch (line_number_mode_) {
    default:
    case LNM_EXPANSION:
        s = sm.getExpansionLineNumber(stmt->getBeginLoc(), &invalid_s);
        t = sm.getExpansionLineNumber(stmt->getEndLoc(), &invalid_t);
        break;
    case LNM_PRESUMED:
        s = sm.getPresumedLineNumber(stmt->getBeginLoc(), &invalid_s);
        t = sm.getPresumedLineNumber(stmt->getEndLoc(), &invalid_t);
        break;
    case LNM_SPELLING:
        s = sm.getSpellingLineNumber(stmt->getBeginLoc(), &invalid_s);
        t = sm.getSpellingLineNumber(stmt->getEndLoc(), &invalid_t);
        break;
    }
    
    if (invalid_s || invalid_t) {
        return true;
    }

    std::string fname = abspath(sm.getFilename(stmt->getBeginLoc()).str());
    if (fname != main_file) {
        return true;
    }

    if (s >= (unsigned)LineOpt.getValue() && t <= (unsigned)LineOptEnd.getValue()) {
        stmt->dump();
        exit(0);
    }

    return true;
  }

  /* Likewise for decl */
  bool VisitDecl(Decl *decl) {
    SourceManager& sm = Context->getSourceManager();
    bool invalid_s = false;
    bool invalid_t = false;
    // unsigned s = sm.getPresumedLineNumber(decl->getBeginLoc(), &invalid_s);
    // unsigned t = sm.getPresumedLineNumber(decl->getEndLoc(), &invalid_t);
    unsigned s, t;
    switch (line_number_mode_) {
    default:
    case LNM_EXPANSION:
        s = sm.getExpansionLineNumber(decl->getBeginLoc(), &invalid_s);
        t = sm.getExpansionLineNumber(decl->getEndLoc(), &invalid_t);
        break;
    case LNM_PRESUMED:
        s = sm.getPresumedLineNumber(decl->getBeginLoc(), &invalid_s);
        t = sm.getPresumedLineNumber(decl->getEndLoc(), &invalid_t);
        break;
    case LNM_SPELLING:
        s = sm.getSpellingLineNumber(decl->getBeginLoc(), &invalid_s);
        t = sm.getSpellingLineNumber(decl->getEndLoc(), &invalid_t);
        break;
    }
    
    if (invalid_s || invalid_t) {
        return true;
    }

    std::string fname = abspath(sm.getFilename(decl->getBeginLoc()).str());
    if (fname != main_file) {
        return true;
    }

    if (s >= (unsigned)LineOpt.getValue() && t <= (unsigned)LineOptEnd.getValue()) {
        decl->dump();
        exit(0);
    }

    return true;
  }
private:
  ASTContext *Context;
};

class FindLineConsumer : public clang::ASTConsumer {
public:
  explicit FindLineConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindLineVisitor Visitor;
};

class FindLineAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::make_unique<FindLineConsumer>(&Compiler.getASTContext());
  }
};

// main ////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv) {
  auto OptionsParser = tooling::CommonOptionsParser(argc, argv, MyToolCategory);
  tooling::ClangTool Tool(OptionsParser.getCompilations(),
                          OptionsParser.getSourcePathList());

  // Parse options.
  if (LineOpt.getValue() < 0) {
      fprintf(stderr, STYLE_BOLD STYLE_RED "invalid/missing line number"
              STYLE_RESET "\n");
      return 1;
  }
  if (LineOptEnd.getValue() < 0) {
      LineOptEnd.setValue(LineOpt.getValue());
  }
  if (LineOptEnd.getValue() < LineOpt.getValue()) {
      fprintf(stderr, STYLE_BOLD STYLE_RED "invalid/missing line range"
              STYLE_RESET "\n");
      return 1;
  }
  std::map<std::string, int> str2mode({
          {"expansion", (int)LNM_EXPANSION},
          {"presumed", (int)LNM_PRESUMED},
          {"spelling", (int)LNM_SPELLING},
          });
  if (!str2mode.count(LineNumberMode.getValue())) {
      fprintf(stderr, STYLE_BOLD STYLE_RED "invalid line number mode"
              STYLE_RESET "\n");
      exit(1);
  }
  line_number_mode_ = str2mode[LineNumberMode.getValue()];
  const std::vector<std::string>& sourceFiles = OptionsParser.getSourcePathList();
  if (sourceFiles.size() != 1) {
      fprintf(stderr, STYLE_BOLD STYLE_RED "Only one file please" STYLE_RESET "\n");
      exit(1);
  }
  main_file = sourceFiles[0];
  main_file = abspath(main_file);
  if (main_file == std::string("")) {
    fprintf(stderr, STYLE_BOLD STYLE_RED "Cannot find main file" STYLE_RESET "\n");
    exit(1);
  }

  return Tool.run(tooling::newFrontendActionFactory<FindLineAction>().get());
}


