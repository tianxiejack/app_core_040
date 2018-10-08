################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/capture/Camera.cpp \
../src/capture/CaptureGroup.cpp \
../src/capture/ChosenCaptureGroup.cpp \
../src/capture/HDV4lcap.cpp \
../src/capture/thread.cpp 

OBJS += \
./src/capture/Camera.o \
./src/capture/CaptureGroup.o \
./src/capture/ChosenCaptureGroup.o \
./src/capture/HDV4lcap.o \
./src/capture/thread.o 

CPP_DEPS += \
./src/capture/Camera.d \
./src/capture/CaptureGroup.d \
./src/capture/ChosenCaptureGroup.d \
./src/capture/HDV4lcap.d \
./src/capture/thread.d 


# Each subdirectory must supply rules for building sources it contributes
src/capture/%.o: ../src/capture/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/capture" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


