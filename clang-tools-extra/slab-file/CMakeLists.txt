set(LLVM_LINK_COMPONENTS support)

add_clang_executable(slab-file
    SlabFile.cpp
  )
target_link_libraries(slab-file
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )
