#!/usr/bin/env bash
set -e

bash ./build_prep.sh "${1}"
bash ./build_compile.sh "${1}"
bash ./build_install.sh "${1}"
