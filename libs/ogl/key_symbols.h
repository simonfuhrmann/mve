/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_KEYSYMS_HEADER
#define OGL_KEYSYMS_HEADER

#define USE_QT_BINDINGS 1

#if USE_QT_BINDINGS

enum Key
{
    KEY_ESCAPE = 0x01000000,                // misc keys
    KEY_TAB = 0x01000001,
    KEY_BACKTAB = 0x01000002,
    KEY_BACKSPACE = 0x01000003,
    KEY_RETURN = 0x01000004,
    KEY_ENTER = 0x01000005,
    KEY_INSERT = 0x01000006,
    KEY_DELETE = 0x01000007,
    KEY_PAUSE = 0x01000008,
    KEY_PRINT = 0x01000009,
    KEY_SYSREQ = 0x0100000a,
    KEY_CLEAR = 0x0100000b,
    KEY_HOME = 0x01000010,                // cursor movement
    KEY_END = 0x01000011,
    KEY_LEFT = 0x01000012,
    KEY_UP = 0x01000013,
    KEY_RIGHT = 0x01000014,
    KEY_DOWN = 0x01000015,
    KEY_PAGEUP = 0x01000016,
    KEY_PAGEDOWN = 0x01000017,
    KEY_SHIFT = 0x01000020,                // modifiers
    KEY_CONTROL = 0x01000021,
    KEY_META = 0x01000022,
    KEY_ALT = 0x01000023,
    KEY_CAPSLOCK = 0x01000024,
    KEY_NUMLOCK = 0x01000025,
    KEY_SCROLLLOCK = 0x01000026,
    KEY_F1 = 0x01000030,                // function keys
    KEY_F2 = 0x01000031,
    KEY_F3 = 0x01000032,
    KEY_F4 = 0x01000033,
    KEY_F5 = 0x01000034,
    KEY_F6 = 0x01000035,
    KEY_F7 = 0x01000036,
    KEY_F8 = 0x01000037,
    KEY_F9 = 0x01000038,
    KEY_F10 = 0x01000039,
    KEY_F11 = 0x0100003a,
    KEY_F12 = 0x0100003b,
    KEY_F13 = 0x0100003c,
    KEY_F14 = 0x0100003d,
    KEY_F15 = 0x0100003e,
    KEY_F16 = 0x0100003f,
    KEY_F17 = 0x01000040,
    KEY_F18 = 0x01000041,
    KEY_F19 = 0x01000042,
    KEY_F20 = 0x01000043,
    KEY_F21 = 0x01000044,
    KEY_F22 = 0x01000045,
    KEY_F23 = 0x01000046,
    KEY_F24 = 0x01000047,
    KEY_SUPER_L = 0x01000053,                 // extra keys
    KEY_SUPER_R = 0x01000054,
    KEY_MENU = 0x01000055,
    KEY_HYPER_L = 0x01000056,
    KEY_HYPER_R = 0x01000057,
    KEY_HELP = 0x01000058,
    KEY_DIRECTIOn_L = 0x01000059,
    KEY_DIRECTIOn_R = 0x01000060,
    KEY_SPACE = 0x20,                // 7 bit printable ASCII
    KEY_EXCLAM = 0x21,
    KEY_QUOTEDBL = 0x22,
    KEY_NUMBERSIGN = 0x23,
    KEY_DOLLAR = 0x24,
    KEY_PERCENT = 0x25,
    KEY_AMPERSAND = 0x26,
    KEY_APOSTROPHE = 0x27,
    KEY_PARENLEFT = 0x28,
    KEY_PARENRIGHT = 0x29,
    KEY_ASTERISK = 0x2a,
    KEY_PLUS = 0x2b,
    KEY_COMMA = 0x2c,
    KEY_MINUS = 0x2d,
    KEY_PERIOD = 0x2e,
    KEY_SLASH = 0x2f,
    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39,
    KEY_COLON = 0x3a,
    KEY_SEMICOLON = 0x3b,
    KEY_LESS = 0x3c,
    KEY_EQUAL = 0x3d,
    KEY_GREATER = 0x3e,
    KEY_QUESTION = 0x3f,
    KEY_AT = 0x40,
    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4a,
    KEY_K = 0x4b,
    KEY_L = 0x4c,
    KEY_M = 0x4d,
    KEY_N = 0x4e,
    KEY_O = 0x4f,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5a,
    KEY_BRACKETLEFT = 0x5b,
    KEY_BACKSLASH = 0x5c,
    KEY_BRACKETRIGHT = 0x5d,
    KEY_ASCIICIRCUM = 0x5e,
    KEY_UNDERSCORE = 0x5f,
    KEY_QUOTELEFT = 0x60,
    KEY_BRACELEFT = 0x7b,
    KEY_BAR = 0x7c,
    KEY_BRACERIGHT = 0x7d,
    KEY_ASCIITILDE = 0x7e
};

#endif /* USE_QT_BINDINGS */

#endif /* OGL_KEYSYMS_HEADER */
