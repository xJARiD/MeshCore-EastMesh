#!/usr/bin/env bash
# Web panel heap-health check for *_repeater_observer nodes.
#
# Reads memory / web / mqtt / wifi status over the HTTPS API, then (optionally)
# hammers the panel the way a browser page-load does and re-reads memory to see
# whether the internal heap fragments. Written while chasing the Aug 2026
# "responds over LoRa but not WiFi" issue (TLS handshakes failing once the
# largest internal heap block dropped below ~40KB).
#
# What to look at:
#   heap_max   largest contiguous internal block. Must stay well above ~40KB.
#   heap_min   low-water mark since boot. Near 0 = the heap ran dry at some point.
#   heals      web panel self-restarts because heap_max fell below 24KB.
#   deferred   web panel starts postponed for lack of heap headroom.
#   leaked     MQTT clients abandoned after a heap-integrity failure.
#
# Usage:
#   eastmesh-tools/web-heap-check.sh <host> [--stress] [--rounds N]
#
# Auth: set REPEATER_PASSWORD (the admin password; the script logs in itself) or
# REPEATER_TOKEN (an existing session token). If neither is set you are prompted
# for the password. Tokens are never written to disk.
#
# Example:
#   REPEATER_PASSWORD=... eastmesh-tools/web-heap-check.sh 192.168.1.50 --stress
set -euo pipefail

host="${1:-}"
stress=0
rounds=5
shift || true
while [ $# -gt 0 ]; do
  case "$1" in
    --stress) stress=1 ;;
    --rounds) rounds="$2"; shift ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
  shift
done
if [ -z "$host" ]; then
  sed -n '2,24p' "$0"; exit 2
fi

base="https://$host"
curlq() { curl -sk -m 20 "$@"; }

token="${REPEATER_TOKEN:-}"
if [ -z "$token" ]; then
  pw="${REPEATER_PASSWORD:-}"
  if [ -z "$pw" ]; then
    read -r -s -p "admin password for $host: " pw; echo
  fi
  token=$(curlq -X POST "$base/login" --data "$pw") || true
  if [ ${#token} -ne 32 ]; then
    echo "login failed: $token" >&2; exit 1
  fi
fi

cmd() { curlq "$base/api/command" -H "X-Auth-Token: $token" --data "$1"; echo; }

echo "== $host  $(date '+%Y-%m-%d %H:%M:%S')"
for c in ver clock "get wifi.status" "get web.status" "get mqtt.status"; do
  printf '%-16s %s\n' "$c" "$(cmd "$c")"
done
before=$(cmd memory)
printf '%-16s %s\n' "memory" "$before"

if [ "$stress" = 1 ]; then
  echo "-- stress: $rounds rounds x (8 parallel page loads + 6 API polls)"
  for r in $(seq 1 "$rounds"); do
    for i in 1 2 3 4; do
      curlq -o /dev/null "$base/" & curlq -o /dev/null "$base/app" &
    done
    wait
    for i in 1 2 3; do
      curlq -o /dev/null "$base/api/stats" -H "X-Auth-Token: $token"
      curlq -o /dev/null "$base/api/session" -H "X-Auth-Token: $token"
    done
  done
  after=$(cmd memory)
  sleep 20
  settled=$(cmd memory)
  printf '%-16s %s\n' "memory before" "$before"
  printf '%-16s %s\n' "memory after" "$after"
  printf '%-16s %s\n' "memory +20s" "$settled"
  printf '%-16s %s\n' "web.status" "$(cmd 'get web.status')"
fi

# One-line verdict on the numbers that matter (post-stress sample if we have one).
final="${settled:-$before}"
hm=$(echo "$final" | sed -n 's/.*"heap_max":\([0-9]*\).*/\1/p')
hmin=$(echo "$final" | sed -n 's/.*"heap_min":\([0-9]*\).*/\1/p')
if [ "${hm:-0}" -lt 40000 ] || [ "${hmin:-0}" -lt 16000 ]; then
  echo "VERDICT: heap starved or fragmented (heap_max=$hm heap_min=$hmin) - TLS handshakes at risk"
else
  echo "VERDICT: ok (heap_max=$hm heap_min=$hmin)"
fi
