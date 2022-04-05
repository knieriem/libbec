enum {
	Calinitial,

	/* calibration state (refer to integration guide / IAQ accuracy) */
	Calnone,
	Calrequired,
	Calongoing,
	Calibrated,

	Ncal = Calibrated+1,
};

typedef const struct Calstats Calstats;
typedef struct Calstatsstate Calstatsstate;
typedef struct Calstatsfilter Calstatsfilter;

typedef struct Calstatspoint Calstatspoint;

struct Calstats {
	Calstatsstate* state;

	Calstatspoint *pt;
	int8 nptmax;

	uint16 period;
	uint16 msperstep;
};

/* Calstatsfilter, -state, and -point[] are intended to be stored
 * in non-volatile memory, so that they cover an extended period
 * even if reboots occur.
 */
struct Calstatsfilter {
	int8 npt;
	int8 ipt;

	uint32 globduration;
	uint32 duration[Ncal];
	uint16 entered[Ncal];
};

struct Calstatsstate {
	struct {
		uint16 cnt;
		uint16 prevdur;	/* minutes */
	} fullcal;

	struct {
		uint16 duration;
		int8 calstate;
	} cur;

	Calstatsfilter f;

	uint16 periods;
	uint16 elapsed;
};

struct Calstatspoint {
	uint16 duration[Ncal];
	uint8 entered[Ncal];
};

extern	void	calstatsinit(Calstats*);
extern	void	calstatsreset(Calstats*);

extern	void	calstatsupdate(Calstats*, int calstate);


typedef struct Calstatsregs Calstatsregs;

typedef struct Calstatsfltval Calstatsfltval;
struct Calstatsfltval {
	uint16 entered;
	uint16 durationpct;
};

struct Calstatsregs {
	uint16 periods;
	uint16 fullcalcnt;
	uint16 prevcaldur;
	uint16 curdur;

	Calstatsfltval val[Ncal-1];
	uint16 initcnt;
};

extern	void	calstatsregs(Calstatsregs*, Calstats*, int pctmult);

