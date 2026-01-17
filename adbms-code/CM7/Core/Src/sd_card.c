#include "sd_card.h"

FATFS fs;    // File system object
FIL fil;     // File object
FRESULT fres; // Result code

void sd_log_string(char* data)
{
    // Mount the SD Card
    fres = f_mount(&fs, "", 1); //The "" forces it to use the default drive The 1 forces it to mount immediately
    if (fres == FR_OK) {
        printf("Mount Success!\n");
    } else if (fres == FR_NOT_READY) { // Error 3
        printf("Error: SD Card not detected (Check CD pin or connections)\n");
        return;
    } else if (fres == FR_DISK_ERR) { // Error 1
        printf("Error: Low Level Hardware Error (Check ClockDiv or MPU)\n");
        return;
    } else if (fres == FR_NO_FILESYSTEM)  { // Error 13
        printf("Error: Card needs FAT32 formatting\n");
        return;
    } else {
        printf("Mount Error: %d\n", fres);
        return;
    }

    // Open the file
    // Opens the file if it exists, or creates a new one.
    // FA_WRITE: Opens for writing
    // FA_OPEN_APPEND: Move to the end of the file (so we don't overwrite old data)
    fres = f_open(&fil, "data.txt", FA_WRITE | FA_OPEN_ALWAYS | FA_OPEN_APPEND);
    if (fres != FR_OK) {
        printf("SD File Open Error: %d\r\n", fres);
    } else {
        //Write buffer to the file
        UINT bytesWrote;
        fres = f_write(&fil, data, strlen(data), &bytesWrote);

        if (fres == FR_OK) {
            printf("SD Wrote: %s", data); // Print what was logged
        } else {
            printf("SD Write Error: %d\r\n", fres);
        }

        //Close file
        f_close(&fil);
    }

    //Unmount, maybe not needed
    f_mount(NULL, "", 0);
}

void print_sd_info()
{
  //printf("SD Card Information:\n");
  //printf("Block size  : %lu\n", hsd1.SdCard.BlockSize);
  //printf("Block nmbr  : %lu\n", hsd1.SdCard.BlockNbr);
  //printf("Card size   : %lu\n", (hsd1.SdCard.BlockSize * hsd1.SdCard.BlockNbr) / 1000);
  //printf("Card version: %lu\n", hsd1.SdCard.CardVersion);
}

//example of using:
// for (int i=0; i<10; i++ ){
//     // Create a buffer for the string
//     char sd_buffer[100];

//     // Write to buffer
//     sprintf(sd_buffer, "Random Data %d\n", i);

//     // Write it to SD
//     sd_log_string(sd_buffer);
// }
