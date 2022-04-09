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

int	mainstacksize = 16*1024;

Text	cmd;
Rune	*scratch;
long	nscralloc;
Cursor	*cursor;
Flayer	*which = 0;
Flayer	*work = 0;
Flayer	flru;
long	snarflen;
long	typestart = -1;
long	typeend = -1;
long	typeesc = -1;
long	modified = 0;		/* strange lookahead for menus */
char	hostlock = 1;
char	hasunlocked = 0;
int	maxtab = 8;
int	autoindent = 1;
int	spacesindent;

static Rectangle
defaultcmdrect(void)
{
	int rw, rh, fw, fh;
	Rectangle r;

	rw = 2 * Dx(screen->r) / 12;
	rh = Dy(screen->r) / 3;
	fw = stringwidth(font, "0");
	fh = font->height;
	if(rw < 20 * fw)
		rw = Dx(screen->r);
	else if(rw > 100 * fw)
		rw = 100 * fw;
	r.min.x = screen->r.max.x - Dx(screen->r) / 2 - rw / 2;
	r.max.x = r.min.x + rw;
	r.min.y = screen->r.min.y;
	r.max.y = r.min.y + rh;
	if(rh > 10 * fh){
		r.max.y = r.min.y + 10 * fh;
		if(rw != Dx(screen->r))
			r = rectaddpt(r, Pt(0, Dy(screen->r) / 2 - rh / 2));
	}else if(Dy(r) < fh + 2*FLMARGIN)
		r.max.y = r.min.y + fh + 2*FLMARGIN;
	return r;
}

void
threadmain(int argc, char *argv[])
{
	int i, got, nclick, scr, chord;
	Text *t;
	Rectangle r;
	Flayer *nwhich;
	ulong p;

	getscreen(argc, argv);
	iconinit();
	initio();
	flru.lprev = &flru;
	flru.lnext = &flru;
	scratch = alloc(100*RUNESIZE);
	nscralloc = 100;
	r = defaultcmdrect();
	rinit(&cmd.rasp);
	flnew(&cmd.l[0], gettext, 1, &cmd);
	flinit(&cmd.l[0], r, font, cmdcols);
	cmd.nwin = 1;
	which = &cmd.l[0];
	cmd.tag = Untagged;
	outTs(Tversion, VERSION);
	startnewfile(Tstartcmdfile, &cmd);
	fmtinstall('P', Pfmt);

	got = 0;
	chord = 0;
	for(;;got = waitforio()){
		if(hasunlocked && RESIZED())
			resize();
		if(got&(1<<RHost))
			rcv();
		if(got&(1<<RPlumb)){
			for(i=0; cmd.l[i].textfn==0; i++)
				;
			current(&cmd.l[i], 0, 1);
			flsetselect(which, cmd.rasp.nrunes, cmd.rasp.nrunes);
			type(which, RPlumb);
		}
		if(got&(1<<RKeyboard))
			if(which)
				type(which, RKeyboard);
			else
				kbdblock();
		if(got&(1<<RMouse)){
			if(hostlock==2 || !ptinrect(mousep->xy, screen->r)){
				mouseunblock();
				continue;
			}
			nwhich = flwhich(mousep->xy);
			if(nwhich && nwhich!=which)
				current(nwhich, 1, 1);
			scr = which && (ptinrect(mousep->xy, which->scroll) ||
				mousep->buttons&(8|16));
			if(mousep->buttons)
				flushtyping(1);
			if((mousep->buttons&1)==0)
				chord = 0;
			if(chord && which && which==nwhich){
				chord |= mousep->buttons;
				t = (Text *)which->user1;
				if(!t->lock){
					int w = which-t->l;
					if(chord&2){
						cut(t, w, 1, 1);
						chord &= ~2;
					}
					if(chord&4){
						paste(t, w);
						chord &= ~4;
					}
				}
			}else if(mousep->buttons&(1|8)){
				if(scr)
					scroll(which, (mousep->buttons&8) ? 4 : 1);
				else if(nwhich && nwhich!=which)
					current(nwhich, 1, 1);
				else{
					t=(Text *)which->user1;
					nclick = flselect(which, &p);
					if(nclick > 0){
						if(nclick > 1)
							outTsl(Ttclick, t->tag, p);
						else
							outTsl(Tdclick, t->tag, p);
						t->lock++;
					}else if(t!=&cmd)
						outcmd();
					if(mousep->buttons&1)
						chord = mousep->buttons;
				}
			}else if((mousep->buttons&2) && which){
				if(scr)
					scroll(which, 2);
				else
					menu2hit();
			}else if(mousep->buttons&(4|16)){
				if(scr)
					scroll(which, (mousep->buttons&16) ? 5 : 3);
				else
					menu3hit();
			}
			mouseunblock();
		}
	}
}


