################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BearSSL/src/aead/ccm.c \
../source/BearSSL/src/aead/eax.c \
../source/BearSSL/src/aead/gcm.c 

C_DEPS += \
./source/BearSSL/src/aead/ccm.d \
./source/BearSSL/src/aead/eax.d \
./source/BearSSL/src/aead/gcm.d 

OBJS += \
./source/BearSSL/src/aead/ccm.o \
./source/BearSSL/src/aead/eax.o \
./source/BearSSL/src/aead/gcm.o 


# Each subdirectory must supply rules for building sources it contributes
source/BearSSL/src/aead/%.o: ../source/BearSSL/src/aead/%.c source/BearSSL/src/aead/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-BearSSL-2f-src-2f-aead

clean-source-2f-BearSSL-2f-src-2f-aead:
	-$(RM) ./source/BearSSL/src/aead/ccm.d ./source/BearSSL/src/aead/ccm.o ./source/BearSSL/src/aead/eax.d ./source/BearSSL/src/aead/eax.o ./source/BearSSL/src/aead/gcm.d ./source/BearSSL/src/aead/gcm.o

.PHONY: clean-source-2f-BearSSL-2f-src-2f-aead

