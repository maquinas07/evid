#ifndef EVID_UTIL_H
#define EVID_UTIL_H

#define PROGRAM_NAME "evid"
#define ARR_SIZE(arr) (sizeof(arr) / sizeof(*arr))

void error(const char *errstr, ...);
void die(const char *errstr, ...);

void print_usage(void);

#endif
