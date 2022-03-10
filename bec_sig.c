#include <u.h>
#include <math.h>
#include <libutil/ieee754.h>
#include <bsec_datatypes.h>

#include "bec.h"


static void*
pos(BecOutconf *c, void *v)
{
	uchar *p;

	p = v;
	return &p[c->pos];
}

void
bec_accuracy(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(uint8*)pos(c, v) = o->accuracy;
}

static void
sigasfixed(BecOutconf *c, void *v, bsec_output_t *o, int expshift)
{
	int32 mant;
	int exp;

	if (ieee754funpack32(*(uint32*)&o->signal, &mant, &exp) != 0) {
		return;
	}
	exp += expshift;
	if (exp > 0) {
		if (exp > 7)
			return;
		mant <<= exp;
	} else if (exp < 0) {
		mant >>= -exp;
	}
	*(int32*)pos(c, v) = mant;
}

void
becsig_asfixed19(BecOutconf *c, void *v, bsec_output_t *o)
{
	sigasfixed(c, v, o, -4);
}

void
becsig_asfixed0(BecOutconf *c, void *v, bsec_output_t *o)
{
	sigasfixed(c, v, o, -23);
}

void
becsig_asfloat32(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(float32*)pos(c, v) = o->signal;
}

void
becsig_asint(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(int*)pos(c, v) = (int)roundf(o->signal);
}
void
becsig_asint32(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(int32*)pos(c, v) = (int32)roundf(o->signal);
}

void
becsig_asint100(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(int32*)pos(c, v) = (int32)roundf(o->signal*100.F);
}

void
becsig_asint1000(BecOutconf *c, void *v, bsec_output_t *o)
{
	*(int32*)pos(c, v) = (int32)roundf(o->signal*1000.F);
}
