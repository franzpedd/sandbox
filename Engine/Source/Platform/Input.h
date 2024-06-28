#pragma once

namespace Cosmos
{
    typedef enum Keycode : unsigned int
    {
        UNKNOWN = 0,

        KEY_RETURN = '\r',
        KEY_ESCAPE = '\x1B',
        KEY_BACKSPACE = '\b',
        KEY_TAB = '\t',
        KEY_SPACE = ' ',
        KEY_EXCLAIM = '!',
        KEY_QUOTEDBL = '"',
        KEY_HASH = '#',
        KEY_PERCENT = '%',
        KEY_DOLLAR = '$',
        KEY_AMPERSAND = '&',
        KEY_QUOTE = '\'',
        KEY_LEFTPAREN = '(',
        KEY_RIGHTPAREN = ')',
        KEY_ASTERISK = '*',
        KEY_PLUS = '+',
        KEY_COMMA = ',',
        KEY_MINUS = '-',
        KEY_PERIOD = '.',
        KEY_SLASH = '/',
        KEY_0 = '0',
        KEY_1 = '1',
        KEY_2 = '2',
        KEY_3 = '3',
        KEY_4 = '4',
        KEY_5 = '5',
        KEY_6 = '6',
        KEY_7 = '7',
        KEY_8 = '8',
        KEY_9 = '9',
        KEY_COLON = ':',
        KEY_SEMICOLON = ';',
        KEY_LESS = '<',
        KEY_EQUALS = '=',
        KEY_GREATER = '>',
        KEY_QUESTION = '?',
        KEY_AT = '@',

        KEY_LEFTBRACKET = '[',
        KEY_BACKSLASH = '\\',
        KEY_RIGHTBRACKET = ']',
        KEY_CARET = '^',
        KEY_UNDERSCORE = '_',
        KEY_BACKQUOTE = '`',
        KEY_A = 'a',
        KEY_B = 'b',
        KEY_C = 'c',
        KEY_D = 'd',
        KEY_E = 'e',
        KEY_F = 'f',
        KEY_G = 'g',
        KEY_H = 'h',
        KEY_I = 'i',
        KEY_J = 'j',
        KEY_K = 'k',
        KEY_L = 'l',
        KEY_M = 'm',
        KEY_N = 'n',
        KEY_O = 'o',
        KEY_P = 'p',
        KEY_Q = 'q',
        KEY_R = 'r',
        KEY_S = 's',
        KEY_T = 't',
        KEY_U = 'u',
        KEY_V = 'v',
        KEY_W = 'w',
        KEY_X = 'x',
        KEY_Y = 'y',
        KEY_Z = 'z',

        KEY_DELETE = '\x7F',

        KEY_CAPSLOCK = 1073741881,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,

        KEY_PRINTSCREEN = 1073741894,
        KEY_SCROLLLOCK,
        KEY_PAUSE,
        KEY_INSERT,
        KEY_HOME,
        KEY_PAGEUP,
        
        KEY_END = 1073741901,
        KEY_PAGEDOWN,
        KEY_RIGHT,
        KEY_LEFT,
        KEY_DOWN,
        KEY_UP,

        KEY_NUMLOCKCLEAR = 1073741907,
        KEY_KP_DIVIDE ,
        KEY_KP_MULTIPLY,
        KEY_KP_MINUS,
        KEY_KP_PLUS,
        KEY_KP_ENTER,
        KEY_KP_1,
        KEY_KP_2,
        KEY_KP_3,
        KEY_KP_4,
        KEY_KP_5,
        KEY_KP_6,
        KEY_KP_7,
        KEY_KP_8,
        KEY_KP_9,
        KEY_KP_0,
        KEY_KP_PERIOD,

        KEY_APPLICATION = 1073741925,
        KEY_POWER,
        KEY_KP_EQUALS,
        KEY_F13,
        KEY_F14,
        KEY_F15,
        KEY_F16,
        KEY_F17,
        KEY_F18,
        KEY_F19,
        KEY_F20,
        KEY_F21,
        KEY_F22,
        KEY_F23,
        KEY_F24,
        KEY_EXEC,
        KEY_HELP,
        KEY_MENU,
        KEY_SELECT,
        KEY_STOP,
        KEY_AGAIN,
        KEY_UNDO,
        KEY_CUT,
        KEY_COPY,
        KEY_PASTE,
        KEY_FIND,
        KEY_MUTE,
        KEY_VOLUMEUP,
        KEY_VOLUMEDOWN,
        KEY_KP_COMMA = 1073741957,
        KEY_KP_EQUALSAS400,

        KEY_ALTERASE = 1073741977,
        KEY_SYSREQ,
        KEY_CANCEL,
        KEY_CLEAR,
        KEY_PRIOR,
        KEY_RETURN2,
        KEY_SEPARATOR,
        KEY_OUT,
        KEY_OPER,
        KEY_CLEARAGAIN,
        KEY_CRSEL,
        KEY_EXSEL,

        KEY_KP_00 = 1073742000,
        KEY_KP_000,
        KEY_THOUSANDSSEPARATOR,
        KEY_DECIMALSEPARATOR,
        KEY_CURRENCYUNIT,
        KEY_CURRENCYSUBUNIT,
        KEY_KP_LEFTPAREN,
        KEY_KP_RIGHTPAREN,
        KEY_KP_LEFTBRACE,
        KEY_KP_RIGHTBRACE,
        KEY_KP_TAB,
        KEY_KP_BACKSPACE,
        KEY_KP_A,
        KEY_KP_B,
        KEY_KP_C,
        KEY_KP_D,
        KEY_KP_E,
        KEY_KP_F,
        KEY_KP_XOR,
        KEY_KP_POWER,
        KEY_KP_PERCENT,
        KEY_KP_LESS,
        KEY_KP_GREATER,
        KEY_KP_AMPERSAND,
        KEY_KP_DBLAMPERSAND,
        KEY_KP_VERTICALBAR,
        KEY_KP_DBLVERTICALBAR,
        KEY_KP_COLON,
        KEY_KP_HASH,
        KEY_KP_SPACE,
        KEY_KP_AT,
        KEY_KP_EXCLAM,
        KEY_KP_MEMSTORE,
        KEY_KP_MEMRECALL,
        KEY_KP_MEMCLEAR,
        KEY_KP_MEMADD,
        KEY_KP_MEMSUBTRACT,
        KEY_KP_MEMMULTIPLY,
        KEY_KP_MEMDIVIDE,
        KEY_KP_PLUSMINUS,
        KEY_KP_CLEAR,
        KEY_KP_CLEARENTRY,
        KEY_KP_BINARY,
        KEY_KP_OCTAL,
        KEY_KP_DECIMAL,
        KEY_KP_HEXADECIMAL,

        KEY_LCTRL = 1073742048,
        KEY_LSHIFT,
        KEY_LALT,
        KEY_LGUI,
        KEY_RCTRL,
        KEY_RSHIFT,
        KEY_RALT,
        KEY_RGUI,

        KEY_MODE = 1073742081,
        KEY_AUDIONEXT,
        KEY_AUDIOPREV,
        KEY_AUDIOSTOP,
        KEY_AUDIOPLAY,
        KEY_AUDIOMUTE,
        KEY_MEDIASELECT,
        KEY_WWW,
        KEY_MAIL,
        KEY_CALCULATOR,
        KEY_COMPUTER,
        KEY_AC_SEARCH,
        KEY_AC_HOME,
        KEY_AC_BACK,
        KEY_AC_FORWARD,
        KEY_AC_STOP ,
        KEY_AC_REFRESH,
        KEY_AC_BOOKMARKS,

        KEY_BRIGHTNESSDOWN = 1073742099,
        KEY_BRIGHTNESSUP,
        KEY_DISPLAYSWITCH,
        KEY_KBDILLUMTOGGLE,
        KEY_KBDILLUMDOWN,
        KEY_KBDILLUMUP,
        KEY_EJECT,
        KEY_SLEEP

    } Keycode;

