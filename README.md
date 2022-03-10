_Bec_ is a thin layer around Bosch Sensortec's [BSEC 2.x] library (BSEC 1 may be used too).
It's purpose is to allow easier integration in a C based code base,
compared to using the C++ layer provided by Bosch Sensortec.

BSEC (Bosch Software Environmental Cluster) is a closed-source library;
it is a necessary piece of software if values derived from the BME68x sensor's
main measurement values shall be obtained,
like IAQ, sIAQ, bVOCeq, or CO2eq.

_Bec_ is decoupled from the actual BME68x driver and device;
subdirectory [bme] provides support  for the official [BME68x Sensor API].


[BSEC 2.x]: https://www.bosch-sensortec.com/software-tools/software/bme688-software/
[BME68x Sensor API]: https://github.com/BoschSensortec/BME68x-Sensor-API
[bme]: ./bme

Only a single sensor instance is supported now,
but multi-instance support may be added in the future.
