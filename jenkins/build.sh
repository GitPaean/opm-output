#!/bin/bash

source `dirname $0`/build-opm-output.sh

ERT_REVISION=master
OPM_COMMON_REVISION=master
OPM_PARSER_REVISION=master
OPM_MATERIAL_REVISION=master
OPM_CORE_REVISION=master

build_opm_output
test $? -eq 0 || exit 1

cp serial/build-opm-output/testoutput.xml .
