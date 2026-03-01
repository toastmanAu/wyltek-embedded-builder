#!/bin/bash
# wyltek-embedded-builder board test runner
# Usage: bash test/run_tests.sh [--md] [--verbose] [--board BOARD_NAME]

set -euo pipefail
REPO=$(cd "$(dirname "$0")/.." && pwd)
cd "$REPO"

MD=0; VERBOSE=0; ONLY_BOARD=""
for arg in "$@"; do
  [[ "$arg" == "--md" ]]      && MD=1
  [[ "$arg" == "--verbose" ]] && VERBOSE=1
done
for i in "$@"; do [[ "$i" == "--board" ]] && { ONLY_BOARD="${@: $((${#@}))}" ; break; }; done
# simpler --board parse
for ((i=1;i<=$#;i++)); do
  if [[ "${!i}" == "--board" ]]; then
    next=$((i+1)); ONLY_BOARD="${!next}"
  fi
done

R='\033[0;31m' G='\033[0;32m' Y='\033[0;33m' C='\033[0;36m' NC='\033[0m' BOLD='\033[1m'

# All board defines (first block + extended block)
BOARDS=(
  CYD CYD2USB DOUBLE_EYE GC9A01_GENERIC ILI9341_ADAFRUIT ILI9341_GENERIC
  M5STACK_CORE ESP32_3248S035 GUITION4848S040 SUNTON_8048S043 WT32_SC01_PLUS
  LILYGO_TDISPLAY_S3 XIAO_S3_ROUND LILYGO_A7670SA LILYGO_TQTC6
  LILYGO_TSIM7080G_S3 LILYGO_TDISPLAY_S3_LONG LILYGO_TKEYBOARD_S3
  LILYGO_TDISPLAY_S3_AMOLED LILYGO_TIMPULSE LILYGO_TDECK LILYGO_TPICO_S3
  TTGO_TBEAM TTGO_TGO TTGO_TDISPLAY WAVESHARE_147_S3 WAVESHARE_200_S3
  ST7789_GENERIC ESP32CAM ESP32S3EYE TTGO_TBEAM_MESHTASTIC TWATCH_2020_V3
  HELTEC_LORA32_V3 LILYGO_TBEAM_SUPREME
  LOLIN_S3_PRO ESP32C3_GC9A01_128 FREENOVE_ESP32S3_CAM TSCINBUNY_ESP32_PLUS_CAM
  ESP32S3_LVGL_HMI_43 ILI9488_SPI_GENERIC GUITION_3248W535 GUITION_8048W550
  GUITION_3232W328
)

if [[ -n "$ONLY_BOARD" ]]; then
  BOARDS=("$ONLY_BOARD")
fi

echo ""
printf "${BOLD}${C}╔══════════════════════════════════════════════════════╗${NC}\n"
printf "${BOLD}${C}║      wyltek-embedded-builder Board Test Suite        ║${NC}\n"
printf "${BOLD}${C}║  g++ $(g++ --version | head -1 | grep -oP '\d+\.\d+\.\d+')  •  ${#BOARDS[@]} boards  •  $(date +%Y-%m-%d)          ║${NC}\n"
printf "${BOLD}${C}╚══════════════════════════════════════════════════════╝${NC}\n\n"

TOTAL_P=0; TOTAL_F=0; TOTAL_BOARDS=0; FAILED_BOARDS=()
declare -A BP BF BO

for board in "${BOARDS[@]}"; do
  bin="/tmp/wytest_${board}"
  build_out=$(g++ -std=c++17 -w -DWY_BOARD_${board} -Isrc \
    test/test_boards.cpp -o "$bin" 2>&1) || {
    BP[$board]=0; BF[$board]=1; BO[$board]="BUILD FAILED: $build_out"
    TOTAL_F=$((TOTAL_F+1)); FAILED_BOARDS+=("$board"); TOTAL_BOARDS=$((TOTAL_BOARDS+1))
    printf "  ${R}✗${NC} %-35s BUILD FAILED\n" "$board"
    continue
  }
  out=$(timeout 10 "$bin" 2>&1) || true
  p=$(echo "$out" | grep -cE '^\s*PASS:' 2>/dev/null || true)
  f=$(echo "$out" | grep -cE '^\s*FAIL:' 2>/dev/null || true)
  BP[$board]=$p; BF[$board]=$f; BO[$board]="$out"
  TOTAL_P=$((TOTAL_P+p)); TOTAL_F=$((TOTAL_F+f)); TOTAL_BOARDS=$((TOTAL_BOARDS+1))
  if [[ $f -eq 0 ]]; then
    printf "  ${G}✓${NC} %-35s ${BOLD}%2d tests${NC}\n" "$board" "$p"
  else
    printf "  ${R}✗${NC} %-35s ${BOLD}%2d passed, %d failed${NC}\n" "$board" "$p" "$f"
    FAILED_BOARDS+=("$board")
  fi
  if [[ $VERBOSE -eq 1 ]]; then echo "$out"; fi
done

# ── Sensor math tests ─────────────────────────────────────────────────────
echo ""
echo "  Running sensor math tests..."
SENSOR_BIN="/tmp/wytest_sensor_math"
SENSOR_BUILD_ERR=$(g++ -std=c++17 -DHOST_TEST -Isrc test/test_sensor_math.cpp   -o "$SENSOR_BIN" -lm 2>&1) || true
if [[ ! -x "$SENSOR_BIN" ]]; then
  printf "  ${R}✗${NC} %-35s BUILD FAILED\n" "sensor_math"
  SENSOR_PASS=0; SENSOR_FAIL=1; SENSOR_OUT="BUILD FAILED: $SENSOR_BUILD_ERR"
else
  SENSOR_OUT=$(timeout 30 "$SENSOR_BIN" 2>&1) || true
  SENSOR_PASS=$(echo "$SENSOR_OUT" | grep -cE '^\s*PASS:' 2>/dev/null || true)
  SENSOR_FAIL=$(echo "$SENSOR_OUT" | grep -cE '^\s*FAIL:' 2>/dev/null || true)
  if [[ $SENSOR_FAIL -eq 0 ]]; then
    printf "  ${G}✓${NC} %-35s ${BOLD}%2d tests${NC}\n" "sensor_math" "$SENSOR_PASS"
  else
    printf "  ${R}✗${NC} %-35s ${BOLD}%2d passed, %d failed${NC}\n" \
      "sensor_math" "$SENSOR_PASS" "$SENSOR_FAIL"
  fi
  if [[ $VERBOSE -eq 1 ]]; then echo "$SENSOR_OUT"; fi
fi
TOTAL_P=$((TOTAL_P + SENSOR_PASS))
TOTAL_F=$((TOTAL_F + SENSOR_FAIL))

echo ""
PASS_BOARDS=$((TOTAL_BOARDS - ${#FAILED_BOARDS[@]}))
[[ ${#FAILED_BOARDS[@]} -eq 0 ]] && col=$G || col=$R
printf "  ${BOLD}Boards: ${col}${PASS_BOARDS}/${TOTAL_BOARDS} passed${NC}${BOLD}  •  Tests: ${col}${TOTAL_P} passed${NC}\n\n"

# Failure details
if [[ $SENSOR_FAIL -gt 0 ]]; then
  echo "  ${R}Failures in sensor_math:${NC}"
  echo "$SENSOR_OUT" | grep -E '^\s*FAIL:|BUILD FAILED' | while read l; do echo "    $l"; done
fi
for board in "${FAILED_BOARDS[@]}"; do
  echo "  ${R}Failures in $board:${NC}"
  echo "${BO[$board]}" | grep -E '^\s*FAIL:|BUILD FAILED' | while read l; do
    echo "    $l"
  done
done

# --md report
if [[ $MD -eq 1 ]]; then
  COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
  REPORT="test/REPORT.md"
  {
    echo "# wyltek-embedded-builder Board Test Report"
    echo ""
    echo "**Date:** $(date '+%Y-%m-%d %H:%M')  |  **Commit:** \`$COMMIT\`  |  **Boards tested:** ${TOTAL_BOARDS}"
    echo ""
    echo "## Summary"
    echo ""
    echo "| Board | Tests | Result |"
    echo "|-------|-------|--------|"
    for board in "${BOARDS[@]}"; do
      [[ ${BF[$board]} -eq 0 ]] && icon="🟢" || icon="🔴"
      echo "| $icon $board | ${BP[$board]} | $([ ${BF[$board]} -eq 0 ] && echo "PASS" || echo "${BF[$board]} FAILED") |"
    done
    echo ""
    echo "**Total:** ${TOTAL_P} tests passed across ${PASS_BOARDS}/${TOTAL_BOARDS} boards"
    echo ""
    if [[ ${#FAILED_BOARDS[@]} -gt 0 ]]; then
      echo "## Failures"
      echo ""
      for board in "${FAILED_BOARDS[@]}"; do
        echo "### $board"
        echo ""
        echo '```'
        echo "${BO[$board]}" | grep -E '^\s*FAIL:|BUILD FAILED'
        echo '```'
        echo ""
      done
    fi
  } > "$REPORT"
  echo "  Report saved → $REPORT"
fi

[[ ${#FAILED_BOARDS[@]} -eq 0 ]] && exit 0 || exit 1
