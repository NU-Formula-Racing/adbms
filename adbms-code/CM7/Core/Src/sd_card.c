#include "sd_card.h"

FATFS fs;   
FIL fil;     
FRESULT fres; 



void sd_init_bin()
{
    char filename_buffer[32];
    int file_index = 0;

    fres = f_mount(&fs, "", 1);
    if (fres != FR_OK) {
        printf("SD Mount Error: %d\r\n", fres);
        return;
    }

    //loop to find number after last existing file
    while (1)
    {
        sprintf(filename_buffer, "bms_data_%d.bin", file_index);

        FRESULT check = f_open(&fil, filename_buffer, FA_READ);

        if (check == FR_OK) 
        {
            //file exists, close and go to next number in loop
            f_close(&fil);
            file_index++;
        }
        else if (check == FR_NO_FILE) 
        {
            //file does not exist, use this as the name to store data
            break;
        }
        else 
        {
            return;
        }
    }

    //create new file to write to with name index found
    fres = f_open(&fil, filename_buffer, FA_WRITE | FA_CREATE_ALWAYS);
    
    if (fres == FR_OK) {
        printf("New Log File Created: %s\r\n", filename_buffer);
        //can add header here?
        
        f_sync(&fil); // f_sync saves changes to SD card immediately
    } else {
        printf("Failed to create file: %d\r\n", fres);
    }
}

void sd_init_csv(int num_voltages, int num_temps)
{
    char buffer[64]; 
    char filename[32];
    int file_index = 0;
    UINT bytesWrote;

    fres = f_mount(&fs, "", 1);
    if (fres != FR_OK) {
        return; //error
    }

    while (1) 
    {
        sprintf(filename, "bms_log_%d.csv", file_index);
        FRESULT check = f_open(&fil, filename, FA_READ);
        
        if (check == FR_OK) {
            f_close(&fil); //file taken
            file_index++;
        } else if (check == FR_NO_FILE) {
            break; //file found
        } else {
            return; //error
        }
    }

    //create file
    fres = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fres != FR_OK) {
        printf("File Create Error: %d\r\n", fres);
        return;
    }
    
    //write csv header
    f_write(&fil, "Tick", 4, &bytesWrote);

    for (int i = 0; i < num_voltages; i++) {
        sprintf(buffer, ",V%d", i + 1);
        f_write(&fil, buffer, strlen(buffer), &bytesWrote);
    }
    for (int i = 0; i < num_temps; i++) {
        sprintf(buffer, ",T%d", i + 1);
        f_write(&fil, buffer, strlen(buffer), &bytesWrote);
    }
    f_write(&fil, "\n", 1, &bytesWrote);

    f_sync(&fil);
    
}

void print_sd_info(SD_HandleTypeDef hsd)
{
  printf("SD Card Information:\n");
  printf("Block size  : %lu\n", hsd.SdCard.BlockSize);
  printf("Block nmbr  : %lu\n", hsd.SdCard.BlockNbr);
  printf("Card size   : %lu\n", (hsd.SdCard.BlockSize * hsd.SdCard.BlockNbr) / 1000);
  printf("Card version: %lu\n", hsd.SdCard.CardVersion);
}

void log_bms_data_bin(uint32_t tick, uint8_t* voltages, int num_voltages, uint8_t* temps, int num_temps)
{
    UINT bytesWrote;
    
    if (fil.obj.fs == NULL) return;   //check if a file is open

    f_write(&fil, &tick, sizeof(uint32_t), &bytesWrote);

    f_write(&fil, voltages, num_voltages * sizeof(uint8_t), &bytesWrote);

    f_write(&fil, temps, num_temps * sizeof(uint8_t), &bytesWrote);

    f_sync(&fil);
}

//csv is a text file, is turned into ASCII
void log_bms_data_csv(uint32_t tick, float* voltages, int num_voltages, float* temps, int num_temps)
{
    char buffer[32];
    UINT bytesWrote;

    if (fil.obj.fs == NULL) return;  //check if a file is open

    sprintf(buffer, "%lu", tick);
    f_write(&fil, buffer, strlen(buffer), &bytesWrote);

    for (int i = 0; i < num_voltages; i++) {
        sprintf(buffer, ",%f", voltages[i]); 
        f_write(&fil, buffer, strlen(buffer), &bytesWrote);
    }

    for (int i = 0; i < num_temps; i++) {
        sprintf(buffer, ",%f", temps[i]);
        f_write(&fil, buffer, strlen(buffer), &bytesWrote);
    }

    f_write(&fil, "\n", 1, &bytesWrote);//end row

    f_sync(&fil);
}

