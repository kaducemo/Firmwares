################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BearSSL/src/x509/asn1enc.c \
../source/BearSSL/src/x509/encode_ec_pk8der.c \
../source/BearSSL/src/x509/encode_ec_rawder.c \
../source/BearSSL/src/x509/encode_rsa_pk8der.c \
../source/BearSSL/src/x509/encode_rsa_rawder.c \
../source/BearSSL/src/x509/skey_decoder.c \
../source/BearSSL/src/x509/x509_decoder.c \
../source/BearSSL/src/x509/x509_knownkey.c \
../source/BearSSL/src/x509/x509_minimal.c \
../source/BearSSL/src/x509/x509_minimal_full.c 

C_DEPS += \
./source/BearSSL/src/x509/asn1enc.d \
./source/BearSSL/src/x509/encode_ec_pk8der.d \
./source/BearSSL/src/x509/encode_ec_rawder.d \
./source/BearSSL/src/x509/encode_rsa_pk8der.d \
./source/BearSSL/src/x509/encode_rsa_rawder.d \
./source/BearSSL/src/x509/skey_decoder.d \
./source/BearSSL/src/x509/x509_decoder.d \
./source/BearSSL/src/x509/x509_knownkey.d \
./source/BearSSL/src/x509/x509_minimal.d \
./source/BearSSL/src/x509/x509_minimal_full.d 

OBJS += \
./source/BearSSL/src/x509/asn1enc.o \
./source/BearSSL/src/x509/encode_ec_pk8der.o \
./source/BearSSL/src/x509/encode_ec_rawder.o \
./source/BearSSL/src/x509/encode_rsa_pk8der.o \
./source/BearSSL/src/x509/encode_rsa_rawder.o \
./source/BearSSL/src/x509/skey_decoder.o \
./source/BearSSL/src/x509/x509_decoder.o \
./source/BearSSL/src/x509/x509_knownkey.o \
./source/BearSSL/src/x509/x509_minimal.o \
./source/BearSSL/src/x509/x509_minimal_full.o 


# Each subdirectory must supply rules for building sources it contributes
source/BearSSL/src/x509/%.o: ../source/BearSSL/src/x509/%.c source/BearSSL/src/x509/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-BearSSL-2f-src-2f-x509

clean-source-2f-BearSSL-2f-src-2f-x509:
	-$(RM) ./source/BearSSL/src/x509/asn1enc.d ./source/BearSSL/src/x509/asn1enc.o ./source/BearSSL/src/x509/encode_ec_pk8der.d ./source/BearSSL/src/x509/encode_ec_pk8der.o ./source/BearSSL/src/x509/encode_ec_rawder.d ./source/BearSSL/src/x509/encode_ec_rawder.o ./source/BearSSL/src/x509/encode_rsa_pk8der.d ./source/BearSSL/src/x509/encode_rsa_pk8der.o ./source/BearSSL/src/x509/encode_rsa_rawder.d ./source/BearSSL/src/x509/encode_rsa_rawder.o ./source/BearSSL/src/x509/skey_decoder.d ./source/BearSSL/src/x509/skey_decoder.o ./source/BearSSL/src/x509/x509_decoder.d ./source/BearSSL/src/x509/x509_decoder.o ./source/BearSSL/src/x509/x509_knownkey.d ./source/BearSSL/src/x509/x509_knownkey.o ./source/BearSSL/src/x509/x509_minimal.d ./source/BearSSL/src/x509/x509_minimal.o ./source/BearSSL/src/x509/x509_minimal_full.d ./source/BearSSL/src/x509/x509_minimal_full.o

.PHONY: clean-source-2f-BearSSL-2f-src-2f-x509

