#include "types.h"
#define CAKE_RECT Rectangle
#include "cake.h"
#include "ui.h"
#include "audio.h"

// What is this game?
// How do you win?
// basic mechanics
const char * manual_info_text =
    "In this game, you take a role of a warlord your aim is to conquer the lands\n"
    "\n"
    "Each region has a number of spots that allow you to purchase buildings\n"
    "Once you have a building, it will start producing units of its type\n"
    "Higher level buildings can accomodate larger armies,\n"
    "so it is worthwhile to upgrade them when you can.\n"
    "\n"
    "While you can't give orders directly to individual units,\n"
    "you can control them by placing flags on bridges to other regions\n"
    "Any unit within the region will then move to that region.\n"
    "Units will actively fight enemy soldiers when they see them,\n"
    "and siege enemy castles when you send them to hostile regions\n"
    "\n"
    "Sometimes, it is advantageus to amass your army instead\n"
    "You can take down the flag on a region, then your army will patrol instead\n"
    "Another way is to set flags so two regions send army to each other\n"
    "This allows you to concentrate your warriors in specific spot much easier\n"
    "\n"
    "The game ends when there's only one player left on the map.";

#if defined(ANDROID)
const char * manual_controls_text =
    "Your primary way of controling the game is through moving your camera\n"
    "You can press and drag on the screen for precise control\n"
    "Or you can use the joystick in lower left part of the screen\n"
    "\n"
    "Two buttons next to the joystick allow you to zoom view in and out\n"
    "\n"
    "In the center you will see a pointer which lets you aim at things\n"
    "Move the view so it aims at a building or bridge end\n"
    "Then you will see a context menu pop up on lower right\n"
    "\n"
    "When you aim at buildings, you can purchase and upgrade them there\n"
    "And when you aim at bridges, you can set or take down the flag\n";
#else
const char * manual_controls_text =
    "You can control the game with just your mouse.\n"
    "When you click and drag on the map, you will pan the camera\n"
    "Scroll wheel allows you to zoom the view in and out\n"
    "\n"
    "Click on the building spots within your region to open context menu\n"
    "Then you can purchase or upgrade those buildings\n"
    "\n"
    "When you click on the bridge end, a flag will be placed on it\n"
    "Unless there was one on it already, then it will be taken down\n";
#endif

const char * manual_economy_text =
    "Every couple of seconds, you will receive some amount of resources\n"
    "This amount depends on how many regions you own, the more, the better\n"
    "\n"
    "However, a much better is to purchase dedicated resource buildings\n"
    "Resource buildings such as farms, provide you with much greater income\n"
    "Even more so when you upgrade them, allowing you to field large armies\n"
    "With just few well upgraded farms\n"
    "\n"
    "Speaking of armies, each time a building produces a unit,\n"
    "a cost of that unit is subtracted from your cofers\n"
    "When you run out of resources, production of your army will stop\n"
    "So it is important to have good income to upkeep balance\n"
    "To ensure constant stream of new units entering your army\n";

const char * manual_fighters_text =
    "Fighters compose the main part of any army\n"
    "They're cheap, recruit quickly and are effective in battle\n"
    "They may lack unique abilities of other unit types\n"
    "but you can field them very effectively in large numbers\n";

const char * manual_archers_text =
    "Ranged units give advantage of significant attack range\n"
    "Allowing them to offend enemy units before they can retaliate\n"
    "However, they're harder to train\n"
    "and aren't as effective once their range advantage is breached\n";

const char * manual_support_text =
    "Each faction has an unique support unit providing exclusive advantages\n"
    "Overall, the support units are the weakest of all\n"
    "Making it dangerous to rely on them too much\n"
    "However, when behind friendly lines, they can turn the tide of battle\n"
    "\n"
    "Knights rely on their priests for healing miracles\n"
    "Higher ranks of priest are able to restore soldiers faster.\n"
    "\n"
    "Mages summon imps to cause havoc and desorientation among enemy troops\n"
    "The little devils use magic to weaken their enemies,\n"
    "This can severely limit combat effectivenes of even strongest warriors\n";

const char * manual_special_text =
    "All factions can field a special unit that has unique advantage\n"
    "They're not as easy to field as other unit typs\n"
    "But they provide additional options to your strategy\n"
    "\n"
    "Knights field mounted riders which excell at movement speed\n"
    "This allows for swift strategic strikes at enemy territory\n"
    "Or quick response when your own regions are under siege\n"
    "\n"
    "Mages make pacts with powerful genie warriors for support\n"
    "Those genie wield unmatched destructive power\n"
    "Yet being fairly fickle, relying on others to shield them from enemies\n";

