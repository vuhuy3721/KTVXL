#include "esp_vfs.h"
#include "esp_log.h"
#include "spiffs_config.h"

extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_spiffs.h"
}

#include "SPIFFS.h"
#define TAG "spiffs"
#define FILE_PATH "/spiffs/test.txt"


void initialize_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Mount SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
}
void create_file_in_spiffs() {
    // Open file for writing
    FILE* file = fopen(FILE_PATH, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    // Write data to file
    const char* data = "Hello, SPIFFS!";
    fwrite(data, 1, strlen(data), file);

    // Close file
    fclose(file);
}
void read_file_from_spiffs() {
    // Open file for reading
    FILE* file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    // Read data from file
    char buffer[50];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);

    // Close file
    fclose(file);

    // Print read data
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        ESP_LOGI(TAG, "Read from file: %s", buffer);
    } else {
        ESP_LOGE(TAG, "Failed to read from file");
    }
}