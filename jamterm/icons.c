#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

Cursor deadmouse={
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x0C, 0x00, 0x8E, 0x1D, 0xC7,
	 0xFF, 0xE3, 0xFF, 0xF3, 0xFF, 0xFF, 0x7F, 0xFE, 
	 0x3F, 0xF8, 0x17, 0xF0, 0x03, 0xE0, 0x00, 0x00,},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x82,
	 0x04, 0x41, 0xFF, 0xE1, 0x5F, 0xF1, 0x3F, 0xFE, 
	 0x17, 0xF0, 0x03, 0xE0, 0x00, 0x00, 0x00, 0x00,}
};
Cursor lockarrow={
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x0F, 0xC0, 0x0F, 0xC0,
	 0x03, 0xC0, 0x07, 0xC0, 0x0E, 0xC0, 0x1C, 0xC0,
	 0x38, 0x00, 0x70, 0x00, 0xE0, 0xDB, 0xC0, 0xDB,}
};

void
iconinit(void)
{
}
