#!/bin/bash

#
# v8.4.0
#

# patch to prevent infinite loop in lv_obj_update_layout
sed -i 's/while(scr->scr_layout_inv) {/for (int i = 0; i < 1000 \&\& scr->scr_layout_inv; i++) {/' v8.4.0/lvgl/src/core/lv_obj_pos.c
cd v8.4.0/build
emcmake cmake ..
make -j8
cd ../..
# revert patch
cd v8.4.0/lvgl
git checkout src/core/lv_obj_pos.c
cd ../..

#
# v9.2.2
#

# patch to prevent infinite loop in lv_obj_update_layout
sed -i 's/while(scr->scr_layout_inv) {/for (int i = 0; i < 1000 \&\& scr->scr_layout_inv; i++) {/' v9.2.2/lvgl/src/core/lv_obj_pos.c
cd v9.2.2/build
emcmake cmake ..
make -j8
cd ../..
# revert patch
cd v9.2.2/lvgl
git checkout src/core/lv_obj_pos.c
cd ../..

#
# v9.3.0
#

# patch to prevent infinite loop in lv_obj_update_layout
sed -i 's/while(scr->scr_layout_inv) {/for (int i = 0; i < 1000 \&\& scr->scr_layout_inv; i++) {/' v9.3.0/lvgl/src/core/lv_obj_pos.c
cd v9.3.0/build
emcmake cmake ..
make -j8
cd ../..
# revert patch
cd v9.3.0/lvgl
git checkout src/core/lv_obj_pos.c
cd ../..

#
# v9.4.0
#

# patch to prevent infinite loop in lv_obj_update_layout
sed -i 's/while(scr->scr_layout_inv) {/for (int i = 0; i < 1000 \&\& scr->scr_layout_inv; i++) {/' v9.4.0/lvgl/src/core/lv_obj_pos.c
cd v9.4.0/build
emcmake cmake ..
make -j8
cd ../..
# revert patch
cd v9.4.0/lvgl
git checkout src/core/lv_obj_pos.c
cd ../..
