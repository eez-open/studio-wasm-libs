#pragma once

#include <stdint.h>

#include "lvgl/lvgl.h"

#include <eez/flow/lvgl_api.h>

extern uint32_t screenLoad_animType;
extern uint32_t screenLoad_speed;
extern uint32_t screenLoad_delay;

#ifdef __cplusplus
extern "C" {
#endif
void flowInit(uint32_t wasmModuleId, uint32_t debuggerMessageSubsciptionFilter, uint8_t *assets, uint32_t assetsSize, bool darkTheme, uint32_t timeZone, bool screensLifetimeSupport);
bool flowTick();
#ifdef __cplusplus
}
#endif

void flowOnPageLoadedStudio(unsigned pageIndex);

void addTimelineKeyframe(
    lv_obj_t *obj,
    void *flowState,
    float start, float end,
    uint32_t enabledProperties,
    int16_t x, uint8_t xEasingFunc,
    int16_t y, uint8_t yEasingFunc,
    int16_t width, uint8_t widthEasingFunc,
    int16_t height, uint8_t heightEasingFunc,
    float opacity, uint8_t opacityEasingFunc,
    int16_t scale, uint8_t scaleEasingFunc,
    int16_t rotate, uint8_t rotateEasingFunc,
    int32_t cp1x, int32_t cp1y, int32_t cp2x, int32_t cp2y
);
void setTimelinePosition(float timelinePosition);
void clearTimeline();

#ifdef __cplusplus
extern "C" {
#endif

void setObjectIndex(lv_obj_t *obj, int32_t index);

#ifdef __cplusplus
}
#endif

void deleteObjectIndex(int32_t index);
