#pragma once

#include "stdafx.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

enum MouseKeys
{
    LButton,
    RButton
};

struct MouseInfo
{
    MouseInfo() = default;

    MouseInfo(long deltaX, long deltaY, long deltaZ, unsigned long absoluteX, unsigned long absoluteY, long clientX, long clientY)
        : deltaX(deltaX)
        , deltaY(deltaY)
        , deltaZ(deltaZ)
        , absoluteX(absoluteX)
        , absoluteY(absoluteY)
        , clientX(clientX)
        , clientY(clientY)
    {}

    long deltaX = 0, deltaY = 0, deltaZ = 0;
    unsigned long absoluteX = 0, absoluteY = 0;
    long clientX = 0, clientY = 0;

    bool operator == (const MouseInfo & ref) const
    {
        return (this->absoluteX == ref.absoluteX) && (this->absoluteY == ref.absoluteY);
    };

    bool operator != (const MouseInfo & ref) const
    {
        return !((this->absoluteX == ref.absoluteX) && (this->absoluteY == ref.absoluteY));
    };
};

class InputManager
{
public:
    InputManager(HWND hWnd, HINSTANCE hInstance);
    void GetKeys();

    bool IsMouseDown(MouseKeys key) const;
    bool IsMouseUp(MouseKeys key) const;
    bool IsMouseMoved() const;

    bool IsKeyDown(unsigned char Key) const;
    bool IsKeyUp(unsigned char Key) const;
    MouseInfo GetMouseInfo() const;

private:

    ComPtr<IDirectInput8> di = nullptr;
    ComPtr<IDirectInputDevice8> keyboard = nullptr;
    ComPtr<IDirectInputDevice8> mouse = nullptr;

    char curKeyBuffer[256];
    char preKeyBuffer[256];

    MouseInfo curMouseBuffer1;
    MouseInfo preMouseBuffer1;

    DIMOUSESTATE curMouseBuffer;
    DIMOUSESTATE preMouseBuffer;

    HWND hwnd = nullptr;
};
