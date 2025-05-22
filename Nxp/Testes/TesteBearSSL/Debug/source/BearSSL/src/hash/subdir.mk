################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BearSSL/src/hash/dig_oid.c \
../source/BearSSL/src/hash/dig_size.c \
../source/BearSSL/src/hash/ghash_ctmul.c \
../source/BearSSL/src/hash/ghash_ctmul32.c \
../source/BearSSL/src/hash/ghash_ctmul64.c \
../source/BearSSL/src/hash/ghash_pclmul.c \
../source/BearSSL/src/hash/ghash_pwr8.c \
../source/BearSSL/src/hash/md5.c \
../source/BearSSL/src/hash/md5sha1.c \
../source/BearSSL/src/hash/mgf1.c \
../source/BearSSL/src/hash/multihash.c \
../source/BearSSL/src/hash/sha1.c \
../source/BearSSL/src/hash/sha2big.c \
../source/BearSSL/src/hash/sha2small.c 

C_DEPS += \
./source/BearSSL/src/hash/dig_oid.d \
./source/BearSSL/src/hash/dig_size.d \
./source/BearSSL/src/hash/ghash_ctmul.d \
./source/BearSSL/src/hash/ghash_ctmul32.d \
./source/BearSSL/src/hash/ghash_ctmul64.d \
./source/BearSSL/src/hash/ghash_pclmul.d \
./source/BearSSL/src/hash/ghash_pwr8.d \
./source/BearSSL/src/hash/md5.d \
./source/BearSSL/src/hash/md5sha1.d \
./source/BearSSL/src/hash/mgf1.d \
./source/BearSSL/src/hash/multihash.d \
./source/BearSSL/src/hash/sha1.d \
./source/BearSSL/src/hash/sha2big.d \
./source/BearSSL/src/hash/sha2small.d 

OBJS += \
./source/BearSSL/src/hash/dig_oid.o \
./source/BearSSL/src/hash/dig_size.o \
./source/BearSSL/src/hash/ghash_ctmul.o \
./source/BearSSL/src/hash/ghash_ctmul32.o \
./source/BearSSL/src/hash/ghash_ctmul64.o \
./source/BearSSL/src/hash/ghash_pclmul.o \
./source/BearSSL/src/hash/ghash_pwr8.o \
./source/BearSSL/src/hash/md5.o \
./source/BearSSL/src/hash/md5sha1.o \
./source/BearSSL/src/hash/mgf1.o \
./source/BearSSL/src/hash/multihash.o \
./source/BearSSL/src/hash/sha1.o \
./source/BearSSL/src/hash/sha2big.o \
./source/BearSSL/src/hash/sha2small.o 


# Each subdirectory must supply rules for building sources it contributes
source/BearSSL/src/hash/%.o: ../source/BearSSL/src/hash/%.c source/BearSSL/src/hash/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-BearSSL-2f-src-2f-hash

clean-source-2f-BearSSL-2f-src-2f-hash:
	-$(RM) ./source/BearSSL/src/hash/dig_oid.d ./source/BearSSL/src/hash/dig_oid.o ./source/BearSSL/src/hash/dig_size.d ./source/BearSSL/src/hash/dig_size.o ./source/BearSSL/src/hash/ghash_ctmul.d ./source/BearSSL/src/hash/ghash_ctmul.o ./source/BearSSL/src/hash/ghash_ctmul32.d ./source/BearSSL/src/hash/ghash_ctmul32.o ./source/BearSSL/src/hash/ghash_ctmul64.d ./source/BearSSL/src/hash/ghash_ctmul64.o ./source/BearSSL/src/hash/ghash_pclmul.d ./source/BearSSL/src/hash/ghash_pclmul.o ./source/BearSSL/src/hash/ghash_pwr8.d ./source/BearSSL/src/hash/ghash_pwr8.o ./source/BearSSL/src/hash/md5.d ./source/BearSSL/src/hash/md5.o ./source/BearSSL/src/hash/md5sha1.d ./source/BearSSL/src/hash/md5sha1.o ./source/BearSSL/src/hash/mgf1.d ./source/BearSSL/src/hash/mgf1.o ./source/BearSSL/src/hash/multihash.d ./source/BearSSL/src/hash/multihash.o ./source/BearSSL/src/hash/sha1.d ./source/BearSSL/src/hash/sha1.o ./source/BearSSL/src/hash/sha2big.d ./source/BearSSL/src/hash/sha2big.o ./source/BearSSL/src/hash/sha2small.d ./source/BearSSL/src/hash/sha2small.o

.PHONY: clean-source-2f-BearSSL-2f-src-2f-hash

