set(LLVM_LINK_COMPONENTS support)

add_clang_executable(line2type
    line2type.cpp
  )
target_link_libraries(line2type
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )

