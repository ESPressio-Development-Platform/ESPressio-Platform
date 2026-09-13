#!/usr/bin/env bash
set -euo pipefail

CXX="${CXX:-g++}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/.native-test-build"
SYSTEM_SRC="${ROOT}/deps/ESPressio-System/src"
FLAGS=(-std=gnu++17 -Wall -Wextra -Werror -pedantic -I"${ROOT}/src" -I"${SYSTEM_SRC}")

if [[ ! -f "${SYSTEM_SRC}/ESPressio_CompositionFramework.hpp" ]]; then
  echo "ERROR: ESPressio-System composition framework not found at ${SYSTEM_SRC}" >&2
  exit 1
fi

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

for source in "${ROOT}"/tests/native/*.cpp; do
  name="$(basename "${source}" .cpp)"
  "${CXX}" "${FLAGS[@]}" "${source}" -o "${BUILD_DIR}/${name}"
  "${BUILD_DIR}/${name}"
done

for source in "${ROOT}"/tests/negative/*.cpp; do
  name="$(basename "${source}" .cpp)"
  log="${BUILD_DIR}/${name}.log"
  if "${CXX}" "${FLAGS[@]}" "${source}" -o "${BUILD_DIR}/${name}" >"${log}" 2>&1; then
    echo "ERROR: negative compile test unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
done

echo "ESPressio-Platform native contract tests passed"
