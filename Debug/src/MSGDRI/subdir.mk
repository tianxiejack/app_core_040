################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/MSGDRI/msgDriv.cpp \
../src/MSGDRI/msgPross.cpp 

OBJS += \
./src/MSGDRI/msgDriv.o \
./src/MSGDRI/msgPross.o 

CPP_DEPS += \
./src/MSGDRI/msgDriv.d \
./src/MSGDRI/msgPross.d 


# Each subdirectory must supply rules for building sources it contributes
src/MSGDRI/%.o: ../src/MSGDRI/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/MSGDRI" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


