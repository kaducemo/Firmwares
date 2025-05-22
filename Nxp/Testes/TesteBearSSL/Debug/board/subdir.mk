################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/peripherals.c \
../board/pin_mux.c 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/peripherals.d \
./board/pin_mux.d 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/peripherals.o \
./board/pin_mux.o 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c board/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\board" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\inc" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source\BearSSL\src" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\source" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\drivers" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\device" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\CMSIS" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\utilities" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\serial_manager" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\uart" -I"C:\Local\FIRMWARE\TESTES\FRDM_K64F\TesteBearSSL\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mthumb -D__NEWLIB__ -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-board

clean-board:
	-$(RM) ./board/board.d ./board/board.o ./board/clock_config.d ./board/clock_config.o ./board/peripherals.d ./board/peripherals.o ./board/pin_mux.d ./board/pin_mux.o

.PHONY: clean-board

