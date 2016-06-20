#include <stdint.h>
#include <sys/sysinfo.h>


uint64_t ramsize() {
  struct sysinfo info = {0};
  // Not portable but meh.
  sysinfo( &info );
  return info.totalram;
}