void
resize(void)
{
	int i;

	flresize(screen->clipr);
	for(i = 0; i<nname; i++)
		if(text[i])
			hcheck(text[i]->tag);
}

static void
flunlink(Flayer *fl)
{
	if(fl->lnext == nil || fl->lnext == fl)
		return;
	fl->lnext->lprev = fl->lprev;
	fl->lprev->lnext = fl->lnext;
	fl->lnext = nil;
	fl->lprev = nil;
}

static void
fllinkhead(Flayer *fl)
{
	flunlink(fl);
	fl->lnext = flru.lnext;
	fl->lprev = &flru;
	fl->lnext->lprev = fl;
	fl->lprev->lnext = fl;
}

void
warpmouse(Flayer *l)
{
	Point p;

	if(l == nil || ptinrect(mousectl->xy, l->entire))
		return;
	p = addpt(screen->r.min, l->warpto);
	if(eqpt(p, ZP) || !ptinrect(p, l->entire)){
		p = addpt(l->entire.min, divpt(subpt(l->entire.max, l->entire.min), 2));
		l->warpto = p;
	}
	moveto(mousectl, p);
}

void
current(Flayer *nw, int warp, int up)
{
	Text *t;

	if(which){
		flborder(which, 0);
		if(warp && ptinrect(mousectl->xy, insetrect(which->entire, 4)))
			which->warpto = subpt(mousectl->xy, screen->r.min);
	}
	if(nw){
		flushtyping(1);
		flupfront(nw);
		flborder(nw, 1);
		//buttons(Up);
		t = (Text *)nw->user1;
		t->front = nw-&t->l[0];
		if(t != &cmd){
			work = nw;
			if(up)
				fllinkhead(nw);
		}
		if(warp)
			warpmouse(nw);
	}
	which = nw;
}

void
closeup(Flayer *l)
{
	Text *t=(Text *)l->user1;
	int i, m;

	m = whichmenu(t->tag);
	if(m < 0)
		return;
	flunlink(l);
	flclose(l);
	if(l == which){
		which = nil;
		for(i=0; cmd.l[i].textfn==0; i++)
			;
		current(&cmd.l[i], 1, 1);
	}
	if(l == work)
		work = 0;
	if(--t->nwin == 0){
		rclear(&t->rasp);
		free((uchar *)t);
		text[m] = 0;
	}else if(l == &t->l[t->front]){
		for(m=0; m<NL; m++)	/* find one; any one will do */
			if(t->l[m].textfn){
				t->front = m;
				return;
			}
		panic("close");
	}
}

Flayer *
findl(Text *t)
{
	int i;
	for(i = 0; i<NL; i++)
		if(t->l[i].textfn==0)
			return &t->l[i];
	return 0;
}

void
duplicate(Flayer *l, Rectangle r, Font *f, int close)
{
	Text *t=(Text *)l->user1;
	Flayer *nl = findl(t);
	Rune *rp;
	ulong n;

	if(nl){
		flnew(nl, gettext, l->user0, (char *)t);
		flinit(nl, r, f, l->f.cols);
		nl->origin = l->origin;
		rp = (*l->textfn)(l, l->f.nchars, &n);
		flinsert(nl, rp, rp+n, l->origin);
		flsetselect(nl, l->p0, l->p1);
		if(close){
			flclose(l);
			flunlink(l);
			if(l==which)
				which = 0;
		}else
			t->nwin++;
		current(nl, 0, 1);
		hcheck(t->tag);
	}
	setcursor(mousectl, cursor);
}

void
buttons(int updown)
{
	while(((mousep->buttons&7)!=0) != updown)
		getmouse();
}

Rectangle
expandempty(Point p, Flayer *l, int new)
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
			|| fl->visible == None
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
		if(Dx(r) < 16*font->width && Dy(r) < 4*font->height)
			return inflatepoint(p);
	}
	return r;
}

