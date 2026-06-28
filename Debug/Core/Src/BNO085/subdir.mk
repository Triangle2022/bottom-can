################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/BNO085/ceva_bno085.c \
../Core/Src/BNO085/sh2.c \
../Core/Src/BNO085/sh2_SensorValue.c \
../Core/Src/BNO085/sh2_util.c \
../Core/Src/BNO085/shtp.c 

OBJS += \
./Core/Src/BNO085/ceva_bno085.o \
./Core/Src/BNO085/sh2.o \
./Core/Src/BNO085/sh2_SensorValue.o \
./Core/Src/BNO085/sh2_util.o \
./Core/Src/BNO085/shtp.o 

C_DEPS += \
./Core/Src/BNO085/ceva_bno085.d \
./Core/Src/BNO085/sh2.d \
./Core/Src/BNO085/sh2_SensorValue.d \
./Core/Src/BNO085/sh2_util.d \
./Core/Src/BNO085/shtp.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/BNO085/%.o Core/Src/BNO085/%.su Core/Src/BNO085/%.cyclo: ../Core/Src/BNO085/%.c Core/Src/BNO085/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Core/Inc/BNO085 -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32G4xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-BNO085

clean-Core-2f-Src-2f-BNO085:
	-$(RM) ./Core/Src/BNO085/ceva_bno085.cyclo ./Core/Src/BNO085/ceva_bno085.d ./Core/Src/BNO085/ceva_bno085.o ./Core/Src/BNO085/ceva_bno085.su ./Core/Src/BNO085/sh2.cyclo ./Core/Src/BNO085/sh2.d ./Core/Src/BNO085/sh2.o ./Core/Src/BNO085/sh2.su ./Core/Src/BNO085/sh2_SensorValue.cyclo ./Core/Src/BNO085/sh2_SensorValue.d ./Core/Src/BNO085/sh2_SensorValue.o ./Core/Src/BNO085/sh2_SensorValue.su ./Core/Src/BNO085/sh2_util.cyclo ./Core/Src/BNO085/sh2_util.d ./Core/Src/BNO085/sh2_util.o ./Core/Src/BNO085/sh2_util.su ./Core/Src/BNO085/shtp.cyclo ./Core/Src/BNO085/shtp.d ./Core/Src/BNO085/shtp.o ./Core/Src/BNO085/shtp.su

.PHONY: clean-Core-2f-Src-2f-BNO085

