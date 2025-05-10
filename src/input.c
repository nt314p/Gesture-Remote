#include "input.h"
#include "hal_types.h"
#include "gatt.h"
#include "hiddev.h"
#include "hidkbmservice.h"

#define HID_KEYBOARD_IN_RPT_LEN 8
#define HID_MOUSE_IN_RPT_LEN 5

#define MOUSE_BUTTON_LEFT 1 << 0
#define MOUSE_BUTTON_MIDDLE 1 << 2
#define MOUSE_BUTTON_RIGHT 1 << 1

static void InputMouse(uint8 buttons, int8 dx, int8 dy, int8 dz)
{
    uint8 buf[HID_MOUSE_IN_RPT_LEN];

    buf[0] = buttons;
    buf[1] = dx;
    buf[2] = dy;
    buf[3] = dz; // mouse wheel
    buf[4] = 0; // apparently AC pan but could be anything

    HidDev_Report(HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buf);
}

// TODO: coalesce movement and button reports?

// Sends a relative mouse movement
void InputMouseMove(int8 dx, int8 dy, int8 dz)
{
    InputMouse(0, dx, dy, dz);
}

// Sends a mouse button down (or up)
void InputMouseButtons(uint8 buttons)
{
    InputMouse(buttons, 0, 0, 0);
}

static void InputKeyboardKeycode(uint8 modifier, uint8 keycode)
{
    uint8 buf[HID_KEYBOARD_IN_RPT_LEN];

    buf[0] = modifier;
    buf[1] = 0; // reserved
    buf[2] = keycode;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    HidDev_Report(HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buf);

    // Send null report for key upstroke
    buf[0] = 0;
    buf[2] = 0;
    HidDev_Report(HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buf);
}

bool IsDigit(uint8 c)
{
    return c >= '0' && c <= '9';
}

// Converts a non-alphanumeric ascii char to an HID keycode
uint8 SpecialAsciiToKeycode(uint8 c)
{
    switch (c)
    {
    case ' ':
        return 0x2C;
    case '!':
        return 0x1E;
    case '@':
        return 0x1F;
    case '#':
        return 0x20;
    case '$':
        return 0x21;
    case '%':
        return 0x22;
    case '^':
        return 0x23;
    case '&':
        return 0x24;
    case '*':
        return 0x25;
    case '(':
        return 0x26;
    case ')':
        return 0x27;
        // TODO: rest of these. probably need to return shift modifer too
    }

    return 0x2C; // Space for unimplemented or unrecognized char
}

// Sends a key press of the input key character
void InputKeyboard(uint8 character)
{
    bool capitalized = character >= 'A' && character <= 'Z';

    if (capitalized) character -= 32; // uncapitalize

    if (character >= 'a' && character <= 'z')
    {
        // Apply left shift
        // Letter keycodes begin at a = 4
        InputKeyboardKeycode(capitalized ? 2 : 0, (character - 'a') + 4);
        return;
    }

    if (IsDigit(character))
    {
        if (character == '0') character += 10; // move zero to the end

        // Number keybocdes begin at 1 = 0x1E
        InputKeyboardKeycode(0, character - '1' + 0x1E);
        return;
    }

    uint8 keycode = SpecialAsciiToKeycode(character);

    capitalized = keycode >= 0x1E && keycode <= 0x27; // Special chars above numbers
    InputKeyboardKeycode(capitalized ? 2 : 0, keycode);
}