Rectangle
inflatepoint(Point p)
{
	Rectangle *c;
	Rectangle r;
	
	r = screen->r;
	c = &cmd.l[cmd.front].entire;
	// L
	if(p.x < c->min.x)
		r.max.x = c->min.x;
	// R
	else if(p.x >= c->max.x)
		r.min.x = c->max.x;
	// M
	else{
		r.min.x = c->min.x;
		//r.max.x = screen->max.x;
		// A
		if(p.y <= c->min.y)
			r.max.y = c->min.y;
		// B
		else
			r.min.y = c->max.y;
	}
	return r;
}

int
promptrect(Rectangle *r, Flayer *l, int new)
{
	*r = getrect(3, mousectl);
	if(eqrect(*r, ZR))
		return 0;
	if(ptinrect(mousep->xy, screen->r))
		*r = expandempty(mousep->xy, l, new);
	if(Dx(*r) < 2*FLMARGIN || Dy(*r) < 2*FLMARGIN)
		*r = cmd.l[cmd.front].entire;
	return 1;
}

void
snarf(Text *t, int w)
{
	Flayer *l = &t->l[w];

	if(l->p1>l->p0){
		snarflen = l->p1-l->p0;
		outTsll(Tsnarf, t->tag, l->p0, l->p1);
	}
}

void
cut(Text *t, int w, int save, int check)
{
	long p0, p1;
	Flayer *l;

	l = &t->l[w];
	p0 = l->p0;
	p1 = l->p1;
	if(p0 == p1)
		return;
	if(p0 < 0)
		panic("cut");
	if(save)
		snarf(t, w);
	outTsll(Tcut, t->tag, p0, p1);
	flsetselect(l, p0, p0);
	t->lock++;
	hcut(t->tag, p0, p1-p0);
	if(check)
		hcheck(t->tag);
}

void
paste(Text *t, int w)
{
	cut(t, w, 0, 0);
	t->lock++;
	outTsl(Tpaste, t->tag, t->l[w].p0);
}

void
scrorigin(Flayer *l, int but, long p0)
{
	Text *t=(Text *)l->user1;

	if(t->tag == Untagged)
		return;

	switch(but){
	case 1:
		outTsll(Torigin, t->tag, l->origin, p0);
		break;
	case 2:
		outTsll(Torigin, t->tag, p0, 1L);
		break;
	case 3:
		horigin(t->tag,p0);
	}
}

int
alnum(int c)
{
	/*
	 * Hard to get absolutely right.  Use what we know about ASCII
	 * and assume anything above the Latin control characters is
	 * potentially an alphanumeric.
	 */
	if(c<=' ')
		return 0;
	if(0x7F<=c && c<=0xA0)
		return 0;
	if(utfrune("!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", c))
		return 0;
	return 1;
}

int
raspc(Rasp *r, long p)
{
	ulong n;
	rload(r, p, p+1, &n);
	if(n)
		return scratch[0];
	return 0;
}

int
getcol(Rasp *r, long p)
{
	int col;

	for(col = 0; p > 0 && raspc(r, p-1)!='\n'; p--, col++)
		;
	return col;
}

long
del(Rasp *r, long o, long p)
{
	int i, col, n;

	if(--p < o)
		return o;
	if(!spacesindent || raspc(r, p)!=' ')
		return p;
	col = getcol(r, p) + 1;
	if((n = col % maxtab) == 0)
		n = maxtab;
	for(i = 0; p-1>=o && raspc(r, p-1)==' ' && i<n-1; --p, i++)
		;
	return p>=o? p : o;
}

long
ctlw(Rasp *r, long o, long p)
{
	int c;

	if(--p < o)
		return o;
	if(raspc(r, p)=='\n')
		return p;
	for(; p>=o && !alnum(c=raspc(r, p)); --p)
		if(c=='\n')
			return p+1;
	for(; p>o && alnum(raspc(r, p-1)); --p)
		;
	return p>=o? p : o;
}

long
ctlu(Rasp *r, long o, long p)
{
	if(--p < o)
		return o;
	if(raspc(r, p)=='\n')
		return p;
	for(; p-1>=o && raspc(r, p-1)!='\n'; --p)
		;
	return p>=o? p : o;
}

int
center(Flayer *l, long a)
{
	Text *t;

	t = l->user1;
	if(!t->lock && (a<l->origin || l->origin+l->f.nchars<a)){
		if(a > t->rasp.nrunes)
			a = t->rasp.nrunes;
		outTsll(Torigin, t->tag, a, 2L);
		return 1;
	}
	return 0;
}

