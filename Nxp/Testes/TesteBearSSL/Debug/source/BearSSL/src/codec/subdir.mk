################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BearSSL/src/codec/ccopy.c \
../source/BearSSL/src/codec/dec16be.c \
../source/BearSSL/src/codec/dec16le.c \
../source/BearSSL/src/codec/dec32be.c \
../source/BearSSL/src/codec/dec32le.c \
../source/BearSSL/src/codec/dec64be.c \
../source/BearSSL/src/codec/dec64le.c \
../source/BearSSL/src/codec/enc16be.c \
../source/BearSSL/src/codec/enc16le.c \
../source/BearSSL/src/codec/enc32be.c \
../source/BearSSL/src/codec/enc32le.c \
../source/BearSSL/src/codec/enc64be.c \
../source/BearSSL/src/codec/enc64le.c \
../source/BearSSL/src/codec/pemdec.c \
../source/BearSSL/src/codec/pemenc.c 

C_DEPS += \
./source/BearSSL/src/codec/ccopy.d \
./source/BearSSL/src/codec/dec16be.d \
./source/BearSSL/src/codec/dec16le.d \
./source/BearSSL/src/codec/dec32be.d \
./source/BearSSL/src/codec/dec32le.d \
./source/BearSSL/src/codec/dec64be.d \
./source/BearSSL/src/codec/dec64le.d \
./source/BearSSL/src/codec/enc16be.d \
./source/BearSSL/src/codec/enc16le.d \
./source/BearSSL/src/codec/enc32be.d \
./source/BearSSL/src/codec/enc32le.d \
./source/BearSSL/src/codec/enc64be.d \
./source/BearSSL/src/codec/enc64le.d \
./source/BearSSL/src/codec/pemdec.d \
./source/BearSSL/src/codec/pemenc.d 

OBJS += \
./source/BearSSL/src/codec/ccopy.o \
./source/BearSSL/src/codec/dec16be.o \
./source/BearSSL/src/codec/dec16le.o \
./source/BearSSL/src/codec/dec32be.o \
./source/BearSSL/src/codec/dec32le.o \
./source/BearSSL/src/codec/dec64be.o \
./source/BearSSL/src/codec/dec64le.o \
./source/BearSSL/src/codec/enc16be.o \
./source/BearSSL/src/codec/enc16le.o \
./source/BearSSL/src/codec/enc32be.o \
./source/BearSSL/src/codec/enc32le.o \
./source/BearSSL/src/codec/enc64be.o \
./source/BearSSL/src/codec/enc64le.o \
./source/BearSSL/src/codec/pemdec.o \
./source/BearSSL/src/codec/pemenc.o 


# Each subdirectory must supply rules for building sources it contributes
source/BearSSL/src/codec/%.o: ../source/BearSSL/src/codec/%.c source/BearSSL/src/codec/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source-2f-BearSSL-2f-src-2f-codec

clean-source-2f-BearSSL-2f-src-2f-codec:
	-$(RM) ./source/BearSSL/src/codec/ccopy.d ./source/BearSSL/src/codec/ccopy.o ./source/BearSSL/src/codec/dec16be.d ./source/BearSSL/src/codec/dec16be.o ./source/BearSSL/src/codec/dec16le.d ./source/BearSSL/src/codec/dec16le.o ./source/BearSSL/src/codec/dec32be.d ./source/BearSSL/src/codec/dec32be.o ./source/BearSSL/src/codec/dec32le.d ./source/BearSSL/src/codec/dec32le.o ./source/BearSSL/src/codec/dec64be.d ./source/BearSSL/src/codec/dec64be.o ./source/BearSSL/src/codec/dec64le.d ./source/BearSSL/src/codec/dec64le.o ./source/BearSSL/src/codec/enc16be.d ./source/BearSSL/src/codec/enc16be.o ./source/BearSSL/src/codec/enc16le.d ./source/BearSSL/src/codec/enc16le.o ./source/BearSSL/src/codec/enc32be.d ./source/BearSSL/src/codec/enc32be.o ./source/BearSSL/src/codec/enc32le.d ./source/BearSSL/src/codec/enc32le.o ./source/BearSSL/src/codec/enc64be.d ./source/BearSSL/src/codec/enc64be.o ./source/BearSSL/src/codec/enc64le.d ./source/BearSSL/src/codec/enc64le.o ./source/BearSSL/src/codec/pemdec.d ./source/BearSSL/src/codec/pemdec.o ./source/BearSSL/src/codec/pemenc.d ./source/BearSSL/src/codec/pemenc.o

.PHONY: clean-source-2f-BearSSL-2f-src-2f-codec

