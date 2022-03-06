#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

Cursor confirmcursor={
	0, 0,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90,
};

Cursor crosscursor = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, },
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, }
};

Cursor boxcursor = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00, }
};

Cursor sightcursor = {
	{-7, -7},
	{0x1F, 0xF8, 0x3F, 0xFC, 0x7F, 0xFE, 0xFB, 0xDF,
	 0xF3, 0xCF, 0xE3, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xC7, 0xF3, 0xCF,
	 0x7B, 0xDF, 0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8, },
	{0x00, 0x00, 0x0F, 0xF0, 0x31, 0x8C, 0x21, 0x84,
	 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82,
	 0x21, 0x84, 0x31, 0x8C, 0x0F, 0xF0, 0x00, 0x00, }
};

Cursor whitearrow = {
	{0, 0},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC, 
	 0xFF, 0xF0, 0xFF, 0xF0, 0xFF, 0xF8, 0xFF, 0xFC, 
	 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC, 
	 0xF3, 0xF8, 0xF1, 0xF0, 0xE0, 0xE0, 0xC0, 0x40, },
	{0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x06, 0xC0, 0x1C, 
	 0xC0, 0x30, 0xC0, 0x30, 0xC0, 0x38, 0xC0, 0x1C, 
	 0xC0, 0x0E, 0xC0, 0x07, 0xCE, 0x0E, 0xDF, 0x1C, 
	 0xD3, 0xB8, 0xF1, 0xF0, 0xE0, 0xE0, 0xC0, 0x40, }
};

Cursor query = {
	{-7,-7},
	{0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 
	 0x7c, 0x7e, 0x78, 0x7e, 0x00, 0xfc, 0x01, 0xf8, 
	 0x03, 0xf0, 0x07, 0xe0, 0x07, 0xc0, 0x07, 0xc0, 
	 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xc0, },
	{0x00, 0x00, 0x0f, 0xf0, 0x1f, 0xf8, 0x3c, 0x3c, 
	 0x38, 0x1c, 0x00, 0x3c, 0x00, 0x78, 0x00, 0xf0, 
	 0x01, 0xe0, 0x03, 0xc0, 0x03, 0x80, 0x03, 0x80, 
	 0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, }
};

Cursor tl = {
	{-4, -4},
	{0xfe, 0x00, 0x82, 0x00, 0x8c, 0x00, 0x87, 0xff, 
	 0xa0, 0x01, 0xb0, 0x01, 0xd0, 0x01, 0x11, 0xff, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x1f, 0x00, },
	{0x00, 0x00, 0x7c, 0x00, 0x70, 0x00, 0x78, 0x00, 
	 0x5f, 0xfe, 0x4f, 0xfe, 0x0f, 0xfe, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x00, }
};

