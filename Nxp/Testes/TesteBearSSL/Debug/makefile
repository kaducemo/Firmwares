################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/BearSSL/src/x509/subdir.mk
-include source/BearSSL/src/symcipher/subdir.mk
-include source/BearSSL/src/ssl/subdir.mk
-include source/BearSSL/src/rsa/subdir.mk
-include source/BearSSL/src/rand/subdir.mk
-include source/BearSSL/src/mac/subdir.mk
-include source/BearSSL/src/kdf/subdir.mk
-include source/BearSSL/src/int/subdir.mk
-include source/BearSSL/src/hash/subdir.mk
-include source/BearSSL/src/ec/subdir.mk
-include source/BearSSL/src/codec/subdir.mk
-include source/BearSSL/src/aead/subdir.mk
-include source/BearSSL/src/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := TesteBearSSL
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: TesteBearSSL.axf

# Tool invocations
TesteBearSSL.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="TesteBearSSL.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mthumb -T TesteBearSSL_Debug.ld -o "TesteBearSSL.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) TesteBearSSL.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "TesteBearSSL.axf"; # arm-none-eabi-objcopy -v -O binary "TesteBearSSL.axf" "TesteBearSSL.bin" ; # checksum -p MK64FN1M0xxx12 -d "TesteBearSSL.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
