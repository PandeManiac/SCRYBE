#!/usr/bin/env bash

set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s <binary-path>\n' "$0" >&2
    exit 1
fi

binary_path=$1

if [ ! -x "$binary_path" ]; then
    printf 'run_external.sh: binary is not executable: %s\n' "$binary_path" >&2
    exit 1
fi

run_command=$(printf '"%s"; printf "\\n"; read -r -p "Press enter to exit..." _' "$binary_path")

if [ -x /usr/bin/kitty ]; then
    /usr/bin/kitty --detach --hold bash -lc "$run_command"
    exit 0
fi

if command -v x-terminal-emulator >/dev/null 2>&1; then
    x-terminal-emulator -e bash -lc "$run_command" >/dev/null 2>&1 &
    exit 0
fi

if command -v gnome-terminal >/dev/null 2>&1; then
    gnome-terminal -- bash -lc "$run_command" >/dev/null 2>&1 &
    exit 0
fi

if command -v xterm >/dev/null 2>&1; then
    xterm -hold -e "$binary_path" >/dev/null 2>&1 &
    exit 0
fi

printf 'run_external.sh: no supported terminal found\n' >&2
exit 1
