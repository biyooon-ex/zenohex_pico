#include <errno.h>
#include <stdint.h>
#include <time.h>

#include "time_compat.h"

int zxp_monotonic_to_realtime_deadline(const struct timespec *monotonic_deadline,
                                       struct timespec *realtime_deadline)
{
  struct timespec monotonic_now, realtime_now;
  if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) != 0 ||
      clock_gettime(CLOCK_REALTIME, &realtime_now) != 0)
  {
    return EINVAL;
  }
  int64_t remaining_ns =
      ((int64_t)monotonic_deadline->tv_sec - monotonic_now.tv_sec) * 1000000000LL +
      ((int64_t)monotonic_deadline->tv_nsec - monotonic_now.tv_nsec);
  if (remaining_ns <= 0)
  {
    return ETIMEDOUT;
  }
  realtime_deadline->tv_sec = realtime_now.tv_sec + remaining_ns / 1000000000LL;
  realtime_deadline->tv_nsec = realtime_now.tv_nsec + remaining_ns % 1000000000LL;
  if (realtime_deadline->tv_nsec >= 1000000000L)
  {
    realtime_deadline->tv_sec++;
    realtime_deadline->tv_nsec -= 1000000000L;
  }
  return 0;
}
