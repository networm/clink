// Copyright (c) 2021 Christopher Antos
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "printer.h"
#include "terminal_out.h"
#include "terminal_helpers.h"
#include "screen_buffer.h"

#include <core/settings.h>

#include <assert.h>

//------------------------------------------------------------------------------
extern bool g_enhanced_cursor;
printer* g_printer = nullptr;
bool g_accept_mouse_input = false;

//------------------------------------------------------------------------------
setting_bool g_adjust_cursor_style(
    "terminal.adjust_cursor_style",
    "Adjusts the cursor visibility and shape",
    "Normally Clink adjusts the cursor visibility and shape, but that will override\n"
    "the Cursor Shape settings for the default Windows console.  Disabling this\n"
    "lets the Cursor Shape settings work, but then Clink can't show Insert Mode via\n"
    "the cursor shape, the 'visible bell' setting doesn't work, Clink can't support\n"
    "the ANSI escape codes for cursor shape, and the cursor may flicker or flash\n"
    "strangely while typing.",
    true);

setting_enum g_mouse_input(
    "terminal.mouse_input",
    "Clink mouse input",
    "Clink can optionally respond to mouse input, instead of letting the terminal\n"
    "respond to mouse input (e.g. to select text on the screen).  When mouse input\n"
    "is enabled in Clink, clicking in the input line sets the cursor position, and\n"
    "clicking in popup lists selects an item, etc.\n"
    "\n"
    "'off' lets the terminal host handle mouse input.\n"
    "'on' lets Clink handle mouse input.\n"
    "'auto' lets Clink handle mouse input in ConEmu and in the default Conhost\n"
    "terminal when Quick Edit mode is unchecked in the console Properties dialog.\n"
    "\n"
    "NOTES:\n"
    "- ConEmu does not let Clink respond to the mouse wheel.\n"
    "- Windows Terminal does not let Clink scroll the terminal, but you can scroll\n"
    "  by holding Shift or Alt while using the mouse wheel.\n"
    "- Holding Shift, Ctrl, or Alt while clicking allows the normal terminal mouse\n"
    "  input to still work (for example, to select text on the screen).",
    "off,on,auto",
    2);

//------------------------------------------------------------------------------
static bool s_locked_cursor_visibility = false;
extern "C" int is_locked_cursor()
{
    return s_locked_cursor_visibility;
}

//------------------------------------------------------------------------------
extern "C" int lock_cursor(int lock)
{
    assert(!lock || !s_locked_cursor_visibility);
    bool was_locked = s_locked_cursor_visibility;
    s_locked_cursor_visibility = !!lock;
    return was_locked;
}

//------------------------------------------------------------------------------
extern "C" int cursor_style(HANDLE handle, int style, int visible)
{
    if (!handle)
        handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(handle, &ci);
    int was_visible = !!ci.bVisible;

    // Assume first encounter of cursor size is the default size.  This only
    // works for Use Legacy Style; the newer cursor shapes all report 25.
    static int g_default_cursor_size = -1;
    static int g_alternate_cursor_size = 100;
    if (g_default_cursor_size < 0)
    {
        g_default_cursor_size = ci.dwSize;
        if (g_default_cursor_size >= 75)
            g_alternate_cursor_size = 50;
    }

    if (is_locked_cursor() || !g_adjust_cursor_style.get())
        return was_visible;
    if (style < 0 && visible < 0)
        return was_visible;

    if (style < 0)
        style = g_enhanced_cursor;
    else
        g_enhanced_cursor = !!style;

    ci.dwSize = style ? g_alternate_cursor_size : g_default_cursor_size;

    if (visible >= 0)
        ci.bVisible = !!visible;

    SetConsoleCursorInfo(handle, &ci);

    return was_visible;
}



//------------------------------------------------------------------------------
static DWORD s_host_input_mode = -1;
static DWORD s_clink_input_mode = -1;

//------------------------------------------------------------------------------
void save_host_input_mode(DWORD mode)
{
    s_host_input_mode = mode;
}

//------------------------------------------------------------------------------
extern "C" void use_host_input_mode(void)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(h, &mode))
            s_clink_input_mode = mode;

        if (s_host_input_mode != -1)
            SetConsoleMode(h, s_host_input_mode);
    }
}

//------------------------------------------------------------------------------
extern "C" void use_clink_input_mode(void)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (s_host_input_mode == -1 && GetConsoleMode(h, &mode))
            s_host_input_mode = mode;

        if (s_clink_input_mode != -1)
            SetConsoleMode(h, s_clink_input_mode);
    }
}



//------------------------------------------------------------------------------
static DWORD select_mouse_input(DWORD mode)
{
    if (!g_accept_mouse_input)
        return 0;

    switch (g_mouse_input.get())
    {
    default:
    case 0:
        // Off.
        break;
    case 1:
        // On.
        switch (get_native_ansi_handler())
        {
        case ansi_handler::conemu:
            mode |= ENABLE_MOUSE_INPUT;
            break;
        default:
            mode &= ~ENABLE_QUICK_EDIT_MODE;
            mode |= ENABLE_MOUSE_INPUT;
            break;
        }
        break;
    case 2:
        // Auto.
        switch (get_native_ansi_handler())
        {
        case ansi_handler::conemu:
            mode |= ENABLE_MOUSE_INPUT;
            break;
        default:
            mode |= (mode & ENABLE_QUICK_EDIT_MODE) ? 0 : ENABLE_MOUSE_INPUT;
            break;
        }
        break;
    }

    return mode;
}

//------------------------------------------------------------------------------
console_config::console_config(HANDLE handle, bool accept_mouse_input)
    : m_handle(handle ? handle : GetStdHandle(STD_INPUT_HANDLE))
{
    extern void save_host_input_mode(DWORD);
    GetConsoleMode(m_handle, &m_prev_mode);
    save_host_input_mode(m_prev_mode);

    m_prev_accept_mouse_input = g_accept_mouse_input;
    g_accept_mouse_input = accept_mouse_input;

    // NOTE:  Windows Terminal doesn't reliably respond to changes of the
    // ENABLE_MOUSE_INPUT flag when ENABLE_AUTO_POSITION is missing.
    DWORD mode = m_prev_mode;
    mode &= ~(ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_MOUSE_INPUT);
    mode |= ENABLE_WINDOW_INPUT;
    mode = select_mouse_input(mode);
    SetConsoleMode(m_handle, mode);
}

console_config::~console_config()
{
    SetConsoleMode(m_handle, m_prev_mode);
    g_accept_mouse_input = m_prev_accept_mouse_input;
}



//------------------------------------------------------------------------------
printer_context::printer_context(terminal_out* terminal, printer* printer)
: m_terminal(terminal)
, m_rb_printer(g_printer)
{
    m_terminal->open();
    m_terminal->begin();

    assert(!g_printer);
    g_printer = printer;
}

//------------------------------------------------------------------------------
printer_context::~printer_context()
{
    m_terminal->end();
    m_terminal->close();
}
