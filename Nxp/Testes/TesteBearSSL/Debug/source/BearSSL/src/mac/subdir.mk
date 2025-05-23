################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BearSSL/src/mac/hmac.c \
../source/BearSSL/src/mac/hmac_ct.c 

C_DEPS += \
./source/BearSSL/src/mac/hmac.d \
./source/BearSSL/src/mac/hmac_ct.d 

OBJS += \
./source/BearSSL/src/mac/hmac.o \
./source/BearSSL/src/mac/hmac_ct.o 


# Each subdirectory must supply rules for building sources it contributes
source/BearSSL/src/mac/%.o: ../source/BearSSL/src/mac/%.c source/BearSSL/src/mac/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\board" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\source" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\device" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\GITHUB\Firmwares\Nxp\Testes\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-BearSSL-2f-src-2f-mac

clean-source-2f-BearSSL-2f-src-2f-mac:
	-$(RM) ./source/BearSSL/src/mac/hmac.d ./source/BearSSL/src/mac/hmac.o ./source/BearSSL/src/mac/hmac_ct.d ./source/BearSSL/src/mac/hmac_ct.o

.PHONY: clean-source-2f-BearSSL-2f-src-2f-mac

