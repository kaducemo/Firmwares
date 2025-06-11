################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/libsodium/sodium/codecs.c \
../source/libsodium/sodium/core.c \
../source/libsodium/sodium/runtime.c \
../source/libsodium/sodium/utils.c \
../source/libsodium/sodium/version.c 

C_DEPS += \
./source/libsodium/sodium/codecs.d \
./source/libsodium/sodium/core.d \
./source/libsodium/sodium/runtime.d \
./source/libsodium/sodium/utils.d \
./source/libsodium/sodium/version.d 

OBJS += \
./source/libsodium/sodium/codecs.o \
./source/libsodium/sodium/core.o \
./source/libsodium/sodium/runtime.o \
./source/libsodium/sodium/utils.o \
./source/libsodium/sodium/version.o 


# Each subdirectory must supply rules for building sources it contributes
source/libsodium/sodium/%.o: ../source/libsodium/sodium/%.c source/libsodium/sodium/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include\sodium" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-libsodium-2f-sodium

clean-source-2f-libsodium-2f-sodium:
	-$(RM) ./source/libsodium/sodium/codecs.d ./source/libsodium/sodium/codecs.o ./source/libsodium/sodium/core.d ./source/libsodium/sodium/core.o ./source/libsodium/sodium/runtime.d ./source/libsodium/sodium/runtime.o ./source/libsodium/sodium/utils.d ./source/libsodium/sodium/utils.o ./source/libsodium/sodium/version.d ./source/libsodium/sodium/version.o

.PHONY: clean-source-2f-libsodium-2f-sodium

