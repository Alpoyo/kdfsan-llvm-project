/* output "true" to stdout if the file defines the function for any of the
 * declared functions in slab.h, otherwise false.
 * The list of declared functions is hardcoded. */
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <cstdio>
#include <climits>
#include <cstdlib>

using namespace clang;
using namespace clang::ast_matchers;

using namespace clang::tooling;
using namespace llvm;

// Ugly global variables ///////////////////////////////////////////////////////
std::string main_file;

// Helper functions ////////////////////////////////////////////////////////////
std::string abspath(std::string relpath) {
    char abs[PATH_MAX];
    char* ret = realpath(relpath.data(), abs);
    if (ret == NULL) {
        return "";
    }

    return std::string(abs, abs + strlen(abs));
}

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

// Options /////////////////////////////////////////////////////////////////////

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

static cl::opt<bool> MyDebugOpt(
    "my-debug",
    cl::desc("Print more stuff for debugging"),
    cl::init(false),
    cl::cat(MyToolCategory)
);

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

// AST Matchers ////////////////////////////////////////////////////////////////

/* Define the function name filter, which matches against anything defined in 
 * slab.h */
auto slabFunc = anyOf(hasName("slab_is_available"), hasName("kmem_cache_create"), hasName("kmem_cache_create_usercopy"), hasName("kmem_cache_destroy"), hasName("kmem_cache_shrink"), hasName("krealloc"), hasName("kfree"), hasName("kfree_sensitive"), hasName("__ksize"), hasName("ksize"), hasName("kmem_valid_obj"), hasName("kmem_dump_obj"), hasName("arch_slab_minalign"), hasName("kmalloc_type"), hasName("__kmalloc_index"), hasName("__kmalloc"), hasName("kmem_cache_alloc"), hasName("kmem_cache_alloc_lru"), hasName("kmem_cache_free"), hasName("kmem_cache_free_bulk"), hasName("kmem_cache_alloc_bulk"), hasName("kfree_bulk"), hasName("__kmalloc_node"), hasName("kmem_cache_alloc_node"), hasName("kmalloc_trace"), hasName("kmalloc_node_trace"), hasName("kmalloc_large"), hasName("kmalloc_large_node"), hasName("kmalloc"), hasName("kmalloc_node"), hasName("kmalloc_array"), hasName("krealloc_array"), hasName("kcalloc"), hasName("__kmalloc_node_track_caller"), hasName("kmalloc_array_node"), hasName("kcalloc_node"), hasName("kmem_cache_zalloc"), hasName("kzalloc"), hasName("kzalloc_node"), hasName("kvmalloc_node"), hasName("kvmalloc"), hasName("kvzalloc_node"), hasName("kvzalloc"), hasName("kvmalloc_array"), hasName("kvcalloc"), hasName("kvrealloc"), hasName("kvfree"), hasName("kvfree_sensitive"), hasName("kmem_cache_size"), hasName("kmalloc_size_roundup"), hasName("kmem_cache_init_late"), hasName("object_to_kmem_cache"), hasName("object_to_stack_trace"), hasName("object_stuff"));

/* Matches an implicit cast that wraps one of the allocator functions. */
DeclarationMatcher matcher_slab_func = functionDecl(slabFunc).bind("func_decl");

// Call-backs //////////////////////////////////////////////////////////////////
class Quitter : public MatchFinder::MatchCallback {
public:
    virtual void run(const MatchFinder::MatchResult &Result) override {
        // TODO mode to print all the line numbers for all decls?

        /* Skip if it's just an include expansion */
        clang::SourceManager* sm = Result.SourceManager;
        const FunctionDecl *node = Result.Nodes.getNodeAs<clang::FunctionDecl>("func_decl");
        std::string fname = abspath(sm->getFilename(node->getBeginLoc()).str());
        if (fname != main_file) {
            return;
        }

        /* There is a match, notify this to stdout */
        printf("true\n");
        if (MyDebugOpt.getValue()) {
            node->dump();
        }
        exit(0);
    }
};

// main ////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv) {
  auto OptionsParser = tooling::CommonOptionsParser(argc, argv, MyToolCategory);
  tooling::ClangTool Tool(OptionsParser.getCompilations(),
                          OptionsParser.getSourcePathList());

  // Parse options.
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

  // Setup and run clang tool with my matcher pass
  Quitter printer;
  MatchFinder finder;
  finder.addMatcher(matcher_slab_func, &printer);

  Tool.run(newFrontendActionFactory(&finder).get());

  /* Report no success */
  printf("false\n");
  return 0;
}

