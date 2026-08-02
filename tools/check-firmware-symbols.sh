#!/bin/sh

set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: $0 <mcu>" >&2
  exit 2
fi

mcu=$1
set -- examples/*/build/arm/"$mcu"/*.elf

if [ ! -f "$1" ]; then
  echo "no firmware images found for $mcu; build the examples first" >&2
  exit 2
fi

for elf do
  unresolved=$(arm-none-eabi-nm -u "$elf")
  if [ -n "$unresolved" ]; then
    echo "$elf has unresolved symbols:" >&2
    echo "$unresolved" >&2
    exit 1
  fi

  forbidden=$(arm-none-eabi-nm "$elf" | awk '{print $3}' | grep -E \
    '^(malloc|calloc|realloc|free|_malloc_r|_calloc_r|_realloc_r|_free_r|_Zn[aw]|_Zd[alp]|__cxa_|__gxx_personality|__aeabi_unwind_cpp|_GLOBAL__sub_I_|_ZSt4(cout|cerr|clog)|_ZSt3cin)' \
    || true)
  if [ -n "$forbidden" ]; then
    echo "$elf contains forbidden runtime symbols:" >&2
    echo "$forbidden" >&2
    exit 1
  fi
done

echo "firmware symbol policy passed for $mcu ($# images)"
