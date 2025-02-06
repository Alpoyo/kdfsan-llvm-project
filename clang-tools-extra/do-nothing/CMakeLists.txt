set(LLVM_LINK_COMPONENTS support)

add_clang_executable(do-nothing
    DoNothing.cpp
  )
target_link_libraries(do-nothing
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )
