#!/usr/bin/env bash
# Build a flashable test image of the sgnir firmware and package it as a build
# record. This script never flashes anything and never plays audio.

set -euo pipefail

CPPFLAGS_OVERRIDE="-fno-exceptions -fno-rtti -std=gnu++11 -Wno-deprecated"
ENCODER_ARGS=(-t stm32f4 -s 48000 -b 12000 -c 6000 -p 256)
DEFAULT_TOOLCHAIN="/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi/"

usage() {
  cat <<'EOF'
Usage: build.sh -m DESCRIPTION [options] [LABEL]

Builds firmware/eurorack-mi-sgnir from its current working tree (clean build)
and writes a build record to <builds-dir>/sgnir-<short-sha>[-LABEL]/:

  rings.bin                      application image (link address 0x08008000)
  sgnir-<LABEL|short-sha>.wav    audio-bootloader image, round-trip verified
  SHA256SUMS                     checksums of both
  BUILD_INFO.txt                 description + how the build was made
  uncommitted.diff, status.txt   only for a dirty tree (includes untracked files)

LABEL   Short name of the build, e.g. diag1, listen2, rc5 (a-z 0-9 . _ -).
        Required when the firmware tree has uncommitted changes.

Options:
  -m, --message TEXT   what the build is for, what changed against the previous
                       build, how to read any diagnostic display (required)
  --builds-dir DIR     where build records go
                       (default: <workbench>/firmware/builds)
  --toolchain DIR      Arm GNU toolchain prefix that contains bin/
                       (default: $TOOLCHAIN_PATH, else the Arm GNU 12.2.Rel1
                       install under /Applications/ArmGNUToolchain)
  --workbench DIR      workbench root (default: derived from this script's path)
  --skip-tests         do not run the host tests (recorded in BUILD_INFO.txt)
  -h, --help           show this help

Exit status is non-zero, and no build record is left behind, if the host tests,
the build, or the WAV round trip fail.
EOF
}

die() { echo "build.sh: $*" >&2; exit 1; }

MESSAGE=""
LABEL=""
BUILDS_DIR=""
TOOLCHAIN="${TOOLCHAIN_PATH:-$DEFAULT_TOOLCHAIN}"
WORKBENCH=""
SKIP_TESTS=0

while [ $# -gt 0 ]; do
  case "$1" in
    -m|--message) [ $# -ge 2 ] || die "$1 needs a value"; MESSAGE="$2"; shift 2 ;;
    --builds-dir) [ $# -ge 2 ] || die "$1 needs a value"; BUILDS_DIR="$2"; shift 2 ;;
    --toolchain)  [ $# -ge 2 ] || die "$1 needs a value"; TOOLCHAIN="$2"; shift 2 ;;
    --workbench)  [ $# -ge 2 ] || die "$1 needs a value"; WORKBENCH="$2"; shift 2 ;;
    --skip-tests) SKIP_TESTS=1; shift ;;
    -h|--help) usage; exit 0 ;;
    -*) die "unknown option: $1 (see --help)" ;;
    *) [ -z "$LABEL" ] || die "more than one LABEL given"; LABEL="$1"; shift ;;
  esac
done

[ -n "$MESSAGE" ] || die "a description is required (-m); see --help"
if [ -n "$LABEL" ] && ! [[ "$LABEL" =~ ^[a-z0-9][a-z0-9._-]*$ ]]; then
  die "invalid LABEL '$LABEL' (allowed: a-z 0-9 . _ -)"
fi

if [ -z "$WORKBENCH" ]; then
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  WORKBENCH="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
fi
FW="$WORKBENCH/firmware/eurorack-mi-sgnir"
[ -f "$FW/rings/makefile" ] || die "no firmware tree at $FW (submodule not checked out?)"
[ -n "$BUILDS_DIR" ] || BUILDS_DIR="$WORKBENCH/firmware/builds"

case "$TOOLCHAIN" in */) ;; *) TOOLCHAIN="$TOOLCHAIN/" ;; esac
GCC="${TOOLCHAIN}bin/arm-none-eabi-gcc"
[ -x "$GCC" ] || die "no arm-none-eabi-gcc under ${TOOLCHAIN}bin/ (use --toolchain)"
GCC_VERSION="$("$GCC" --version | head -1)"

SHA="$(git -C "$FW" rev-parse --short HEAD)"
DESCRIBE="$(git -C "$FW" describe --tags --always 2>/dev/null || echo "$SHA")"
STATUS="$(git -C "$FW" status --porcelain)"
if [ -n "$STATUS" ]; then
  DIRTY=1
  [ -n "$LABEL" ] || die "the firmware tree has uncommitted changes: a LABEL is required"
else
  DIRTY=0
fi

NAME="sgnir-$SHA${LABEL:+-$LABEL}"
WAV="sgnir-${LABEL:-$SHA}.wav"
OUT="$BUILDS_DIR/$NAME"
[ ! -e "$OUT" ] || die "$OUT already exists. Build records are never overwritten: pick a new LABEL"

TMP="$(mktemp -d "${TMPDIR:-/tmp}/sgnir-build.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

# --- host tests -------------------------------------------------------------

run_suite() {  # name, binary -> prints "<n> cases ok", fails on any failure
  local name="$1" bin="$2" log="$TMP/$1.log"
  if ! "$bin" > "$log" 2>&1 || ! tail -1 "$log" | grep -q '^all cases ok'; then
    tail -20 "$log" >&2
    die "host tests failed: $name"
  fi
  echo "$(grep -c '^ok ' "$log") cases ok"
}

if [ "$SKIP_TESTS" -eq 1 ]; then
  TESTS_LINE="Host tests: SKIPPED (--skip-tests)"
  echo "== host tests skipped"
else
  echo "== host tests"
  (cd "$FW" && c++ -std=c++11 -O2 -DTEST -I. -o "$TMP/schliere_test" \
      rings/test/schliere_test.cc rings/dsp/schliere.cc) \
      || die "host test build failed: rings/test/schliere_test.cc"
  PORT_RESULT="$(run_suite port "$TMP/schliere_test")"
  (cd "$WORKBENCH" && c++ -std=c++17 -O1 -DTEST \
      -Ireference/schliere-owl/owl/test -Ireference/schliere-owl/owl/schliere \
      -Ifirmware/eurorack-mi-sgnir -o "$TMP/schliere_equiv" \
      tests/schliere/equivalence_test.cpp \
      firmware/eurorack-mi-sgnir/rings/dsp/schliere.cc) \
      || die "host test build failed: tests/schliere/equivalence_test.cpp"
  EQUIV_RESULT="$(run_suite equivalence "$TMP/schliere_equiv")"
  TESTS_LINE="Host tests: Schliere port $PORT_RESULT, equivalence vs. OWL reference $EQUIV_RESULT"
  echo "   port: $PORT_RESULT; equivalence: $EQUIV_RESULT"
fi

# --- firmware ---------------------------------------------------------------

echo "== firmware (clean build, $GCC_VERSION)"
MAKE_ARGS=(-f rings/makefile "TOOLCHAIN_PATH=$TOOLCHAIN" "CPPFLAGS=$CPPFLAGS_OVERRIDE")
(
  cd "$FW"
  make "${MAKE_ARGS[@]}" clean > /dev/null 2>&1 || true
  # The resources are checked in; if they look older than resources/*.py, make
  # tries to regenerate them with the Python 2 resource compiler.
  touch rings/resources.cc rings/resources.h
  make "${MAKE_ARGS[@]}" bin
) > "$TMP/make.log" 2>&1 || { tail -40 "$TMP/make.log" >&2; die "firmware build failed"; }
BIN="$FW/build/rings/rings.bin"
[ -s "$BIN" ] || die "build produced no rings.bin"
WARNINGS="$(grep -c 'warning:' "$TMP/make.log" || true)"

# --- package (staged, moved into place only when everything checks out) ------

STAGE="$TMP/$NAME"
mkdir -p "$STAGE"
cp "$BIN" "$STAGE/rings.bin"
chmod 644 "$STAGE/rings.bin"
SIZE="$(wc -c < "$STAGE/rings.bin" | tr -d ' ')"
echo "   rings.bin: $SIZE bytes, $WARNINGS compiler warnings"

