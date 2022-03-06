enum {
	Colrioback,

	/* the following group has to be in order, they are used by libframe */
	Colback,
	Colhigh,
	Colbord,
	Coltext,
	Colhtext,

	Coltitle,
	Colltitle,
	Colhold,
	Collhold,
	Colpalehold,
	Colpaletext,
	Colsize,

	/* menuhit */
	Colmenubar,
	Colmenuback,
	Colmenuhigh,
	Colmenubord,
	Colmenutext,
	Colmenuhtext,

	Numcolors
};

extern Image *col[Numcolors];
void themeload(char *s, int n);
char *themestring(int *n);
