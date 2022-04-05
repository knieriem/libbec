typedef const struct BecOutconf BecOutconf;

struct BecOutconf {
    uint8 id;
    void (*cvt)(BecOutconf*, void*, bsec_output_t*);
    int pos;
    int acc;
};

typedef void BecOutcvtfn(BecOutconf*, void *val, bsec_output_t*);

extern BecOutcvtfn bec_accuracy;
extern BecOutcvtfn becsig_asfloat32;
extern BecOutcvtfn becsig_asfixed0;
extern BecOutcvtfn becsig_asfixed19;
extern BecOutcvtfn becsig_asint32;
extern BecOutcvtfn becsig_asint;
extern BecOutcvtfn becsig_asint100;
extern BecOutcvtfn becsig_asint1000;

typedef struct Becinst Becinst;
typedef struct Bmeinst Bmeinst;

typedef const struct Becconf Becconf;
typedef const struct Becsensor Becsensor;

typedef const struct Becbmedrv Becbmedrv;

extern Becbmedrv becbme68xdrv;

struct Bmeinst {
	bsec_bme_settings_t cur;
	uint32 heatdur;
	uint32 measdur;
	void *dev;
	int8	err;
};

enum {
	BmeErr = -1,
	BmeErrcomm = -2,
};

struct Becinst {
	Becconf *conf;
	Becsensor *sensor;

	int64 timestamp;
	int64 nextcall;

	Bmeinst bme;

	int8 stateerr;
	int8 err;
	int errctx;
};


enum {
	BecFailctxInit = 1,
	BecFailctxSetconf,
	BecFailctxUpdatesubs,
	BecFailctxSensorctl,
	BecFailctxBme,
	BecFailctxSteps,
	BecFailctxGetstate,
	BecFailctxSetstate,
};

struct Becconf {
	BecOutconf *outputs;
	int numoutputs;

	uint32 save_intval;

	void (*yield)(void);
};

typedef struct Becstate Becstate;

struct Becstate {
	uchar *data;
	int ndata;

	int64 timestamp;
	int64 nextcall;
};

typedef struct Bsecconf Bsecconf;

struct Bsecconf {
	uchar *conf;
	int confsize;
	Becstate state;
};

struct Becsensor {
	void (*bsecconf)(Bsecconf*);

	Becbmedrv *bmedrv;
	void*	bmedev;
};


typedef struct Becinputs Becinputs;

struct Becinputs {
	bsec_input_t *p;
	bsec_input_t *w;
	int64 ns;
};

struct Becbmedrv {
	int	(*meas)(Bmeinst*, Becinputs *dest, bsec_bme_settings_t*);
};

extern	int	bec_init(Becinst*, float32 sample_rate);
extern	int	bec_step(Becinst*, void *outvals);

extern	void (*becdebugbmec)(Becinst*, bsec_bme_settings_t*);


extern	int	bec_getstate(Becinst*, Becstate*);
extern	int	bec_setstate(Becinst*, Becstate*);
