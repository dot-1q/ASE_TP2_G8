#include "store_songs.h"



void store_song(EEPROM_t dev, int16_t song[], uint16_t start_addr, uint8_t lenght){


    // Write Data
	// uint16_t wdata[lenght];

    int addr = start_addr;

    int len =  eeprom_WriteByte(&dev, addr, lenght);
    ESP_LOGI(TAG, "WriteByte(addr=%d) len=%d value=%d", addr, len, lenght);
    if (len != 1) {
        ESP_LOGE(TAG, "WriteByte Fail addr=%d", addr);
    }
	addr++;

    printf("Writing %d shorts\n", lenght);

    uint8_t c1;
    uint8_t c2;

	for(int i = 0; i < lenght; i++){
        // wdata[i] = song[i];

        c1 = song[i] & 0xFF;
        c2 = song[i] >> 8;

        len =  eeprom_WriteByte(&dev, addr, c1);
        ESP_LOGI(TAG, "WriteByte(addr=%d) len=%d value=%d", addr, len, c1);
        if (len != 1) {
            ESP_LOGE(TAG, "WriteByte Fail addr=%d", addr);
        }
        addr++;
        len =  eeprom_WriteByte(&dev, addr, c2);
        ESP_LOGI(TAG, "WriteByte(addr=%d) len=%d value=%d", addr, len, c2);
        if (len != 1) {
            ESP_LOGE(TAG, "WriteByte Fail addr=%d", addr);
        }
        addr++;
    }

    ESP_LOGI(TAG, "Wrote song form addr: %d to addr: %d", start_addr, addr);

}