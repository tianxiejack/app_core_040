################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Displayer.cpp \
../src/VideoProcess.cpp \
../src/cuda_mem.cpp \
../src/encTrans.cpp \
../src/main.cpp \
../src/main_cap.cpp \
../src/main_gst.cpp \
../src/main_osd.cpp \
../src/main_process.cpp \
../src/process040.cpp 

CU_SRCS += \
../src/cuda.cu 

CU_DEPS += \
./src/cuda.d 

OBJS += \
./src/Displayer.o \
./src/VideoProcess.o \
./src/cuda.o \
./src/cuda_mem.o \
./src/encTrans.o \
./src/main.o \
./src/main_cap.o \
./src/main_gst.o \
./src/main_osd.o \
./src/main_process.o \
./src/process040.o 

CPP_DEPS += \
./src/Displayer.d \
./src/VideoProcess.d \
./src/cuda_mem.d \
./src/encTrans.d \
./src/main.d \
./src/main_cap.d \
./src/main_gst.d \
./src/main_osd.d \
./src/main_process.d \
./src/process040.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile --relocatable-device-code=false -gencode arch=compute_50,code=compute_50 -gencode arch=compute_50,code=sm_50 -m64 -ccbin aarch64-linux-gnu-g++  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


