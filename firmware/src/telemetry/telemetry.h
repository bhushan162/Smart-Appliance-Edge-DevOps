#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "mixer_types.h"

void telemetry_init(void);
void telemetry_report_status(const mixer_status_t *status);

#endif
