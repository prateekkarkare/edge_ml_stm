################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Prateek\ Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c 

OBJS += \
./Middlewares/STM32_USBD_Library/Class/usbd_cdc.o 

C_DEPS += \
./Middlewares/STM32_USBD_Library/Class/usbd_cdc.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/STM32_USBD_Library/Class/usbd_cdc.o: C:/Users/Prateek\ Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo %cd%
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DUSE_HAL_DRIVER -DOSX_BMS_SENSORTILE -DSTM32L476xx -DUSE_STM32L4XX_NUCLEO -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Projects/SensorTile/Applications/DataLog/Inc" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/CMSIS/Device/ST/STM32L4xx/Include" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/STM32L4xx_HAL_Driver/Inc" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/CMSIS/Include" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/Common" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/hts221" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/lsm6dsm" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/lps22hb" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/lsm303agr" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/Components/stc3115" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Drivers/BSP/SensorTile" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/Third_Party/FatFs/src" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/Third_Party/FatFs/src/drivers" -I"C:/Users/Prateek Karkare/Desktop/Work/SensorTile/Sensor/v1.2.0/Middlewares/ST/STM32_USB_Device_Library/Core/Inc"  -O0 -g1 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"Middlewares/STM32_USBD_Library/Class/usbd_cdc.d" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


