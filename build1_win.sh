#!/usr/bin/env bash

bash ci/version.sh
python grammar/asdl_cpp.py
python grammar/asdl_cpp.py grammar/ASR.asdl src/libasr/asr.h
pushd src/lpython/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lpython/parser && re2c -W -b preprocessor.re -o preprocessor.cpp && popd
pushd src/lpython/parser && bison -Wall -d -r all parser.yy && popd
