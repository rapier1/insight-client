#include "geoip.h"

double geoip_lat_or_long (char *ip, MMDB_s dbhandle, char *coord) {
  int gai_error, mmdb_error;
  MMDB_lookup_result_s result = MMDB_lookup_string(&dbhandle, ip, &gai_error, &mmdb_error);
  if (gai_error != 0 )
    return 0;
  // some error with access to the db handle
  if (MMDB_SUCCESS != mmdb_error)
    return 0;
  // could not find the ip address in the db
  if (!result.found_entry)
    return 0;
  MMDB_entry_data_s entry_data;
  int status = MMDB_get_value(&result.entry, &entry_data, "location", coord, NULL);
  // encountered some odd problem
  if (status != MMDB_SUCCESS)
    return 0;
  // there is an entry but there is no data associated with that entry. 
  if (!entry_data.has_data) 
    return 0;
  // all error checks passed. return the requested coordinate
  return entry_data.double_value;
}