    typedef enum Keymod
    {
        KEYMOD_NONE = 0x0000,
        KEYMOD_LSHIFT = 0x0001,
        KEYMOD_RSHIFT = 0x0002,
        KEYMOD_LCTRL = 0x0040,
        KEYMOD_RCTRL = 0x0080,
        KEYMOD_LALT = 0x0100,
        KEYMOD_RALT = 0x0200,
        KEYMOD_LGUI = 0x0400,
        KEYMOD_RGUI = 0x0800,
        KEYMOD_NUM = 0x1000,
        KEYMOD_CAPS = 0x2000,
        KEYMOD_MODE = 0x4000,
        KEYMOD_SCROLL = 0x8000,

        KEYMOD_CTRL = KEYMOD_LCTRL | KEYMOD_RCTRL,
        KEYMOD_SHIFT = KEYMOD_LSHIFT | KEYMOD_RSHIFT,
        KEYMOD_ALT = KEYMOD_LALT | KEYMOD_RALT,
        KEYMOD_GUI = KEYMOD_LGUI | KEYMOD_RGUI,
         
        KEYMOD_RESERVED = KEYMOD_SCROLL
    } Keymod;

    typedef enum Buttoncode : unsigned int
    {
        MOUSE_LEFT = 1,
        MOUSE_MIDDLE = 2,
        MOUSE_RIGHT = 3,
        MOUSE_4 = 4,
        MOUSE_5 = 5
    } Buttoncode;
}