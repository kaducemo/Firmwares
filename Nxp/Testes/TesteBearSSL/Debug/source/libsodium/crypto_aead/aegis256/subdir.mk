################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/libsodium/crypto_aead/aegis256/aead_aegis256.c \
../source/libsodium/crypto_aead/aegis256/aegis256_aesni.c \
../source/libsodium/crypto_aead/aegis256/aegis256_armcrypto.c \
../source/libsodium/crypto_aead/aegis256/aegis256_soft.c 

C_DEPS += \
./source/libsodium/crypto_aead/aegis256/aead_aegis256.d \
./source/libsodium/crypto_aead/aegis256/aegis256_aesni.d \
./source/libsodium/crypto_aead/aegis256/aegis256_armcrypto.d \
./source/libsodium/crypto_aead/aegis256/aegis256_soft.d 

OBJS += \
./source/libsodium/crypto_aead/aegis256/aead_aegis256.o \
./source/libsodium/crypto_aead/aegis256/aegis256_aesni.o \
./source/libsodium/crypto_aead/aegis256/aegis256_armcrypto.o \
./source/libsodium/crypto_aead/aegis256/aegis256_soft.o 


# Each subdirectory must supply rules for building sources it contributes
source/libsodium/crypto_aead/aegis256/%.o: ../source/libsodium/crypto_aead/aegis256/%.c source/libsodium/crypto_aead/aegis256/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include\sodium" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-libsodium-2f-crypto_aead-2f-aegis256

clean-source-2f-libsodium-2f-crypto_aead-2f-aegis256:
	-$(RM) ./source/libsodium/crypto_aead/aegis256/aead_aegis256.d ./source/libsodium/crypto_aead/aegis256/aead_aegis256.o ./source/libsodium/crypto_aead/aegis256/aegis256_aesni.d ./source/libsodium/crypto_aead/aegis256/aegis256_aesni.o ./source/libsodium/crypto_aead/aegis256/aegis256_armcrypto.d ./source/libsodium/crypto_aead/aegis256/aegis256_armcrypto.o ./source/libsodium/crypto_aead/aegis256/aegis256_soft.d ./source/libsodium/crypto_aead/aegis256/aegis256_soft.o

.PHONY: clean-source-2f-libsodium-2f-crypto_aead-2f-aegis256

