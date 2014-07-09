#ifndef GEOIP_H
#define GEOIP_H

#include "maxminddb.h"

double geoip_lat_or_long (char *, MMDB_s, char *);

#endif
