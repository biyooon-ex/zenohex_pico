#include <time.h>

extern int zxp_monotonic_to_realtime_deadline(const struct timespec *monotonic_deadline,
                                              struct timespec *realtime_deadline);
