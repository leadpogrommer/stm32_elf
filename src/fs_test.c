#include <stdio.h>
#include <ctype.h>
#include "fs_test.h"
#include "FreeRTOS.h"
#include "task.h"

static char buff[200];
static xTaskHandle test_task_handle;
static void printing_task(){
    printf("Press enter (new build):");
    getchar();
    printf("Starting test...\n");
    FILE *data_file = fopen("/h/data.txt", "r");
    uint32_t read_len = 0;
    FILE *res_file = fopen("/h/r1.txt", "w");
    while ((read_len = fread(buff, 1, 10, data_file))){
        for(uint32_t i = 0; i < read_len; i++){
            buff[i] = toupper(buff[i]);
        }
        fwrite(buff, 1, read_len, res_file);
    }
    fclose(res_file);
    res_file = fopen("/h/r2.txt", "w");
    fseek(data_file, 0, SEEK_SET);
    while ((read_len = fread(buff, 1, 150, data_file))){
        for(uint32_t i = 0; i < read_len; i++){
            buff[i] = tolower(buff[i]);
        }
        fwrite(buff, 1, read_len, res_file);
    }

    fclose(res_file);
    printf("Done\n");
    printf("Free heap: %d\n", xPortGetFreeHeapSize());

    vTaskDelete(NULL);
}

void start_fs_test(){
    xTaskCreate(printing_task, "Test task", 2048, NULL, 2, &test_task_handle);

}