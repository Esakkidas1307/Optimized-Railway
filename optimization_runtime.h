#ifndef OPTIMIZATION_RUNTIME_H
#define OPTIMIZATION_RUNTIME_H

#include "optimization_support.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*RailwayRuntimeSaveCallback)(void);

int railway_runtime_init(unsigned interval_seconds);
int railway_runtime_register_saver(RailwayDataFile file, RailwayRuntimeSaveCallback callback);
void railway_runtime_mark_dirty(RailwayDataFile file);
void railway_runtime_mark_clean(RailwayDataFile file);
int railway_runtime_is_dirty(RailwayDataFile file);
void railway_runtime_service(void);
void railway_runtime_flush(void);
void railway_runtime_shutdown(void);
void railway_runtime_request_timeout(void);
int railway_runtime_timeout_requested(void);
int railway_runtime_interruption_requested(void);

#ifdef __cplusplus
}
#endif
#endif
