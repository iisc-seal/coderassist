set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(transform
  Transform.cpp
  Utils.cpp
  )
target_link_libraries(transform
  clangTooling
  clangBasic
  clangASTMatchers
  )
