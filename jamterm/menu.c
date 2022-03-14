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

uchar	**name;	/* first byte is ' ' or '\'': modified state */
Text	**text;	/* pointer to Text associated with file */
ushort	*tag;		/* text[i].tag, even if text[i] not defined */
int	nname;
int	mname;
int	mw;

char	*genmenu3(int);
char	*genmenu2(int);
char	*genmenu2c(int);

enum Menu2
{
	Cut,
	Paste,
	Snarf,
	Plumb,
	Look,
	Search,
	Push,
	Pop,
	NMENU2,
	Send = Search,
	NMENU2C,
};

enum Menu3
{
	New,
	Zerox,
	Resize,
	Close,
	Write,
	NMENU3
};

char	*menu2str[] = {
	"cut",
	"paste",
	"snarf",
	"plumb",
	"look",
	nil,		/* storage for last pattern */
	"push",
	"pop",
};

int	ncmd;
int	ncbuf;
char	**cmds;
char	**clabels;

char	*menu3str[] = {
	"new",
	"zerox",
	"resize",
	"close",
	"write",
};

Menu	menu2 =	{0, genmenu2};
Menu	menu2c ={0, genmenu2c};
Menu	menu3 =	{0, genmenu3};

extern int kekfd[2];

void
menu2hit(void)
{
	char sbuf[256];
	Text *t=(Text *)which->user1;
	int w = which-t->l;
	int m;
	Point p;
	Menu *menu;

	if(hversion==0 || plumbfd<0)
		menu2str[Plumb] = "(plumb)";
	p = subpt(mousectl->xy, screen->r.min);
	menu = t == &cmd ? &menu2c : &menu2;
	menu->lasthit = 0;
	m = menuhit(2, mousectl, menu, nil);
	if(hostlock || t->lock)
		return;
	if(which != nil){
		which->warpto = p;
		warpmouse(which);
	}

	switch(m){
	case Cut:
		cut(t, w, 1, 1);
		break;

	case Paste:
		paste(t, w);
		break;

	case Snarf:
		snarf(t, w);
		break;

	case Plumb:
		if(hversion > 0)
			outTsll(Tplumb, t->tag, which->p0, which->p1);
		break;

	case Look:
		outTsll(Tlook, t->tag, which->p0, which->p1);
		setlock();
		break;

	case Search:
		if(menu2str[Search] == nil && t != &cmd)
			break;
		outcmd();
		if(t==&cmd)
			outTsll(Tsend, 0 /*ignored*/, which->p0, which->p1);
		else
			outT0(Tsearch);
		setlock();
		break;

	case Push:
		if(t == &cmd)
			break;
		memset(sbuf, 0, sizeof sbuf);
		if(enter(nil, sbuf, sizeof sbuf, mousectl, keyboardctl, nil) < 0
		|| strlen(sbuf) == 0)
			break;
		if(ncmd >= ncbuf){
			if((cmds = realloc(cmds, (ncbuf+1) * sizeof *cmds)) == nil)
				panic("realloc");
			if((clabels = realloc(clabels, (ncbuf+1) * sizeof *clabels)) == nil)
				panic("realloc");
			ncbuf++;
		}
		if((cmds[ncmd] = strdup(sbuf)) == nil)
			panic("strdup");
		if((clabels[ncmd] = mallocz(25, 1)) == nil)
			panic("mallocz");
		if(snprint(clabels[ncmd], 24, "%s", sbuf) >= 24-3)
			snprint(clabels[ncmd]+24-3, 3, "...");
		ncmd++;
		break;

	case Pop:
		if(t == &cmd || ncmd <= 0)
			break;
		ncmd--;
		free(cmds[ncmd]);
		cmds[ncmd] = nil;
		break;

	default:
		m -= NMENU2;
		if(m < 0 || m >= ncmd)
			break;
		memset(sbuf, 0, sizeof sbuf);
		w = snprint(sbuf, sizeof sbuf, "%s\n", cmds[m]);
		if(write(kekfd[1], sbuf, w + 1) != w + 1)
			fprint(2, "jamterm: %r\n");
		break;
	}
}

