#include <stddef.h>

#include "AmlTime.h"


time_t timeGetTime (void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (time_t) (tv.tv_usec / 1000.0 + 1000 * tv.tv_sec);
}


time_t GetTickCount (void) {
  return timeGetTime();
}
