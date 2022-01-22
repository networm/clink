// Copyright (c) 2022 Christopher Antos
// License: http://opensource.org/licenses/MIT

#pragma once

#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#ifdef DEBUG
#define USE_MEMORY_TRACKING
#define INCLUDE_CALLSTACKS
#define USE_RTTI
#endif

//------------------------------------------------------------------------------
// Define FISH_ARROW_KEYS to make arrow keys move as fish shell completion.
// Otherwise arrow keys move as in powershell completion.
#define FISH_ARROW_KEYS

//------------------------------------------------------------------------------
// Define to use "..." rather than "…" when truncating things.
//#define USE_ASCII_ELLIPSIS

//------------------------------------------------------------------------------
// Debugging options.
#ifdef DEBUG
//#define SHOW_DISPLAY_GENERATION
//#define DEBUG_SUGGEST
//#define DEBUG_RESOLVEIMPL
//#define USE_OS_UTF_CONVERSION
#endif