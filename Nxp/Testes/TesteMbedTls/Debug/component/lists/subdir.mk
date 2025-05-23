################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/lists/fsl_component_generic_list.c 

C_DEPS += \
./component/lists/fsl_component_generic_list.d 

OBJS += \
./component/lists/fsl_component_generic_list.o 


# Each subdirectory must supply rules for building sources it contributes
component/lists/%.o: ../component/lists/%.c component/lists/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSERIAL_PORT_TYPE_UART=1 -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\board" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\source" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\drivers" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\device" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\CMSIS" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\utilities" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\component\serial_manager" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\component\uart" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\mbedtls\include" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\mbedtls\library" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\mbedtls\port\ksdk" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\mmcau" -I"C:\GIT\Firmwares\NXP\EstudosTestes\TesteMbedTls\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-lists

clean-component-2f-lists:
	-$(RM) ./component/lists/fsl_component_generic_list.d ./component/lists/fsl_component_generic_list.o

.PHONY: clean-component-2f-lists

