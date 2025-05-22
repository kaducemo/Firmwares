################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/libsodium/crypto_pwhash/argon2/argon2-core.c \
../source/libsodium/crypto_pwhash/argon2/argon2-encoding.c \
../source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.c \
../source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.c \
../source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.c \
../source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.c \
../source/libsodium/crypto_pwhash/argon2/argon2.c \
../source/libsodium/crypto_pwhash/argon2/blake2b-long.c \
../source/libsodium/crypto_pwhash/argon2/pwhash_argon2i.c \
../source/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c 

C_DEPS += \
./source/libsodium/crypto_pwhash/argon2/argon2-core.d \
./source/libsodium/crypto_pwhash/argon2/argon2-encoding.d \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.d \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.d \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.d \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.d \
./source/libsodium/crypto_pwhash/argon2/argon2.d \
./source/libsodium/crypto_pwhash/argon2/blake2b-long.d \
./source/libsodium/crypto_pwhash/argon2/pwhash_argon2i.d \
./source/libsodium/crypto_pwhash/argon2/pwhash_argon2id.d 

OBJS += \
./source/libsodium/crypto_pwhash/argon2/argon2-core.o \
./source/libsodium/crypto_pwhash/argon2/argon2-encoding.o \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.o \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.o \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.o \
./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.o \
./source/libsodium/crypto_pwhash/argon2/argon2.o \
./source/libsodium/crypto_pwhash/argon2/blake2b-long.o \
./source/libsodium/crypto_pwhash/argon2/pwhash_argon2i.o \
./source/libsodium/crypto_pwhash/argon2/pwhash_argon2id.o 


# Each subdirectory must supply rules for building sources it contributes
source/libsodium/crypto_pwhash/argon2/%.o: ../source/libsodium/crypto_pwhash/argon2/%.c source/libsodium/crypto_pwhash/argon2/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include\sodium" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source\libsodium\include" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\Teste2Sodium\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-libsodium-2f-crypto_pwhash-2f-argon2

clean-source-2f-libsodium-2f-crypto_pwhash-2f-argon2:
	-$(RM) ./source/libsodium/crypto_pwhash/argon2/argon2-core.d ./source/libsodium/crypto_pwhash/argon2/argon2-core.o ./source/libsodium/crypto_pwhash/argon2/argon2-encoding.d ./source/libsodium/crypto_pwhash/argon2/argon2-encoding.o ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.d ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.o ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.d ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.o ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.d ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.o ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.d ./source/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.o ./source/libsodium/crypto_pwhash/argon2/argon2.d ./source/libsodium/crypto_pwhash/argon2/argon2.o ./source/libsodium/crypto_pwhash/argon2/blake2b-long.d ./source/libsodium/crypto_pwhash/argon2/blake2b-long.o ./source/libsodium/crypto_pwhash/argon2/pwhash_argon2i.d ./source/libsodium/crypto_pwhash/argon2/pwhash_argon2i.o ./source/libsodium/crypto_pwhash/argon2/pwhash_argon2id.d ./source/libsodium/crypto_pwhash/argon2/pwhash_argon2id.o

.PHONY: clean-source-2f-libsodium-2f-crypto_pwhash-2f-argon2

