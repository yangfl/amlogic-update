#ifndef AML_TIME_H
#define AML_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>


time_t timeGetTime (void);
time_t GetTickCount (void);


#ifdef __cplusplus
}
#endif

#endif  // AML_TIME_H
