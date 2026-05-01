#include <Arduino.h>

#include <ota.h>
#include <gnss.h>
#include <lora_wan.h>
#include <deep_sleep.h>

void vGNSSTask(void *parameters);

uint8_t wake_status = 0;
int count1 = 0;
int count2 = 0;

SemaphoreHandle_t serialSem;

void setup()
{
    Serial.begin(115200);
    // delay(5000);

    wake_status = print_wakeup_reason();

    setup_gnss();
    setup_lora_wan_app();

    serialSem = xSemaphoreCreateMutex();

    if (serialSem == NULL)
    {
        Serial.println("xSemaphoreCreateMutex returned NULL");
        while (1)
            ;
    }

    // xTaskCreate(task1, "Task 1", 2048, NULL, 1, NULL);
    // xTaskCreate(task2, "Task 2", 2048, NULL, 1, NULL);
    // xTaskCreate(vGNSSTask, "GNSS Task", 4096, NULL, 2, NULL);

    // setup_ota();
}

void loop()
{
    handle_ota();
    enter_lora_wan_app(wake_status);
}

void vGNSSTask(void *parameters)
{
    for (;;)
    {
        // uint8_t hasLocation = get_location(serialSem);

        // if (hasLocation)
        //     go_sleep();
        // send_gps_data(13.72883826776297, 100.7759913633405, 0);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}