int
onethird(Flayer *l, long a)
{
	Text *t;
	Rectangle s;
	long lines;

	t = l->user1;
	if(!t->lock && (a<l->origin || l->origin+l->f.nchars<a)){
		if(a > t->rasp.nrunes)
			a = t->rasp.nrunes;
		s = insetrect(l->scroll, 1);
		lines = ((s.max.y-s.min.y)/l->f.font->height+1)/3;
		if (lines < 2)
			lines = 2;
		outTsll(Torigin, t->tag, a, lines);
		return 1;
	}
	return 0;
}

void
flushtyping(int clearesc)
{
	Text *t;
	ulong n;

	if(clearesc)
		typeesc = -1;	
	if(typestart == typeend) {
		modified = 0;
		return;
	}
	t = which->user1;
	if(t != &cmd)
		modified = 1;
	rload(&t->rasp, typestart, typeend, &n);
	scratch[n] = 0;
	if(t==&cmd && typeend==t->rasp.nrunes && scratch[typeend-typestart-1]=='\n'){
		setlock();
		outcmd();
	}
	outTslS(Ttype, t->tag, typestart, scratch);
	typestart = -1;
	typeend = -1;
}

int
nontypingkey(int c)
{
	switch(c){
	case Kup:
	case Kdown:
	case Khome:
	case Kend:
	case Kpgdown:
	case Kpgup:
	case Kleft:
	case Kright:
	case Ksoh:
	case Kenq:
	case Kstx:
	case Kbel:
		return 1;
	}
	return 0;
}


