################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adbms_driver.c \
../Core/Src/adbms_interface.c \
../Core/Src/main.c \
../Core/Src/nfr_can_driver.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/thermistor_driver.c \
../Core/Src/virtual_timer.c 

OBJS += \
./Core/Src/adbms_driver.o \
./Core/Src/adbms_interface.o \
./Core/Src/main.o \
./Core/Src/nfr_can_driver.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/thermistor_driver.o \
./Core/Src/virtual_timer.o 

C_DEPS += \
./Core/Src/adbms_driver.d \
./Core/Src/adbms_interface.d \
./Core/Src/main.d \
./Core/Src/nfr_can_driver.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/thermistor_driver.d \
./Core/Src/virtual_timer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -DUSE_PWR_LDO_SUPPLY -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/adbms_driver.cyclo ./Core/Src/adbms_driver.d ./Core/Src/adbms_driver.o ./Core/Src/adbms_driver.su ./Core/Src/adbms_interface.cyclo ./Core/Src/adbms_interface.d ./Core/Src/adbms_interface.o ./Core/Src/adbms_interface.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/nfr_can_driver.cyclo ./Core/Src/nfr_can_driver.d ./Core/Src/nfr_can_driver.o ./Core/Src/nfr_can_driver.su ./Core/Src/stm32h7xx_hal_msp.cyclo ./Core/Src/stm32h7xx_hal_msp.d ./Core/Src/stm32h7xx_hal_msp.o ./Core/Src/stm32h7xx_hal_msp.su ./Core/Src/stm32h7xx_it.cyclo ./Core/Src/stm32h7xx_it.d ./Core/Src/stm32h7xx_it.o ./Core/Src/stm32h7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/thermistor_driver.cyclo ./Core/Src/thermistor_driver.d ./Core/Src/thermistor_driver.o ./Core/Src/thermistor_driver.su ./Core/Src/virtual_timer.cyclo ./Core/Src/virtual_timer.d ./Core/Src/virtual_timer.o ./Core/Src/virtual_timer.su

.PHONY: clean-Core-2f-Src

