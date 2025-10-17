#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
HB_PLATFORM=${HB_PLATFORM:-linux}
HB_COMPILER=${HB_COMPILER:-gcc}

run_make() {
  local dir=$1
  shift
  ( cd "$ROOT_DIR/$dir" && make "$@" HB_PLATFORM="$HB_PLATFORM" HB_COMPILER="$HB_COMPILER" )
}

run_make src/ast/lexer clean
run_make src/ast/lexer
run_make utils/hbast clean
run_make utils/hbast
run_make tests/ast clean
run_make tests/ast tests

echo "hbast build complete: $ROOT_DIR/bin/$HB_PLATFORM/$HB_COMPILER/hbast"
