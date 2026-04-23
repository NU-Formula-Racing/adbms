################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
OBJS += \
./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o 

C_DEPS += \
./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o: C:/Users/drake/OneDrive/Desktop/Formula/adbms/adbms-code/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c Core/Inc/Common/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -DUSE_PWR_LDO_SUPPLY -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../FATFS/Target -I../FATFS/App -I../../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Inc-2f-Common-2f-Src

clean-Core-2f-Inc-2f-Common-2f-Src:
	-$(RM) ./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.cyclo ./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d ./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o ./Core/Inc/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.su

.PHONY: clean-Core-2f-Inc-2f-Common-2f-Src

