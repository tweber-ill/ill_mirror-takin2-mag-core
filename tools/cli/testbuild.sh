#!/bin/sh

/usr/local/bin/bison -S lalr1.cc --defines=cliparser_impl.h -o cliparser_impl.cpp cliparser.y
/usr/local/bin/flex --c++ --header-file=clilexer_impl.h -o clilexer_impl.cpp clilexer.l
clang++ -std=c++14 -o cliparser cliparser.cpp cliparser_impl.cpp clilexer_impl.cpp
