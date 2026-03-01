#include "RaylibWrapper.h"
#include "Inputs.h"
#include "RubyValue.h"

VALUE rb_Keyboard_press(VALUE self, VALUE key)
{
    auto vkey = NUM2INT(key);
    return raylib::IsKeyDown(vkey) ? Qtrue : Qfalse;
}

void DefineRaylibKeyboardConstants(VALUE rb_mKeyboard)
{
    struct KeyMapping
    {
        const char *name;
        int value;
    };

    static constexpr KeyMapping KEYS[] = {
        {"Apostrophe", raylib::KEY_APOSTROPHE},
        {"Comma", raylib::KEY_COMMA},
        {"Minus", raylib::KEY_MINUS},
        {"Period", raylib::KEY_PERIOD},
        {"Slash", raylib::KEY_SLASH},
        {"Zero", raylib::KEY_ZERO},
        {"One", raylib::KEY_ONE},
        {"Two", raylib::KEY_TWO},
        {"Three", raylib::KEY_THREE},
        {"Four", raylib::KEY_FOUR},
        {"Five", raylib::KEY_FIVE},
        {"Six", raylib::KEY_SIX},
        {"Seven", raylib::KEY_SEVEN},
        {"Eight", raylib::KEY_EIGHT},
        {"Nine", raylib::KEY_NINE},
        {"Semicolon", raylib::KEY_SEMICOLON},
        {"Equal", raylib::KEY_EQUAL},
        {"A", raylib::KEY_A},
        {"B", raylib::KEY_B},
        {"C", raylib::KEY_C},
        {"D", raylib::KEY_D},
        {"E", raylib::KEY_E},
        {"F", raylib::KEY_F},
        {"G", raylib::KEY_G},
        {"H", raylib::KEY_H},
        {"I", raylib::KEY_I},
        {"J", raylib::KEY_J},
        {"K", raylib::KEY_K},
        {"L", raylib::KEY_L},
        {"M", raylib::KEY_M},
        {"N", raylib::KEY_N},
        {"O", raylib::KEY_O},
        {"P", raylib::KEY_P},
        {"Q", raylib::KEY_Q},
        {"R", raylib::KEY_R},
        {"S", raylib::KEY_S},
        {"T", raylib::KEY_T},
        {"U", raylib::KEY_U},
        {"V", raylib::KEY_V},
        {"W", raylib::KEY_W},
        {"X", raylib::KEY_X},
        {"Y", raylib::KEY_Y},
        {"Z", raylib::KEY_Z},
        {"LBracket", raylib::KEY_LEFT_BRACKET},
        {"Backslash", raylib::KEY_BACKSLASH},
        {"RBracket", raylib::KEY_RIGHT_BRACKET},
        {"Grave", raylib::KEY_GRAVE},
        {"Space", raylib::KEY_SPACE},
        {"Escape", raylib::KEY_ESCAPE},
        {"Enter", raylib::KEY_ENTER},
        {"Tab", raylib::KEY_TAB},
        {"Backspace", raylib::KEY_BACKSPACE},
        {"Insert", raylib::KEY_INSERT},
        {"Delete", raylib::KEY_DELETE},
        {"Right", raylib::KEY_RIGHT},
        {"Left", raylib::KEY_LEFT},
        {"Down", raylib::KEY_DOWN},
        {"Up", raylib::KEY_UP},
        {"PageUp", raylib::KEY_PAGE_UP},
        {"PageDown", raylib::KEY_PAGE_DOWN},
        {"Home", raylib::KEY_HOME},
        {"End", raylib::KEY_END},
        {"CapsLock", raylib::KEY_CAPS_LOCK},
        {"ScrollLock", raylib::KEY_SCROLL_LOCK},
        {"NumLock", raylib::KEY_NUM_LOCK},
        {"PrintScreen", raylib::KEY_PRINT_SCREEN},
        {"Pause", raylib::KEY_PAUSE},
        {"F1", raylib::KEY_F1},
        {"F2", raylib::KEY_F2},
        {"F3", raylib::KEY_F3},
        {"F4", raylib::KEY_F4},
        {"F5", raylib::KEY_F5},
        {"F6", raylib::KEY_F6},
        {"F7", raylib::KEY_F7},
        {"F8", raylib::KEY_F8},
        {"F9", raylib::KEY_F9},
        {"F10", raylib::KEY_F10},
        {"F11", raylib::KEY_F11},
        {"F12", raylib::KEY_F12},
        {"LShift", raylib::KEY_LEFT_SHIFT},
        {"LControl", raylib::KEY_LEFT_CONTROL},
        {"LAlt", raylib::KEY_LEFT_ALT},
        {"LSuper", raylib::KEY_LEFT_SUPER},
        {"RShift", raylib::KEY_RIGHT_SHIFT},
        {"RControl", raylib::KEY_RIGHT_CONTROL},
        {"RAlt", raylib::KEY_RIGHT_ALT},
        {"RSuper", raylib::KEY_RIGHT_SUPER},
        {"Menu", raylib::KEY_KB_MENU},
        {"Numpad0", raylib::KEY_KP_0},
        {"Numpad1", raylib::KEY_KP_1},
        {"Numpad2", raylib::KEY_KP_2},
        {"Numpad3", raylib::KEY_KP_3},
        {"Numpad4", raylib::KEY_KP_4},
        {"Numpad5", raylib::KEY_KP_5},
        {"Numpad6", raylib::KEY_KP_6},
        {"Numpad7", raylib::KEY_KP_7},
        {"Numpad8", raylib::KEY_KP_8},
        {"Numpad9", raylib::KEY_KP_9},
        {"NumpadDecimal", raylib::KEY_KP_DECIMAL},
        {"NumpadDivide", raylib::KEY_KP_DIVIDE},
        {"NumpadMultiply", raylib::KEY_KP_MULTIPLY},
        {"NumpadMinus", raylib::KEY_KP_SUBTRACT},
        {"NumpadPlus", raylib::KEY_KP_ADD},
        {"NumpadEnter", raylib::KEY_KP_ENTER},
        {"NumpadEqual", raylib::KEY_KP_EQUAL},
    };

    for (const auto &k : KEYS)
    {
        rb_define_const(rb_mKeyboard, k.name, INT2NUM(k.value));
    }
}

void Init_Keyboard()
{
    VALUE rb_mKeyboard = rb_define_module_under(rb_mInputs, "Keyboard");

    DefineRaylibKeyboardConstants(rb_mKeyboard);

    rb_define_module_function(rb_mKeyboard, "press?", _rbf rb_Keyboard_press, 1);
}
