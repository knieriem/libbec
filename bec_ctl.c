#include <u.h>
#include <bsec_interface.h>
#include <bsec_datatypes.h>

#include "bec.h"
#include "private.h"


/* The workbuf is needed to decode/encode configuration
 * and state binary objects. It is also reused as a storage for
 * local, temporary variables.
 */
static struct {
	uchar buf[BSEC_MAX_WORKBUFFER_SIZE];
	uchar *p;
	uchar *ep;
} work = {
	.p= &work.buf[0],
	.ep= &work.buf[sizeof work.buf],
};

static void
workbufreset(void)
{
	work.p = &work.buf[0];
}

static void*
workbufalloc(int sz)
{
	uintptr p;

	p = (uintptr)work.p;
	p = (p + 7) & ~(uintptr)7;

	work.p = &((uchar*)p)[sz];
	if (work.p > work.ep) {
		for (;;) {
			;	/* FIXME */
		}
	}
	return (void*)p;
}

static int
error(Becinst *inst, int st, int ctx)
{
	inst->err = st;
	inst->errctx = ctx;
	return -1;
}

static int
updatesubscriptions(Becconf *c, float32 sample_rate)
{
	bsec_sensor_configuration_t	*reqvirt;
	bsec_sensor_configuration_t	*w;
	bsec_sensor_configuration_t	*physconf;
	uint8 nphysconf;

	workbufreset();
	nphysconf = BSEC_MAX_PHYSICAL_SENSOR;
	physconf = workbufalloc((c->numoutputs+nphysconf) * sizeof *reqvirt);
	reqvirt = &physconf[nphysconf];
    
	BecOutconf *oc, *ep;
	uint8 previd;

	previd = 0;
	w = &reqvirt[0];
	ep = &c->outputs[c->numoutputs];
	for (oc = &c->outputs[0]; oc<ep; oc++) {
		if (oc->id == previd) {
			continue;
		}
		w->sensor_id = oc->id;
		w->sample_rate = sample_rate;
		w++;
		previd = oc->id;
	}

	return bsec_update_subscription(reqvirt, w-reqvirt, physconf, &nphysconf);
}

int
bec_updatesubscriptions(Becinst *inst, float32 sample_rate)
{
	int st;

	st = updatesubscriptions(inst->conf, sample_rate);
	if (st != 0) {
		return error(inst, st, BecFailctxUpdatesubs);
	}
	return 0;	
}

int
bec_init(Becinst *inst, float32 sample_rate)
{
	Bsecconf bc;
	int skipstate;
	int st;

	inst->stateerr = 0;
	skipstate = 0;
retry:
	st = bsec_init();
	if (st != 0) {
		return error(inst, st, BecFailctxInit);
	}
	inst->sensor->bsecconf(&bc);
	st = bsec_set_configuration(bc.conf, bc.confsize, work.buf, sizeof work.buf);
	if (st != 0) {
		return error(inst, st, BecFailctxSetconf);
	}

	if (!skipstate)
	if (bec_setstate(inst, &bc.state) == -1) {
		inst->stateerr = inst->err;

		/* unsure, if the bsec library is in a clean state now,
		 * it may be safer to initialize again completely
		 */
		skipstate = 1;
		goto retry;
	}

	st = updatesubscriptions(inst->conf, sample_rate);
	if (st != 0) {
		return error(inst, st, BecFailctxUpdatesubs);
	}
	return 0;
}

void
becappendinput(Becinputs *dest, uint8 id, float32 v)
{
	bsec_input_t *w;

	w = dest->w;
	w->sensor_id = id;
	w->signal = v;
	w->time_stamp = dest->ns;

	dest->w++;
}


static void
cnvoutputs(Becconf *c, void *outvals, bsec_output_t *b, int nb)
{
	BecOutconf *oc, *ep;
	bsec_output_t *o;
	int i;

	ep = &c->outputs[c->numoutputs];
	for (i = 0; i < nb; i++) {
		c->yield();
		for (oc = &c->outputs[0]; oc < ep; oc++) {
			o = &b[i];
			if (oc->id != o->sensor_id) {
				continue;
			}
			oc->cvt(oc, outvals, o);

			/* first element of "outval" is the timestamp */
			*(int64*)outvals = b[i].time_stamp;
		}
	}
}

