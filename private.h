extern	void	becappendinput(Becinputs*, uint8 id, float32 v);

extern	int	becbme_needupdatemeas(Bmeinst*, bsec_bme_settings_t*);
extern	int	becbme_needupdateheater(Bmeinst*, bsec_bme_settings_t*);
extern	void	becbme_setunconfigured(Bmeinst*);

#if ! defined(BSECV)
#	define BSECV 2
#endif

#if BSECV == 1
#	define BSEC_INPUT_PROFILE_PART 0
#	define heater_duration heating_duration
#	define opmode(c)	BME68X_FORCED_MODE
#else
#	define opmode(c)	((c)->op_mode)
#endif
