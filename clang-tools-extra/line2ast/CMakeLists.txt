set(LLVM_LINK_COMPONENTS support)

add_clang_executable(line2ast
    line2ast.cpp
  )
target_link_libraries(line2ast
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )

