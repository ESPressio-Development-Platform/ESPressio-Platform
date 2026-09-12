#!/usr/bin/env bash
set -euo pipefail

CXX="${CXX:-g++}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/.native-test-build"
FLAGS=(-std=gnu++17 -Wall -Wextra -Werror -pedantic -I"${ROOT}/src")

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

"${CXX}" "${FLAGS[@]}" \
  "${ROOT}/tests/native/platform_capabilities_test.cpp" \
  -o "${BUILD_DIR}/platform_capabilities_test"
"${BUILD_DIR}/platform_capabilities_test"

for source in "${ROOT}"/tests/negative/*.cpp; do
  name="$(basename "${source}" .cpp)"
  log="${BUILD_DIR}/${name}.log"
  if "${CXX}" "${FLAGS[@]}" "${source}" -o "${BUILD_DIR}/${name}" >"${log}" 2>&1; then
    echo "ERROR: negative compile test unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
done

echo "ESPressio-Platform native capability tests passed"
