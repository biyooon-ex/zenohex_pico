#include <errno.h>
#include <time.h>

#include "time_compat.h"

/*
 * Convert an absolute CLOCK_MONOTONIC deadline to an equivalent absolute
 * CLOCK_REALTIME deadline. The remaining duration is calculated with
 * CLOCK_MONOTONIC, so that calculation is not affected by wall-clock
 * adjustments.
 *
 * First calculate the remaining monotonic duration:
 *
 *   remaining = monotonic_deadline - monotonic_now
 *
 * Then express that same duration as a realtime absolute deadline:
 *
 *   realtime_deadline = realtime_now + remaining
 *
 * The input deadline and the clock values read below are normalized timespec
 * values, where 0 <= tv_nsec < 1,000,000,000. Subtraction may produce negative
 * nanoseconds, so borrow one second. Addition may produce too many nanoseconds,
 * so carry one second. A deadline at or before now has no remaining duration
 * and returns ETIMEDOUT. On success, realtime_deadline receives a normalized
 * CLOCK_REALTIME absolute deadline.
 */
int zxp_monotonic_to_realtime_deadline(const struct timespec *monotonic_deadline,
                                       struct timespec *realtime_deadline)
{
  struct timespec monotonic_now, realtime_now;
  if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) != 0 ||
      clock_gettime(CLOCK_REALTIME, &realtime_now) != 0)
  {
    return EINVAL;
  }
  if (monotonic_deadline->tv_sec < monotonic_now.tv_sec ||
      (monotonic_deadline->tv_sec == monotonic_now.tv_sec &&
       monotonic_deadline->tv_nsec <= monotonic_now.tv_nsec))
  {
    return ETIMEDOUT;
  }

  time_t remaining_seconds = monotonic_deadline->tv_sec - monotonic_now.tv_sec;
  long remaining_nanoseconds = monotonic_deadline->tv_nsec - monotonic_now.tv_nsec;
  if (remaining_nanoseconds < 0)
  {
    remaining_seconds--;
    remaining_nanoseconds += 1000000000L;
  }

  realtime_deadline->tv_sec = realtime_now.tv_sec + remaining_seconds;
  realtime_deadline->tv_nsec = realtime_now.tv_nsec + remaining_nanoseconds;
  if (realtime_deadline->tv_nsec >= 1000000000L)
  {
    realtime_deadline->tv_sec++;
    realtime_deadline->tv_nsec -= 1000000000L;
  }
  return 0;
}
