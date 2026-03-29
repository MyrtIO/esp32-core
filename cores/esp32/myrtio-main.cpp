#include "Arduino.h"

#if defined(MYRTIO_ENABLE)

extern "C" void myrtio_main(void);

void setup(void) {
    myrtio_main();
}

void loop(void) {
    delay(1000);
}

#endif
