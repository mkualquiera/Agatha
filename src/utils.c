#if INTERFACE
#include <stdlib.h>
#endif

#include "utils.h"

int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {

    // Chop off all of the values that would cause skew...
    int end = RAND_MAX / n; // truncate skew
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}