void
menu3hit(void)
{
	Rectangle r;
	Flayer *l;
	int m, i;
	Text *t;

	l = which;
	mw = -1;
	if(l != nil){
		t = (Text *)l->user1;
		menu3.lasthit = whichmenu(t->tag) + NMENU3;
	}
	switch(m = menuhit(3, mousectl, &menu3, nil)){
	case -1:
		break;

	case New:
		if(!hostlock)
			sweeptext(1, 0);
		break;

	case Zerox:
	case Resize:
		if(hostlock || l == nil)
			break;
		if(promptrect(&r))
			duplicate(l, r, l->f.font, m == Resize);
		break;

	case Close:
		if(hostlock || l == nil)
			break;
		t=(Text *)l->user1;
		if (t->nwin>1)
			closeup(l);
		else if(t!=&cmd) {
			outTs(Tclose, t->tag);
			setlock();
		}
		break;

	case Write:
		if(hostlock || l == nil)
			break;
		outTs(Twrite, ((Text *)l->user1)->tag);
		setlock();
		break;

	default:
		if(t = text[m-NMENU3]){
			i = t->front;
			if(t->nwin==0 || t->l[i].textfn==0)
				return;	/* not ready yet; try again later */
			if(t->nwin>1 && which==&t->l[i])
				do
					if(++i==NL)
						i = 0;
				while(i!=t->front && t->l[i].textfn==0);
			if(&t->l[i] != which)
				current(&t->l[i], 1, 1);
		}else if(!hostlock)
			sweeptext(0, tag[m-NMENU3]);
		break;
	}
}

Rectangle
stealrect(Point p)
{
	int i;
	Rectangle *c, *r;
	Flayer *fl;
	Text *t;

	for(i=1, r=nil; i<nname; i++){
		t = text[i];
		if(t == nil || t->nwin == 0)
			continue;
		fl = t->l + t->front;
		c = &fl->entire;
		if(fl->textfn == nil || fl->visible == None || !ptinrect(p, *c))
			continue;
		if(fl->visible == All)
			return *c;
		/* hit-or-miss */
		if(r == nil || Dx(*c) < Dx(*r) || Dy(*c) < Dy(*r))
			r = c;
	}
	return r != nil ? *r : inflatepoint(mousep->xy);
}

Text *
sweeptext(int new, int tag)
{
	Rectangle r;
	Text *t;

	if((t = mallocz(sizeof(*t), 1)) == nil)
		return nil;
	r = stealrect(mousep->xy);
	if(Dx(r) < 2*FLMARGIN || Dy(r) < 2*FLMARGIN)
		r = cmd.l[cmd.front].entire;
	current((Flayer *)0, 0, 0);
	flnew(&t->l[0], gettext, 0, (char *)t);
	flinit(&t->l[0], r, font, maincols);	/*bnl*/
	t->nwin = 1;
	rinit(&t->rasp);
	if(new)
		startnewfile(Tstartnewfile, t);
	else{
		rinit(&t->rasp);
		t->tag = tag;
		startfile(t);
	}
	return t;
}

int
whichmenu(int tg)
{
	int i;

	for(i=0; i<nname; i++)
		if(tag[i] == tg)
			return i;
	return -1;
}

void
menuins(int n, uchar *s, Text *t, int m, int tg)
{
	int i;

	if(nname == mname){
		if(mname == 0)
			mname = 32;
		else
			mname *= 2;
		name = realloc(name, sizeof(name[0])*mname);
		text = realloc(text, sizeof(text[0])*mname);
		tag = realloc(tag, sizeof(tag[0])*mname);
		if(name==nil || text==nil || tag==nil)
			panic("realloc");
	}
	for(i=nname; i>n; --i)
		name[i]=name[i-1], text[i]=text[i-1], tag[i]=tag[i-1];
	text[n] = t;
	tag[n] = tg;
	name[n] = alloc(strlen((char*)s)+2);
	name[n][0] = m;
	strcpy((char*)name[n]+1, (char*)s);
	nname++;
	menu3.lasthit = n+NMENU3;
}

