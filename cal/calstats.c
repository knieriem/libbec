#include <u.h>
#include <libc.h>

#include "calstats.h"

#ifdef TESTCALSTATS
#	define dprint(s, v) calstatstest_print(s, v)
extern	void	calstatstest_print(char *s, int v);
#else
#	define dprint(s, v)
#endif

static int
incrcnt8(uint8 *v)
{
	if (*v < 0xFF) {
		(*v)++;
		return 1;
	}
	return 0;
}

static int
incrcnt16(uint16 *v)
{
	if (*v < 0xFFFF) {
		(*v)++;
		return 1;
	}
	return 0;
}

static void
updatefilter(Calstats *st, int state, int ischange)
{
	Calstatsfilter *f;
	Calstatspoint *pt;

	f = &st->state->f;
	pt = &st->pt[f->ipt];
	if (ischange) {
		dprint("state", state);
		if (incrcnt8(&pt->entered[state])) {
			f->entered[state]++;
		}
	}
	if (state == 0) {
		return;
	}
	if (incrcnt16(&pt->duration[state])) {
		f->duration[state]++;
		f->globduration++;
	}
}

static void
rotatefilters(Calstats *st)
{
	Calstatsfilter *f;
	Calstatspoint *pt;
	int next;
	int d;
	int i;

	f = &st->state->f;
	next = f->ipt + 1;
	if (f->npt == st->nptmax) {
		if (next == st->nptmax) {
			next = 0;
		}
		pt = &st->pt[next];
		for (i=0; i<Ncal; i++) {
			d = pt->duration[i];
			f->globduration -= d;
			f->duration[i] -= d;
			f->entered[i] -= pt->entered[i];
			pt->duration[i] = 0;
			pt->entered[i] = 0;
		}
	} else {
		f->npt++;
	}
	f->ipt = next;
}

void
calstatsupdate(Calstats *st, int calstate)
{
	Calstatsstate *s;
	int ischange;

	calstate++;
	if (calstate > Ncal-1) {
		return;
	}

	s = st->state;
	ischange = s->cur.calstate != calstate;
	if (ischange) {
		if (s->cur.calstate == Calibrated) {
			s->fullcal.prevdur = s->cur.duration;
		}
		if (calstate == Calibrated) {
			incrcnt16(&s->fullcal.cnt);
		}
		s->cur.duration = 0;
		s->cur.calstate = calstate;
		dprint("remain", st->period - s->elapsed);
	}
	updatefilter(st, calstate, ischange);

	incrcnt16(&s->cur.duration);
	s->elapsed++;

	if (s->elapsed == st->period) {
		rotatefilters(st);
		incrcnt16(&s->periods);
		s->elapsed = 0;
		dprint("period", s->periods);
		dprint("ipt", s->f.ipt);
	}
}

static void
calcfltvals(Calstatsfltval *v, Calstatsfilter *f, int pctmult)
{
	int i;

	for (i=0; i<Ncal-1; i++) {
		v[i].durationpct = (100UL*pctmult*f->duration[i+1] + f->globduration/2) / f->globduration;
		v[i].entered = f->entered[i+1];
	}
}

static uint16
durmin(Calstats *st, uint16 cnt)
{
	uint32 u;

	u = cnt;
	u = (uint32) cnt * st->msperstep / 60 / 1000;
	if (u > 0xFFFF) {
		u = 0xFFFF;
	}
	return u;
}

void
calstatsregs(Calstatsregs *r, Calstats *st, int pctmult)
{
	Calstatsstate *s;

	s = st->state;
	r->periods = s->periods;
	r->fullcalcnt = s->fullcal.cnt;
	r->prevcaldur = durmin(st, s->fullcal.prevdur);
	r->curdur = durmin(st, s->cur.duration);

	calcfltvals(r->val, &s->f, pctmult);
	r->initcnt = s->f.entered[0];
}

void
calstatsinit(Calstats *st)
{
	Calstatsstate *s;
	Calstatsfilter *f;

	s = st->state;
	f = &s->f;

	/* apply some boundary checks */
	if (f->ipt < 0 || f->ipt > st->nptmax-1) {
		calstatsreset(st);
	} else if (f->npt < 1 || f->npt > st->nptmax) {
		calstatsreset(st);
	}

	updatefilter(st, Calinitial, 1);
}

void
calstatsreset(Calstats *st)
{
	memset(st->state, 0, sizeof *st->state);
	memset(&st->pt[0], 0, sizeof st->pt[0]);
	st->state->f.npt = 1;
}
