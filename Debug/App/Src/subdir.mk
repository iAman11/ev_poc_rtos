################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/app_adc.c \
../App/Src/app_evse.c \
../App/Src/app_led.c \
../App/Src/app_main.c \
../App/Src/app_pwm.c \
../App/Src/app_relay.c 

OBJS += \
./App/Src/app_adc.o \
./App/Src/app_evse.o \
./App/Src/app_led.o \
./App/Src/app_main.o \
./App/Src/app_pwm.o \
./App/Src/app_relay.o 

C_DEPS += \
./App/Src/app_adc.d \
./App/Src/app_evse.d \
./App/Src/app_led.d \
./App/Src/app_main.d \
./App/Src/app_pwm.d \
./App/Src/app_relay.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.c App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F030x8 -c -I../Core/Inc -I"C:/Users/INTERN_AMAN/STM32CubeIDE/workspace_1.19.0/moduler_firmware/App" -I"C:/Users/INTERN_AMAN/STM32CubeIDE/workspace_1.19.0/moduler_firmware/App/Inc" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-App-2f-Src

clean-App-2f-Src:
	-$(RM) ./App/Src/app_adc.cyclo ./App/Src/app_adc.d ./App/Src/app_adc.o ./App/Src/app_adc.su ./App/Src/app_evse.cyclo ./App/Src/app_evse.d ./App/Src/app_evse.o ./App/Src/app_evse.su ./App/Src/app_led.cyclo ./App/Src/app_led.d ./App/Src/app_led.o ./App/Src/app_led.su ./App/Src/app_main.cyclo ./App/Src/app_main.d ./App/Src/app_main.o ./App/Src/app_main.su ./App/Src/app_pwm.cyclo ./App/Src/app_pwm.d ./App/Src/app_pwm.o ./App/Src/app_pwm.su ./App/Src/app_relay.cyclo ./App/Src/app_relay.d ./App/Src/app_relay.o ./App/Src/app_relay.su

.PHONY: clean-App-2f-Src

