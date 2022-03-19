#!/usr/bin/env bash

bash ci/version.sh
python grammar/asdl_cpp.py
python grammar/asdl_cpp.py grammar/ASR.asdl src/libasr/asr.h
