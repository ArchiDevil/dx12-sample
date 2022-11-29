#include "stdafx.h"
#include "InputManager.h"

InputManager::InputManager(HWND hWnd, HINSTANCE hInstance)
{
    this->hwnd = hWnd;

    ThrowIfFailed(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di, nullptr));

    ThrowIfFailed(di->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr));
    ThrowIfFailed(di->CreateDevice(GUID_SysMouse, &mouse, nullptr));

    ThrowIfFailed(keyboard->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
    ThrowIfFailed(mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));

    ThrowIfFailed(keyboard->SetDataFormat(&c_dfDIKeyboard));
    ThrowIfFailed(mouse->SetDataFormat(&c_dfDIMouse));

    ZeroMemory(&curKeyBuffer, sizeof(char) * 256);
    ZeroMemory(&curMouseBuffer, sizeof(curMouseBuffer));
}

void InputManager::GetKeys()
{
    preMouseBuffer = curMouseBuffer;
    memcpy(preKeyBuffer, curKeyBuffer, 256);

    keyboard->Acquire();
    keyboard->GetDeviceState(sizeof(curKeyBuffer), &curKeyBuffer);

    preMouseBuffer1 = curMouseBuffer1;
    curMouseBuffer1 = GetMouseInfo();

    mouse->Acquire();
    mouse->GetDeviceState(sizeof(curMouseBuffer), &curMouseBuffer);
}

MouseInfo InputManager::GetMouseInfo() const
{
    POINT pt;
    POINT pt2;
    GetCursorPos(&pt);
    pt2 = pt;
    ScreenToClient(hwnd, &pt2);
    MouseInfo out(curMouseBuffer.lX, curMouseBuffer.lY, curMouseBuffer.lZ, pt.x, pt.y, pt2.x, pt2.y);
    return out;
}

bool InputManager::IsMouseUp(MouseKeys key) const
{
    if (preMouseBuffer.rgbButtons[(int)key] && !curMouseBuffer.rgbButtons[(int)key])
        return true;
    return false;
}

bool InputManager::IsMouseDown(MouseKeys key) const
{
    if (preMouseBuffer.rgbButtons[(int)key] && curMouseBuffer.rgbButtons[(int)key])
        return true;
    return false;
}

bool InputManager::IsKeyDown(unsigned char Key) const
{
    if (curKeyBuffer[Key] && preKeyBuffer[Key])
        return true;
    return false;
}

bool InputManager::IsKeyUp(unsigned char Key) const
{
    if (!curKeyBuffer[Key] && preKeyBuffer[Key])
        return true;
    return false;
}

bool InputManager::IsMouseMoved() const
{
    if (curMouseBuffer.lX != 0 || curMouseBuffer.lY != 0 || curMouseBuffer.lZ != 0)
        return true;
    return false;
}