static void
initinputs(Becinputs *in, bsec_input_t *buf, int64 ns)
{
	in->p = buf;
	in->w = buf;
	in->ns = ns;
}

static int
numinputs(Becinputs *in)
{
	return in->w - in->p;
}

void (*becdebugbmec)(Becinst*, bsec_bme_settings_t*);

int
bec_step(Becinst *inst, void *outvals)
{
	bsec_bme_settings_t bmec;
	bsec_output_t *out;
	Becinputs in;
	uint8 nout;
	int st;

	inst->timestamp = inst->nextcall;
	st = bsec_sensor_control(inst->timestamp, &bmec);
	if (st != 0) {
		return error(inst, st, BecFailctxSensorctl);
	}
	inst->nextcall = bmec.next_call;

	if (becdebugbmec != nil) {
		becdebugbmec(inst, &bmec);
	}

	if (!bmec.trigger_measurement) {
		return 0;
	}

	workbufreset();
	initinputs(&in, workbufalloc(BSEC_MAX_PHYSICAL_SENSOR * sizeof(bsec_input_t)), inst->timestamp);

	st = inst->sensor->bmedrv->meas(&inst->bme, &in, &bmec);
	if (st < 0) {
		return error(inst, 0, BecFailctxBme);
	}

	out = workbufalloc(sizeof(bsec_output_t)*BSEC_NUMBER_OUTPUTS);
	nout = BSEC_NUMBER_OUTPUTS;

	st = bsec_do_steps(in.p, numinputs(&in), out, &nout);
	if (st != 0) {
		return error(inst, st, BecFailctxSteps);
	}

	cnvoutputs(inst->conf, outvals, out, nout);
	inst->nextcall = bmec.next_call;
	return 0;
}

int
bec_getstate(Becinst *inst, Becstate *state)
{
	uint32 n;
	int st;

	st = bsec_get_state(0, state->data, BSEC_MAX_STATE_BLOB_SIZE, work.buf, sizeof work.buf, &n);
	if (st != 0) {
		return error(inst, st, BecFailctxGetstate);
	}
	state->ndata = n;
	state->timestamp = inst->timestamp;
	state->nextcall = inst->nextcall;
	return 0;	
}

int
bec_setstate(Becinst *inst, Becstate *state)
{
	int st;

	if (state->ndata == 0) {
		return 0;
	}
	st = bsec_set_state(&state->data[0], state->ndata, work.buf, sizeof work.buf);
	if (st != 0) {
		return error(inst, st, BecFailctxSetstate);
	}
	inst->nextcall = state->timestamp;
	return 0;	
}


int
becbme_needupdatemeas(Bmeinst *inst, bsec_bme_settings_t *c)
{
	bsec_bme_settings_t *cur;

	cur = &inst->cur;
	if (cur->process_data == c->process_data)
	if (cur->temperature_oversampling == c->temperature_oversampling) 
	if (cur->humidity_oversampling == c->humidity_oversampling)
	if (cur->pressure_oversampling == c->pressure_oversampling) {
		return 0;
	}

	/* don't update process_data yet, it will be done in *heater */
	cur->temperature_oversampling = c->temperature_oversampling;
	cur->humidity_oversampling = c->humidity_oversampling;
	cur->pressure_oversampling = c->pressure_oversampling;

	return 1;
}

int
becbme_needupdateheater(Bmeinst *inst, bsec_bme_settings_t *c)
{
	bsec_bme_settings_t *cur;

	cur = &inst->cur;
	if (cur->process_data == c->process_data)
	if (cur->heater_temperature == c->heater_temperature)
	if (cur->heater_duration == c->heater_duration) {
		return 0;
	}
	cur->process_data = c->process_data;
	cur->heater_temperature = c->heater_temperature;
	cur->heater_duration = c->heater_duration;
	return 1;	
}

void
becbme_setunconfigured(Bmeinst *inst)
{
	inst->cur.process_data = 0;
}



/*
 * This library assums that BSEC_OK is zero
 */
#if BSEC_OK != 0
#	error "BSEC_OK not zero"
#endif
