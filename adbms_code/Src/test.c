#include "test.h"


int main()
{
    test_interface_raw_functions();
}

void test_interface_raw_functions()
{
    adbms_raw_* adbms_raw = malloc(sizeof(adbms_raw_));
    adbms_2950_* adbms_2950 = malloc(sizeof(adbms_2950_));
    adbms_6830_* adbms_6830 = malloc(sizeof(adbms_6830_));


    ADBMS_Initialize(adbms_raw);
    
}