void
type(Flayer *l, int res)	/* what a bloody mess this is */
{
	Text *t = (Text *)l->user1;
	Rune buf[100];
	Rune *p = buf;
	int c, backspacing;
	long a, a0;
	int scrollkey;

	scrollkey = 0;
	if(res == RKeyboard)
		scrollkey = nontypingkey(qpeekc());	/* ICK */

	if(hostlock || t->lock){
		kbdblock();
		return;
	}
	a = l->p0;
	if(a!=l->p1 && !scrollkey){
		flushtyping(1);
		cut(t, t->front, 1, 1);
		return;	/* it may now be locked */
	}
	backspacing = 0;
	while((c = kbdchar())>0){
		if(res == RKeyboard){
			if(nontypingkey(c) || c==Kesc)
				break;
			/* backspace, ctrl-u, ctrl-w, del */
			if(c==Kbs || c==Knack || c==Ketb || c==Kdel){
				backspacing = 1;
				break;
			}
		}
		if(spacesindent && c == '\t'){
			int i, col, n;
			col = getcol(&t->rasp, a);
			n = maxtab - col % maxtab;
			for(i = 0; i < n && p < buf+nelem(buf); i++)
				*p++ = ' ';
		} else
			*p++ = c;
		if(autoindent)
		if(c == '\n'){
			/* autoindent */
			int cursor, ch;
			cursor = ctlu(&t->rasp, 0, a+(p-buf)-1);
			while(p < buf+nelem(buf)){
				ch = raspc(&t->rasp, cursor++);
				if(ch == ' ' || ch == '\t')
					*p++ = ch;
				else
					break;
			}
		}
		if(c == '\n' || p >= buf+sizeof(buf)/sizeof(buf[0]))
			break;
	}
	if(p > buf){
		if(typestart < 0)
			typestart = a;
		if(typeesc < 0)
			typeesc = a;
		hgrow(t->tag, a, p-buf, 0);
		t->lock++;	/* pretend we Trequest'ed for hdatarune*/
		hdatarune(t->tag, a, buf, p-buf);
		a += p-buf;
		l->p0 = a;
		l->p1 = a;
		typeend = a;
		if(c=='\n' || typeend-typestart>100)
			flushtyping(0);
		onethird(l, a);
	}
	if(c==Kdown){
		flushtyping(0);
		scrorigin(l, 3, l->origin+frcharofpt(&l->f, Pt(l->scroll.max.x, l->scroll.min.y + l->f.font->height)));
	}else if(c==Kup){
		flushtyping(0);
		scrorigin(l, 1, 2);
	}else if(c==Kpgdown){
		flushtyping(0);
		scrorigin(l, 3, l->origin+frcharofpt(&l->f, Pt(l->scroll.max.x, l->scroll.max.y - l->f.font->height)));
		/* backspacing immediately after outcmd(): sorry */
	}else if(c==Kpgup){
		flushtyping(0);
		scrorigin(l, 1, Dy(l->scroll)/l->f.font->height);
	}else if(c == Kright){
		flushtyping(0);
		a0 = l->p0;
		if(a0 < t->rasp.nrunes)
			a0++;
		flsetselect(l, a0, a0);
		center(l, a0);
	}else if(c == Kleft){
		flushtyping(0);
		a0 = l->p0;
		if(a0 > 0)
			a0--;
		flsetselect(l, a0, a0);
		center(l, a0);
	}else if(c == Khome){
		flushtyping(0);
		center(l, 0);
	}else if(c == Kend){
		flushtyping(0);
		center(l, t->rasp.nrunes);
	}else if(c == Ksoh || c == Kenq){
		flushtyping(1);
		if(c == Ksoh)
			while(a > 0 && raspc(&t->rasp, a-1)!='\n')
				a--;
		else
			while(a < t->rasp.nrunes && raspc(&t->rasp, a)!='\n')
				a++;
		l->p0 = l->p1 = a;
		for(l=t->l; l<&t->l[NL]; l++)
			if(l->textfn)
				flsetselect(l, l->p0, l->p1);
	}else if(backspacing && !hostlock){
		/* backspacing immediately after outcmd(): sorry */
		if(l->f.p0>0 && a>0){
			switch(c){
			case Kbs:
			case Kdel:	/* del */
				l->p0 = del(&t->rasp, l->origin, a);
				break;
			case Knack:	/* ctrl-u */
				l->p0 = ctlu(&t->rasp, l->origin, a);
				break;
			case Ketb:	/* ctrl-w */
				l->p0 = ctlw(&t->rasp, l->origin, a);
				break;
			}
			l->p1 = a;
			if(l->p1 != l->p0){
				/* cut locally if possible */
				if(typestart<=l->p0 && l->p1<=typeend){
					t->lock++;	/* to call hcut */
					hcut(t->tag, l->p0, l->p1-l->p0);
					/* hcheck is local because we know rasp is contiguous */
					hcheck(t->tag);
				}else{
					flushtyping(0);
					cut(t, t->front, 0, 1);
				}
			}
			if(typeesc >= l->p0)
				typeesc = l->p0;
			if(typestart >= 0){
				if(typestart >= l->p0)
					typestart = l->p0;
				typeend = l->p0;
				if(typestart == typeend){
					typestart = -1;
					typeend = -1;
					modified = 0;
				}
			}
		}
	}else if(c == Kstx){
		t = &cmd;
		for(l=t->l; l->textfn==0; l++)
			;
		current(l, 1, 1);
		flushtyping(0);
		a = t->rasp.nrunes;
		flsetselect(l, a, a);
		center(l, a);
 	}else if(c == Kbel){
 		int up = 1;
		t = &cmd;
		if(work != nil){
			if(which != work){
				current(work, 1, up);
				return;
			}
	 		t = (Text*)work->user1;
			l = &t->l[t->front];
		}
		if(t == &cmd || t->nwin == 1 && nname > 1){
			if(flru.lnext == &flru)
				return;
			for(l=work!=nil?work->lnext:flru.lnext; l==&flru; l=l->lnext)
				;
			up = 0;
		}else{
	 		for(int i=t->front; (i = (i+1)%NL) != t->front; )
	 			if(t->l[i].textfn != 0){
	 				l = &t->l[i];
	 				break;
	 			}
		}
		current(l, 1, up);
	}else{
		if(c==Kesc && typeesc>=0){
			l->p0 = typeesc;
			l->p1 = a;
			flushtyping(1);
		}
		for(l=t->l; l<&t->l[NL]; l++)
			if(l->textfn)
				flsetselect(l, l->p0, l->p1);
	}
}


void
outcmd(void){
	if(work)
		outTsll(Tworkfile, ((Text *)work->user1)->tag, work->p0, work->p1);
}

void
panic(char *s)
{
	panic1(display, s);
}

void
panic1(Display*, char *s)
{
	fprint(2, "jamterm: panic: ");
	perror(s);
	abort();
}

Rune*
gettext(Flayer *l, long n, ulong *np)
{
	Text *t;

	t = l->user1;
	rload(&t->rasp, l->origin, l->origin+n, np);
	return scratch;
}

long
scrtotal(Flayer *l)
{
	return ((Text *)l->user1)->rasp.nrunes;
}

void*
alloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		panic("alloc");
	memset(p, 0, n);
	return p;
}
