/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "ogl/defines.h"
#include "ogl/events.h"

OGL_NAMESPACE_BEGIN

void
event_debug_print (ogl::MouseEvent const& event)
{
    std::cout << "Mouse event ";
    switch (event.type)
    {
        case ogl::MOUSE_EVENT_MOVE: std::cout << "MOVE"; break;
        case ogl::MOUSE_EVENT_PRESS: std::cout << "PRESS"; break;
        case ogl::MOUSE_EVENT_RELEASE: std::cout << "RELEASE"; break;
        case ogl::MOUSE_EVENT_WHEEL_UP: std::cout << "WHEELUP"; break;
        case ogl::MOUSE_EVENT_WHEEL_DOWN: std::cout << "WHEELDOWN"; break;
        default: std::cout << "UNKNOWN"; break;
    }

    std::cout << ", button ";
    switch (event.button)
    {
        case ogl::MOUSE_BUTTON_NONE: std::cout << "NONE"; break;
        case ogl::MOUSE_BUTTON_LEFT: std::cout << "LEFT"; break;
        case ogl::MOUSE_BUTTON_RIGHT: std::cout << "RIGHT"; break;
        case ogl::MOUSE_BUTTON_MIDDLE: std::cout << "MIDDLE"; break;
        case ogl::MOUSE_BUTTON_X1: std::cout << "X1BUT"; break;
        case ogl::MOUSE_BUTTON_X2: std::cout << "X2BUT"; break;
        default:std::cout << "UNKNOWN";
    }

    std::cout << " at (" << event.x << "," << event.y << ")" << std::endl;
}

/* ---------------------------------------------------------------- */

void
event_debug_print (ogl::KeyboardEvent const& event)
{
    std::cout << "Keyboard event ";
    switch (event.type)
    {
        case ogl::KEYBOARD_EVENT_PRESS: std::cout << "PRESS"; break;
        case ogl::KEYBOARD_EVENT_RELEASE: std::cout << "RELEASE"; break;
        default: std::cout << "UNKNOWN"; break;
    }

    std::cout << ", keycode " << event.keycode << std::endl;
}

OGL_NAMESPACE_END
