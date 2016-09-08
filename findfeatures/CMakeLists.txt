set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)
set(CLANG_EXCEPTIONS_FLAG "-fexceptions")

# z3 library
add_library( libz3 SHARED IMPORTED GLOBAL )
set_target_properties( libz3 PROPERTIES IMPORTED_LOCATION "/home/shalini/software/z3/build/libz3.so" )

add_clang_executable(findFeatures
  main.cpp
  RecursionClass.cpp
  SortingClass.cpp
  Helper.cpp
  FunctionAnalysis.cpp
  StmtDetails.cpp
  MyASTVisitor.cpp
  BlockDetails.cpp
  GetSymbolicExprVisitor.cpp
  MainFunction.cpp
  GlobalVars.cpp
  )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_EXCEPTIONS_FLAG}" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CLANG_EXCEPTIONS_FLAG}" )
target_link_libraries(findFeatures
  clangTooling
  clangBasic
  clangASTMatchers
  libz3
  )
