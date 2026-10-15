#ifndef MIXER_SM_H
#define MIXER_SM_H

#include "mixer_types.h"

void mixer_sm_init(void);
void mixer_sm_handle_event(const mixer_event_t *evt);
void mixer_sm_process_tick(void);
void mixer_sm_get_status(mixer_status_t *out_status);

#endif /* MIXER_SM_H */
