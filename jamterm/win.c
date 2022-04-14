#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include "flayer.h"
#include "samterm.h"

Rectangle
static snaprect(Rectangle r)
{
	return r;
}

/* in case of overlaps, always glob the smallest rect rather than
 * the biggest to maintain splits and selectable flayers */
static Rectangle
stealrect(Point p)
{
	int i;
	Rectangle c, r;
	Flayer *fl;
	Text *t;

	r = screen->r;
	for(i=0; i<nname; i++){
		t = text[i];
		if(t == nil || t->nwin == 0)
			continue;
		for(fl=t->l; fl<t->l+NL; fl++){
			if(fl->textfn == nil
			|| !ptinrect(p, fl->entire))
				continue;
			c = fl->entire;
			/* certain to have clicked on this rect,
			 * and it's on top */
			if(fl->visible == All)
				return c;
			/* otherwise, there are only overlapping
			 * rects, find the smallest one */
			if(Dx(c) * Dy(c) < Dx(r) * Dx(r))
				r = c;
		}
	}
	return r;
}

static Rectangle
fillvoid(Point p, Flayer *l, int new)
{
	int i;
	Rectangle c, r;
	Flayer *fl;
	Text *t;

	r = screen->r;
	for(i=0; i<nname; i++){
		t = text[i];
		if(t == nil || t->nwin == 0)
			continue;
		for(fl=t->l; fl<t->l+NL; fl++){
			c = fl->entire;
			if(fl->textfn == nil
			|| !rectXrect(r, c))
				continue;
			if(!new && fl == l)
				continue;
			if(c.max.x <= p.x && c.max.x > r.min.x)
				r.min.x = c.max.x;
			if(p.x <= c.min.x && c.min.x < r.max.x)
				r.max.x = c.min.x;
			if(!rectXrect(c, r))
				continue;
			if(c.max.y <= p.y && c.max.y > r.min.y)
				r.min.y = c.max.y;
			if(p.y <= c.min.y && c.min.y < r.max.y)
				r.max.y = c.min.y;
		}
		if(Dx(r) < 2*font->width || Dy(r) < font->height)
			return ZR;
	}
	return r;
}

int
promptrect(Rectangle *rp, Flayer *l, int new)
{
	Point p;
	Rectangle r;

	r = getrect(3, mousectl);
	if(eqrect(r, ZR)){
		*rp = r;
		return 0;
	/* around the minimum for libframe to not choke */
	}else if(Dx(r) < 2 * stringwidth(font, "0") + 2 * FLMARGIN
	|| Dy(r) < font->height + 2 * FLMARGIN){
		p = r.min;	/* mousep->xy == r.max */
		if(!ptinrect(p, screen->r))
			return 0;
		r = fillvoid(p, l, new);
		if(eqrect(r, ZR))
			r = stealrect(p);
	}else
		r = snaprect(r);
	*rp = r;
	return 1;
}
