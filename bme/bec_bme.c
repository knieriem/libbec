#include <u.h>
#include <stdio.h>
#include <bsec_datatypes.h>
#include <bme68x.h>

#include "../bec.h"
#include "../private.h"

enum {
/* When using github.com/knieriem/BME68x-Sensor-API,
 * there are additional flags that tell about the presence of
 * T, P, and H values. In case for some reason one of these
 * channels is disabled, the raw value will be 0x80000 resp. 0x8000.
 * The official library does not detect this case, and will just
 * calculate values using 0x80000, or 0x8000 as input.
 */
#ifdef BME68X_TEMPM_PRESENT_MSK
	TPH_PRESENT = BME68X_TEMPM_PRESENT_MSK | BME68X_HUMM_PRESENT_MSK | BME68X_PRESM_PRESENT_MSK,
#else
	TPH_PRESENT = 0,
#endif
};

static int
setupmeas(Bmeinst *inst, bsec_bme_settings_t *c)
{
	struct bme68x_dev *dev;
	struct bme68x_conf sc;
	struct bme68x_heatr_conf hc;
	int st;

	dev = inst->dev;
	
	if (!TPH_PRESENT) {
		/* If the official BME68x driver is used,
		 * it is safer to configure settings before every measurement:
		 */
		becbme_setunconfigured(inst);
	}

	if (becbme_needupdatemeas(inst, c)) {
		sc.filter = BME68X_FILTER_OFF;
		sc.odr = BME68X_ODR_NONE;
		sc.os_temp = c->temperature_oversampling;
		sc.os_hum = c->humidity_oversampling;
		sc.os_pres = c->pressure_oversampling;
		st = bme68x_set_conf(&sc, dev);
		if (st != 0) {
			return st;
		}
		inst->measdur = bme68x_get_meas_dur(BME68X_FORCED_MODE, &sc, dev);
	}
	if (c->run_gas) {
		if (becbme_needupdateheater(inst, c)) {
			hc.enable = BME68X_ENABLE;
			hc.heatr_temp = c->heater_temperature;
			hc.heatr_dur = c->heater_duration;
			inst->heatdur = c->heater_duration * 1000;

			st = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &hc, dev);
			if (st != 0) {
				becbme_setunconfigured(inst);
				return st;
			}
		}
	}
	return 0;
}

static int
seterr(Bmeinst *inst, int st)
{
	inst->err = st;
	if (st == BME68X_E_COM_FAIL) {
		return BmeErrcomm;
	}
	return -1;
}

static int
meas_forced(Bmeinst *inst, Becinputs *dest, bsec_bme_settings_t *c)
{
	struct bme68x_dev *dev;
	struct bme68x_data data;
	uint32 process;
	int st;
	uint8 nfield;
	int nretry;

	/* for now, selectivity is not supported */
	if (opmode(c) != BME68X_FORCED_MODE) {
		return -1;
	}

	dev = inst->dev;

	nretry = 0;
retry:
	st = setupmeas(inst, c);
	if (st != 0) {
		return seterr(inst, st);
	}

	st = bme68x_set_op_mode(BME68X_FORCED_MODE, dev);
	if (st != 0) {
		return seterr(inst, st);
	}

	dev->delay_us(inst->heatdur + inst->measdur, dev->intf_ptr);

	st = bme68x_get_data(BME68X_FORCED_MODE, &data, &nfield, dev);
	if (st != 0) {
		return seterr(inst, st);
	}

	/* in forced mode, there must be exactly one new field */
	if (nfield == 0) {
		return -1;
	}

	if (TPH_PRESENT)
	if ((data.status & TPH_PRESENT) != TPH_PRESENT) {
		becbme_setunconfigured(inst);
		if (nretry < 1) {
			goto retry;
		}
		nretry++;
	}

	process = c->process_data;
	if (process & BSEC_PROCESS_PRESSURE) {
		becappendinput(dest, BSEC_INPUT_PRESSURE, data.pressure);
	}
	if (process & BSEC_PROCESS_TEMPERATURE) {
		becappendinput(dest, BSEC_INPUT_TEMPERATURE, data.temperature);
	}
	if (process & BSEC_PROCESS_HUMIDITY) {
		becappendinput(dest, BSEC_INPUT_HUMIDITY, data.humidity);
	}
	if (process & BSEC_PROCESS_GAS) {
		if (data.status & BME68X_GASM_VALID_MSK) {
			becappendinput(dest, BSEC_INPUT_GASRESISTOR, data.gas_resistance);
		}
	}
	if (BSECV==2) {
		becappendinput(dest, BSEC_INPUT_PROFILE_PART, 0.0f);
	}
	return 0;
}

Becbmedrv becbme68xdrv = {
	.meas= meas_forced,
};



/*
 * This library assums that BME68X_OK is zero
 */
#if BME68X_OK != 0
#	error "BME68X_OK not zero"
#endif