void
menudel(int n)
{
	int i;

	if(nname==0 || n>=nname || text[n])
		panic("menudel");
	free(name[n]);
	--nname;
	for(i = n; i<nname; i++)
		name[i]=name[i+1], text[i]=text[i+1], tag[i]=tag[i+1];
}

void
setpat(char *s)
{
	static char pat[17];

	pat[0] = '/';
	strncpy(pat+1, s, 15);
	menu2str[Search] = pat;
}

#define	NBUF	64
static uchar buf[NBUF*UTFmax]={' ', ' ', ' ', ' '};

char *
paren(char *s)
{
	uchar *t = buf;

	*t++ = '(';
	do; while(*t++ = *s++);
	t[-1] = ')';
	*t = 0;
	return (char *)buf;
}
char*
genmenu2(int n)
{
	Text *t=(Text *)which->user1;
	char *p;
	
	if(n >= NMENU2 + ncmd)
		return 0;
	if(n == Pop)
		p = ncmd <= 0 ? "(pop)" : "pop";
	else if(n == Search && menu2str[n] == nil)
		p = "(search)";
	else if(n >= NMENU2)
		p = clabels[n-NMENU2];
	else
		p = menu2str[n];
	if(!hostlock && !t->lock || n==Search || n==Look)
		return p;
	return paren(p);
}
char*
genmenu2c(int n)
{
	Text *t=(Text *)which->user1;
	char *p;
	if(n >= NMENU2C)
		return 0;
	else if(n == Send)
		p="send";
	else
		p = menu2str[n];
	if(!hostlock && !t->lock)
		return p;
	return paren(p);
}
char *
genmenu3(int n)
{
	Text *t;
	int c, i, k, l, w;
	Rune r;
	char *p;

	if(n >= NMENU3+nname)
		return 0;
	if(n < NMENU3){
		p = menu3str[n];
		if(hostlock)
			p = paren(p);
		return p;
	}
	n -= NMENU3;
	if(n == 0)	/* unless we've been fooled, this is cmd */
		return (char *)&name[n][1];
	if(mw == -1){
		mw = 7;	/* strlen("~~jam~~"); */
		for(i=1; i<nname; i++){
			w = utflen((char*)name[i]+1)+4;	/* include "'+. " */
			if(w > mw)
				mw = w;
		}
	}
	if(mw > NBUF)
		mw = NBUF;
	t = text[n];
	buf[0] = name[n][0];
	buf[1] = '-';
	buf[2] = ' ';
	buf[3] = ' ';
	if(t){
		if(t->nwin == 1)
			buf[1] = '+';
		else if(t->nwin > 1)
			buf[1] = '*';
		if(work && t==(Text *)work->user1) {
			buf[2]= '.';
			if(modified)
				buf[0] = '\'';
		}
	}
	l = utflen((char*)name[n]+1);
	if(l > NBUF-4-2){
		i = 4;
		k = 1;
		while(i < NBUF/2){
			k += chartorune(&r, (char*)name[n]+k);
			i++;
		}
		c = name[n][k];
		name[n][k] = 0;
		strcpy((char*)buf+4, (char*)name[n]+1);
		name[n][k] = c;
		strcat((char*)buf, "...");
		while((l-i) >= NBUF/2-4){
			k += chartorune(&r, (char*)name[n]+k);
			i++;
		}
		strcat((char*)buf, (char*)name[n]+k);
	}else
		strcpy((char*)buf+4, (char*)name[n]+1);
	i = utflen((char*)buf);
	k = strlen((char*)buf);
	while(i<mw && k<sizeof buf-1){
		buf[k++] = ' ';
		i++;
	}
	buf[k] = 0;
	return (char *)buf;
}
