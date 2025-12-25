#!/bin/bash
MISC="../exported-functions-misc.txt"
LVGL="../exported-functions-lvgl.txt"
ALL="../exported-functions.txt"
~/emsdk/upstream/bin/llvm-objdump -t ./lvgl/lib/liblvgl.a | grep "g     F CODE.*\.hidden lv_" | cut -c 40- | sed 's/^/_/' > $LVGL
cat $MISC > $ALL
cat $LVGL >> $ALL
