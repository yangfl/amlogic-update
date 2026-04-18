#ifndef GET_TIME_H
#define GET_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <sys/time.h>


inline time_t timeGetTime (void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (time_t) tv.tv_usec / 1000 + 1000 * tv.tv_sec;
}


inline time_t GetTickCount (void) {
  return timeGetTime();
}



#ifdef __cplusplus
}
#endif

#endif  // GET_TIME_H
