#include <Arduino.h>

#include <gnss.h>
#include <lora.h>
#include <deep_sleep.h>

void task1(void *parameters);
void task2(void *parameters);
void vGNSSTask(void *parameters);

int count1 = 0;
int count2 = 0;

SemaphoreHandle_t serialSem;

void setup()
{
    Serial.begin(115200);

    print_wakeup_reason();

    setup_gnss();
    setup_lora();

    serialSem = xSemaphoreCreateMutex();

    if (serialSem == NULL)
    {
        Serial.println("xSemaphoreCreateMutex returned NULL");
        while (1)
            ;
    }

    xTaskCreate(task1, "Task 1", 2048, NULL, 1, NULL);
    xTaskCreate(task2, "Task 2", 2048, NULL, 1, NULL);
    xTaskCreate(vGNSSTask, "GNSS Task", 4096, NULL, 2, NULL);
}

void loop()
{
}

void task1(void *parameters)
{
    for (;;)
    {
        xSemaphoreTake(serialSem, portMAX_DELAY);
        Serial.printf("Task 1 Timer second: %d\n", count1++);
        xSemaphoreGive(serialSem);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task2(void *parameters)
{
    for (;;)
    {
        // if (count2 >= 5)
        //     go_sleep();

        xSemaphoreTake(serialSem, portMAX_DELAY);
        Serial.printf("Task 2 counter: %d\n", count2++);
        xSemaphoreGive(serialSem);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void vGNSSTask(void *parameters)
{
    for (;;)
    {
        uint8_t hasLocation = get_location(serialSem);

        if (hasLocation)
            go_sleep();

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