const char * manual_credits_text =
    "This game has been made by:\n"
    "  Purrie Brightstar\n"
    "  Rewalt Muchaczowicz\n"
    "\n"
    "Tools used:\n"
    "  Libraries:\n"
    "    Raylib\n"
    "    JSMN\n"
    "\n"
    "  Code:\n"
    "    Emacs\n"
    "    gdb\n"
    "    gf2\n"
    "\n"
    "  Audio and Music:\n"
    "    sfxr\n"
    "    OpenMPT\n"
    "\n"
    "  Graphics:\n"
    "    Krita\n"
    "    Aseprite\n"
    "\n"
    "  Maps:\n"
    "    Tiled\n";

void manual_text (Rectangle area, const char * text, const Theme * theme) {
    DrawRectangleRec(area, theme->background);
    DrawRectangleLinesEx(area, theme->frame_thickness, theme->frame);

    area = cake_margin_all(area, theme->margin * 2);
    DrawText(text, area.x, area.y, theme->font_size, theme->text);
}

typedef enum {
    PAGE_INFO,
    PAGE_CONTROLS,
    PAGE_ECONOMY,
    PAGE_FIGHTERS,
    PAGE_ARCHERS,
    PAGE_SUPPORTS,
    PAGE_SPECIALS,
    PAGE_CREDITS,
} ManualPage;

ManualPage selected_page = PAGE_INFO;
const char * buttons[] = {
    [PAGE_INFO]     = "Game Information",
    [PAGE_CONTROLS] = "Controls",
    [PAGE_ECONOMY]  = "Economy",
    [PAGE_FIGHTERS] = "Warriors",
    [PAGE_ARCHERS]  = "Archers",
    [PAGE_SUPPORTS] = "Support Units",
    [PAGE_SPECIALS] = "Special Units",
    [PAGE_CREDITS]  = "Credits",
    NULL
};

ExecutionMode manual_mode (Assets * assets, const Theme * theme) {
    const Color black = (Color) {18, 18, 18, 255};
    const char * texts[] = {
        [PAGE_INFO]     = manual_info_text,
        [PAGE_CONTROLS] = manual_controls_text,
        [PAGE_ECONOMY]  = manual_economy_text,
        [PAGE_FIGHTERS] = manual_fighters_text,
        [PAGE_ARCHERS]  = manual_archers_text,
        [PAGE_SUPPORTS] = manual_support_text,
        [PAGE_SPECIALS] = manual_special_text,
        [PAGE_CREDITS]  = manual_credits_text,
    };

    while (1) {
        if (WindowShouldClose()) {
            return EXE_MODE_EXIT;
        }
        UpdateMusicStream(assets->main_theme);
        BeginDrawing();
        ClearBackground(black);

        Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
        Rectangle menu = cake_cut_vertical(&screen, 0.3f, 0);
        menu = cake_margin_all(menu, theme->margin);
        screen = cake_margin_all(screen, theme->margin);

        manual_text(screen, texts[selected_page], theme);

        DrawRectangleRec(menu, theme->background);
        DrawRectangleLinesEx(menu, theme->frame_thickness, theme->frame);

        menu = cake_margin_all(menu, theme->margin);

        float button_size = theme->font_size + theme->margin * 2.0f;
        Vector2 cursor = GetMousePosition();
        bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        uint8_t i = 0;
        while (buttons[i]) {
            Rectangle button = cake_cut_horizontal(&menu, button_size, theme->spacing);
            draw_button(button, buttons[i], cursor, UI_LAYOUT_LEFT, theme);
            if (click && CheckCollisionPointRec(cursor, button)) {
                play_sound(assets, SOUND_UI_CLICK);
                selected_page = i;
            }
            i++;
        }

        Rectangle button = cake_cut_horizontal(&menu, -button_size, theme->spacing);
        draw_button(button, "Back", cursor, UI_LAYOUT_LEFT, theme);
        if (click && CheckCollisionPointRec(cursor, button)) {
            play_sound(assets, SOUND_UI_CLICK);
            return EXE_MODE_MAIN_MENU;
        }
        EndDrawing();
    }

    return EXE_MODE_MAIN_MENU;
}
