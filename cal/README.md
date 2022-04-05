Calstats provides a means to get some insights into the
indoor-air quality calibration process of a BSEC/BME68x sensor system.
It is meant as a tool during testing a system using a BSEC library.

Calstats reports how often one of the calibration states 0..3 was entered during the last _n_ days,
and how much time has been spent in each state (as percentage).

Some additional variables are reported too, like

-	total running time,
-	time spent in current calibration state, or
-	time spent in fully calibrated state previously.