echo "== audio-bootloader WAV"
export UV_CACHE_DIR="${UV_CACHE_DIR:-$WORKBENCH/.uv-cache}"
uv run --quiet --project "$WORKBENCH/tools" qpsk-encode "${ENCODER_ARGS[@]}" \
    -o "$STAGE/$WAV" "$STAGE/rings.bin" > "$TMP/encode.log" 2>&1 \
    || { cat "$TMP/encode.log" >&2; die "qpsk-encode failed"; }
uv run --quiet --project "$WORKBENCH/tools" qpsk-decode "$STAGE/$WAV" \
    -o "$TMP/roundtrip.bin" > "$TMP/decode.log" 2>&1 \
    || { cat "$TMP/decode.log" >&2; die "qpsk-decode failed"; }
DECODE_LINE="$(grep -E 'packets, [0-9]+ CRC failures' "$TMP/decode.log" | tail -1)"
PACKETS="$(echo "$DECODE_LINE" | sed -E 's/.*\(([0-9]+) packets.*/\1/')"
CRC_FAILURES="$(echo "$DECODE_LINE" | sed -E 's/.*packets, ([0-9]+) CRC failures.*/\1/')"
[ "$CRC_FAILURES" = "0" ] || die "round trip: CRC failures ($DECODE_LINE)"
head -c "$SIZE" "$TMP/roundtrip.bin" | cmp -s - "$STAGE/rings.bin" \
    || die "round trip: decoded payload differs from rings.bin"
echo "   $WAV: round trip ok ($PACKETS packets, 0 CRC failures)"

if command -v sha256sum > /dev/null 2>&1; then SHASUM=(sha256sum); else SHASUM=(shasum -a 256); fi
(cd "$STAGE" && "${SHASUM[@]}" rings.bin "$WAV" > SHA256SUMS)
BIN_SHA="$(awk '$2 == "rings.bin" {print $1}' "$STAGE/SHA256SUMS")"
WAV_SHA="$(awk -v f="$WAV" '$2 == f {print $1}' "$STAGE/SHA256SUMS")"

if [ "$DIRTY" -eq 1 ]; then
  git -C "$FW" status --short > "$STAGE/status.txt"
  git -C "$FW" diff HEAD > "$STAGE/uncommitted.diff"
  # Untracked files are not part of `git diff HEAD`; append them so the record
  # is enough to reproduce the build.
  git -C "$FW" ls-files --others --exclude-standard | while IFS= read -r f; do
    (cd "$FW" && git diff --no-index -- /dev/null "$f" || true)
  done >> "$STAGE/uncommitted.diff"
  TREE_LINE="Base: eurorack-mi-sgnir $SHA ($DESCRIBE), UNCOMMITTED changes on top (uncommitted.diff, status.txt)"
else
  TREE_LINE="Base: eurorack-mi-sgnir $SHA ($DESCRIBE), clean tree"
fi

{
  echo "$MESSAGE"
  echo
  echo "$TREE_LINE"
  echo "Toolchain: $GCC_VERSION,"
  echo "  CPPFLAGS=\"$CPPFLAGS_OVERRIDE\", clean build, $WARNINGS compiler warnings"
  echo "$TESTS_LINE"
  echo "rings.bin: $SIZE bytes, SHA-256 $BIN_SHA"
  echo "$WAV: qpsk-encode ${ENCODER_ARGS[*]} (workbench tools/),"
  echo "  SHA-256 $WAV_SHA"
  echo "Roundtrip: qpsk-decode -> $PACKETS packets, 0 CRC failures, first $SIZE bytes identical to rings.bin"
  echo "Built $(date +%Y-%m-%d). Flash status and results: firmware/flashlog/ (this file is not updated after flashing)."
} > "$STAGE/BUILD_INFO.txt"

mkdir -p "$BUILDS_DIR"
mv "$STAGE" "$OUT"

echo "== done"
echo "record: $OUT"
echo "wav:    $OUT/$WAV"
