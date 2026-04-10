#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "myrtio/core.h"

#ifndef APP_TASK_STACK_SIZE
#ifdef CONFIG_ARDUINO_LOOP_STACK_SIZE
#define APP_TASK_STACK_SIZE CONFIG_ARDUINO_LOOP_STACK_SIZE
#else
#define APP_TASK_STACK_SIZE 8192
#endif
#endif

#ifndef APP_TASK_CORE
#ifdef ARDUINO_RUNNING_CORE
#define APP_TASK_CORE ARDUINO_RUNNING_CORE
#else
#define APP_TASK_CORE 1
#endif
#endif

static void app_task(void *arg) {
    (void)arg;
    myrtio_main();
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    nvs_flash_init();
    xTaskCreatePinnedToCore(app_task, "app_task", APP_TASK_STACK_SIZE,
                            NULL, 1, NULL, APP_TASK_CORE);
}