Cursor t = {
	{-7, -8},
	{0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x06, 0xc0, 
	 0x1c, 0x70, 0x10, 0x10, 0x0c, 0x60, 0xfc, 0x7f, 
	 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
	 0x03, 0x80, 0x0f, 0xe0, 0x03, 0x80, 0x03, 0x80, 
	 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor tr = {
	{-11, -4},
	{0x00, 0x7f, 0x00, 0x41, 0x00, 0x31, 0xff, 0xe1, 
	 0x80, 0x05, 0x80, 0x0d, 0x80, 0x0b, 0xff, 0x88, 
	 0x00, 0x88, 0x0, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0xf8, },
	{0x00, 0x00, 0x00, 0x3e, 0x00, 0x0e, 0x00, 0x1e, 
	 0x7f, 0xfa, 0x7f, 0xf2, 0x7f, 0xf0, 0x00, 0x70, 
	 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, }
};

Cursor r = {
	{-8, -7},
	{0x07, 0xc0, 0x04, 0x40, 0x04, 0x40, 0x04, 0x58, 
	 0x04, 0x68, 0x04, 0x6c, 0x04, 0x06, 0x04, 0x02, 
	 0x04, 0x06, 0x04, 0x6c, 0x04, 0x68, 0x04, 0x58, 
	 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x07, 0xc0, },
	{0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 
	 0x03, 0x90, 0x03, 0x90, 0x03, 0xf8, 0x03, 0xfc, 
	 0x03, 0xf8, 0x03, 0x90, 0x03, 0x90, 0x03, 0x80, 
	 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, }
};

Cursor br = {
	{-11, -11},
	{0x00, 0xf8, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0xff, 0x88, 0x80, 0x0b, 0x80, 0x0d, 0x80, 0x05, 
	 0xff, 0xe1, 0x00, 0x31, 0x00, 0x41, 0x00, 0x7f, },
	{0x00, 0x00, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x0, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x00, 0x70, 0x7f, 0xf0, 0x7f, 0xf2, 0x7f, 0xfa, 
	 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0x00, }
};

Cursor b = {
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 
	 0xfc, 0x7f, 0x0c, 0x60, 0x10, 0x10, 0x1c, 0x70, 
	 0x06, 0xc0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 
	 0x03, 0x80, 0x03, 0x80, 0x0f, 0xe0, 0x03, 0x80, 
	 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor bl = {
	{-4, -11},
	{0x1f, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0xff, 0xd0, 0x01, 0xb0, 0x01, 0xa0, 0x01, 
	 0x87, 0xff, 0x8c, 0x00, 0x82, 0x00, 0xfe, 0x00, },
	{0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0f, 0xfe, 0x4f, 0xfe, 0x5f, 0xfe, 
	 0x78, 0x00, 0x70, 0x00, 0x7c, 0x00, 0x00, 0x0, }
};

Cursor l = {
	{-7, -7},
	{0x03, 0xe0, 0x02, 0x20, 0x02, 0x20, 0x1a, 0x20, 
	 0x16, 0x20, 0x36, 0x20, 0x60, 0x20, 0x40, 0x20, 
	 0x60, 0x20, 0x36, 0x20, 0x16, 0x20, 0x1a, 0x20, 
	 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x03, 0xe0, },
	{0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 
	 0x09, 0xc0, 0x09, 0xc0, 0x1f, 0xc0, 0x3f, 0xc0, 
	 0x1f, 0xc0, 0x09, 0xc0, 0x09, 0xc0, 0x01, 0xc0, 
	 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, }
};

Cursor *corners[9] = {
	&tl,	&t,	&tr,
	&l,	nil,	&r,
	&bl,	&b,	&br,
};

enum {
	Noredraw	= 1,
	Rgbcol	= 2,
	Imagecol	= 3,
};

typedef struct Color Color;

struct Color {
	char *id;
	int type;
	union {
		u32int rgb;
		char *path;
	};
	int flags;
};

static Color theme[Numcolors] = {
	[Colrioback]   = {"rioback",   Rgbcol, {0x777777}, 0},
	[Colback]      = {"back",      Rgbcol, {0xffffff}, 0},
	[Colhigh]      = {"high",      Rgbcol, {0xcccccc}, 0},
	[Colbord]      = {"border",    Rgbcol, {0x999999}, 0},
	[Coltext]      = {"text",      Rgbcol, {DBlack>>8}, 0},
	[Colhtext]     = {"htext",     Rgbcol, {DBlack>>8}, 0},
	[Coltitle]     = {"title",     Rgbcol, {DGreygreen>>8}, 0},
	[Colltitle]    = {"ltitle",    Rgbcol, {DPalegreygreen>>8}, 0},
	[Colhold]      = {"hold",      Rgbcol, {DMedblue>>8}, 0},
	[Collhold]     = {"lhold",     Rgbcol, {DGreyblue>>8}, 0},
	[Colpalehold]  = {"palehold",  Rgbcol, {DPalegreyblue>>8}, 0},
	[Colpaletext]  = {"paletext",  Rgbcol, {0x666666}, 0},
	[Colsize]      = {"size",      Rgbcol, {DRed>>8}, 0},
	[Colmenubar]   = {"menubar",   Rgbcol, {DDarkgreen>>8}, Noredraw},
	[Colmenuback]  = {"menuback",  Rgbcol, {0xeaffea}, Noredraw},
	[Colmenuhigh]  = {"menuhigh",  Rgbcol, {DDarkgreen>>8}, Noredraw},
	[Colmenubord]  = {"menubord",  Rgbcol, {DMedgreen>>8}, Noredraw},
	[Colmenutext]  = {"menutext",  Rgbcol, {DBlack>>8}, Noredraw},
	[Colmenuhtext] = {"menuhtext", Rgbcol, {0xeaffea}, Noredraw},
};

Image *col[Numcolors];

static char *
readall(int f, int *osz)
{
	int bufsz, sz, n;
	char *s;

	bufsz = 1023;
	s = nil;
	for(sz = 0;; sz += n){
		if(bufsz-sz < 1024){
			bufsz *= 2;
			s = realloc(s, bufsz);
		}
		if((n = readn(f, s+sz, bufsz-sz-1)) < 1)
			break;
	}
	if(n < 0 || sz < 1){
		free(s);
		return nil;
	}
	s[sz] = 0;
	*osz = sz;

	return s;
}

void
iconinit(void)
{
	int i, f, sz;
	char *s;

	if((f = open("/dev/theme", OREAD|OCEXEC)) >= 0){
		if((s = readall(f, &sz)) != nil)
			themeload(s, sz);
		free(s);
		close(f);
	}

	for(i = 0; i < nelem(col); i++){
		if(col[i] == nil)
			col[i] = allocimage(display, Rect(0,0,1,1), RGB24, 1, theme[i].rgb<<8|0xff);
	}
}
void redraw(void);
void
themeload(char *s, int n)
{
	int i, fd;
	char *t, *a[2], *e, *newp;
	Image *newc, *repl;
	u32int rgb;

	if((t = malloc(n+1)) == nil)
		return;
	memmove(t, s, n);
	t[n] = 0;

	for(s = t; s != nil && *s; s = e){
		if((e = strchr(s, '\n')) != nil)
			*e++ = 0;
		if(tokenize(s, a, 2) == 2){
			for(i = 0; i < nelem(theme); i++) {
				if(strcmp(theme[i].id, a[0]) == 0) {
					newc = nil;
					if(a[1][0] == '/'){
						if((fd = open(a[1], OREAD)) >= 0){
							if ((newc = readimage(display, fd, 0)) == nil)
								goto End;
							close(fd);
							if ((repl = allocimage(display, Rect(0, 0, Dx(newc->r), Dy(newc->r)), RGB24, 1, 0x000000ff)) == nil)
								goto End;
							if (theme[i].type == Imagecol)
								free(theme[i].path);
							if ((newp = strdup(a[1])) == nil)
								goto End;
							theme[i].type = Imagecol;
							theme[i].path = newp;
							draw(repl, repl->r, newc, 0, newc->r.min);
							freeimage(newc);
							newc = repl;
						}
					}else{
						rgb = strtoul(a[1], nil, 16);
						if((newc = allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, rgb<<8 | 0xff)) != nil) {
							if (theme[i].type == Imagecol)
								free(theme[i].path);
							theme[i].type = Rgbcol;
							theme[i].rgb = rgb;
						}
					}
					if(new != nil){
						freeimage(col[i]);
						col[i] = newc;
					}
					break;
				}
			}
		}
	}
End:
	free(t);
	redraw();
}

char *
themestring(int *n)
{
	char *s, *t, *e;
	int i;

	if((t = malloc(512)) != nil){
		s = t;
		e = s+512;
		for(i = 0; i < nelem(theme); i++)
			if (theme[i].type == Rgbcol)
				s = seprint(s, e, "%s\t%06ux\n", theme[i].id, theme[i].rgb);
			else if (theme[i].type == Imagecol)
				s = seprint(s, e, "%s\t%s\n", theme[i].id, theme[i].path);
		*n = s - t;
	}

	return t;
}
