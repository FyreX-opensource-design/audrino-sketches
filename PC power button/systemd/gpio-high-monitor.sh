#!/bin/bash
# Watches for 0 -> 1 on a GPIO line and runs TRIGGER_SCRIPT once per transition.
# Requires: libgpiod (gpioget)

set -euo pipefail

: "${GPIO_CHIP:=gpiochip1}"
: "${GPIO_LINE:=92}"
: "${TRIGGER_SCRIPT:=/usr/local/bin/on-gpio-high.sh}"
: "${POLL_INTERVAL_SEC:=0.05}"

if [[ ! -x "$TRIGGER_SCRIPT" ]]; then
  echo "Trigger script not found or not executable: $TRIGGER_SCRIPT" >&2
  exit 1
fi

read_gpio() {
  local out
  out="$(gpioget "$GPIO_CHIP" "$GPIO_LINE")"
  if [[ "$out" == *"=active"* ]]; then
    echo 1
  else
    echo 0
  fi
}

prev="$(read_gpio)"

while true; do
  cur="$(read_gpio)"

  if [[ "$prev" -eq 0 && "$cur" -eq 1 ]]; then
    "$TRIGGER_SCRIPT"
  fi

  prev="$cur"
  sleep "$POLL_INTERVAL_SEC"
done
