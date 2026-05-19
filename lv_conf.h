#ifndef LV_CONF_H
#define LV_CONF_H

/* Color depth: 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Size of the memory available for LVGL */
#define LV_MEM_SIZE (48U * 1024U)

/* Use custom malloc/free */
#define LV_MEM_CUSTOM 0

/* Enable built-in fonts */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable symbols */
#define LV_USE_FONT_SYMBOL 1

/* Enable animations */
#define LV_USE_ANIMATION 1

/* Enable shadow drawing */
#define LV_USE_SHADOW 1

/* Enable gradient drawing */
#define LV_USE_GRADIENT 1

/* Enable anti-aliasing */
#define LV_USE_ANTI_ALIASING 1

/* Default display refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Use a custom tick source */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* Enable debug features */
#define LV_USE_DEBUG 0

/* Enable logging */
#define LV_USE_LOG 0

/* Enable asserts */
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MEM 0
#define LV_USE_ASSERT_STYLE 0

/* Performance monitor */
#define LV_USE_PERF_MONITOR 0

/* Memory monitor */
#define LV_USE_MEM_MONITOR 0

/* Enable GPU */
#define LV_USE_GPU 0

/* Enable widgets */
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_CHART 1

/* Enable themes */
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_BASIC 1
#define LV_USE_THEME_MONO 0

/* Layout */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/* File system */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0

/* PNG decoder */
#define LV_USE_PNG 0

/* JPG decoder */
#define LV_USE_SJPG 0

/* GIF decoder */
#define LV_USE_GIF 0

/* BMP decoder */
#define LV_USE_BMP 0

/* Enable examples */
#define LV_BUILD_EXAMPLES 0

#endif /* LV_CONF_H */