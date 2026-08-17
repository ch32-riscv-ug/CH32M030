#!/bin/bash
#
# Download datasheets / EVT archives and extract them.
# データシート / EVT をダウンロードして解凍する。
# Run by GitHub Actions (.github/workflows/update.yml); the workflow commits changes.

set -euo pipefail

cd "$(dirname "$0")"

HARD_FAILS=()
SOFT_FAILS=()
LAST_FETCH_OK=0

# fetch <url> <output>
# HTTP errors are hard failures. Transport errors and truncated downloads are
# skipped until the next daily run. Valid files atomically replace old files.
fetch() {
  local url="$1" out="$2"
  local tmp="${out}.download.$$"
  local code
  LAST_FETCH_OK=0
  echo "Fetching ${out} <- ${url}"
  code=$(curl -sSL --http1.1 \
              --connect-timeout 30 --max-time 900 \
              --speed-time 30 --speed-limit 1024 \
              -o "$tmp" -w '%{http_code}' "$url") || code="000"
  if [ "$code" = "200" ] && _valid "$tmp" "$out"; then
    mv -f "$tmp" "$out"
    echo "  saved ${out} ($(wc -c < "$out") bytes)"
    LAST_FETCH_OK=1
    return 0
  fi
  rm -f "$tmp"
  if [ "$code" = "200" ] || [ "$code" = "000" ]; then
    echo "  -> transient failure (status=${code}); skipping, will retry next run" >&2
    SOFT_FAILS+=("${out}  ${url}")
  else
    echo "  -> HTTP ${code}: genuine error (URL changed or server error)" >&2
    HARD_FAILS+=("${out}  HTTP ${code}  ${url}")
  fi
}

_valid() {
  local tmp="$1" out="$2"
  if [ ! -s "$tmp" ]; then
    echo "  -> empty response" >&2
    return 1
  fi
  case "${out,,}" in
    *.pdf)
      if [ "$(head -c 4 "$tmp")" != "%PDF" ]; then
        echo "  -> not a PDF" >&2
        return 1
      fi
      ;;
    *.zip)
      if ! unzip -tqq "$tmp" >/dev/null 2>&1; then
        echo "  -> not a valid/complete ZIP" >&2
        return 1
      fi
      ;;
  esac
}

unzip_evt() {
  rm -rfv EVT
  unzip -O GB2312 "$1"
}

finish() {
  if [ "${#SOFT_FAILS[@]}" -gt 0 ]; then
    echo "::warning::skipped ${#SOFT_FAILS[@]} download(s) due to transient errors; will retry next run"
    printf '  - %s\n' "${SOFT_FAILS[@]}"
  fi
  if [ "${#HARD_FAILS[@]}" -gt 0 ]; then
    echo "::error::${#HARD_FAILS[@]} download(s) failed with a genuine error (URL changed / server error)" >&2
    printf '  - %s\n' "${HARD_FAILS[@]}" >&2
    exit 1
  fi
}

mkdir -p datasheet_en datasheet_zh

# https://www.wch-ic.com/products/CH32M030.html
cd datasheet_en
# https://www.wch-ic.com/downloads/CH32M030DS0_PDF.html
fetch "https://www.wch-ic.com/download/file?id=408" CH32M030DS0.PDF
# https://www.wch-ic.com/downloads/CH32M030RM_PDF.html
fetch "https://www.wch-ic.com/download/file?id=420" CH32M030RM.PDF
cd ..

# https://www.wch.cn/products/CH32M030.html
cd datasheet_zh
# https://www.wch.cn/downloads/CH32M030DS0_PDF.html
fetch "https://file.wch.cn/download/file?id=506" CH32M030DS0.PDF
# https://www.wch.cn/downloads/CH32M030DS2_PDF.html
fetch "https://file.wch.cn/download/file?id=543" CH32M030DS2.PDF
# https://www.wch.cn/downloads/CH32M030RM_PDF.html
fetch "https://file.wch.cn/download/file?id=507" CH32M030RM.PDF
cd ..

# https://www.wch.cn/downloads/CH32M030EVT_ZIP.html
fetch "https://file.wch.cn/download/file?id=505" CH32M030EVT.ZIP
if [ "$LAST_FETCH_OK" = 1 ]; then unzip_evt CH32M030EVT.ZIP; fi

finish
