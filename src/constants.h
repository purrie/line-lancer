#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define DEBUG

#define FPS 60

#define UNIT_SIZE 6.0f
#define UNIT_SPEED 20.0f

#define REGION_INCOME_INTERVAL 10
#define REGION_INCOME 3

#define BUILDING_MAX_UPGRADES 2

#define LAYER_MAP -0.3f
#define LAYER_PATH -0.2f
#define LAYER_BUILDING -0.1f
#define MAP_BEVEL 5

#define RENDER_DEBUG_BRIDGE
#define RENDER_PATHS

#define UI_DIALOG_BUILDING_WIDTH 350
#define UI_DIALOG_BUILDING_HEIGHT 200
#define UI_DIALOG_UPGRADE_WIDTH 200
#define UI_DIALOG_UPGRADE_HEIGHT 350
#define UI_BAR_SIZE 30
#define UI_BAR_MARGIN 5
#define UI_FONT_SIZE_BAR 20
#define UI_FONT_SIZE_BUTTON 20
#define UI_BORDER_SIZE 3

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#endif // CONSTANTS_H_
