################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/encTrans/gpio_rdwr.cpp \
../src/encTrans/gst_capture.cpp \
../src/encTrans/spidev_trans.cpp \
../src/encTrans/sync422_trans.cpp 

CU_SRCS += \
../src/encTrans/cuda_convert.cu 

CU_DEPS += \
./src/encTrans/cuda_convert.d 

OBJS += \
./src/encTrans/cuda_convert.o \
./src/encTrans/gpio_rdwr.o \
./src/encTrans/gst_capture.o \
./src/encTrans/spidev_trans.o \
./src/encTrans/sync422_trans.o 

CPP_DEPS += \
./src/encTrans/gpio_rdwr.d \
./src/encTrans/gst_capture.d \
./src/encTrans/spidev_trans.d \
./src/encTrans/sync422_trans.d 


# Each subdirectory must supply rules for building sources it contributes
src/encTrans/%.o: ../src/encTrans/%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/encTrans" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile --relocatable-device-code=false -gencode arch=compute_50,code=compute_50 -gencode arch=compute_50,code=sm_50 -m64 -ccbin aarch64-linux-gnu-g++  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/encTrans/%.o: ../src/encTrans/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/encTrans" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/OSA_CAP/inc -I../src/encTrans -I../src -G -g -O0 --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


