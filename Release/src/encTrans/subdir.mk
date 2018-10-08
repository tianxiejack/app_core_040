################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/encTrans/gpio_rdwr.cpp \
../src/encTrans/gst_capture.cpp \
../src/encTrans/spidev_trans.cpp \
../src/encTrans/sync422_trans.cpp \
../src/encTrans/vidScheduler.cpp 

OBJS += \
./src/encTrans/gpio_rdwr.o \
./src/encTrans/gst_capture.o \
./src/encTrans/spidev_trans.o \
./src/encTrans/sync422_trans.o \
./src/encTrans/vidScheduler.o 

CPP_DEPS += \
./src/encTrans/gpio_rdwr.d \
./src/encTrans/gst_capture.d \
./src/encTrans/spidev_trans.d \
./src/encTrans/sync422_trans.d \
./src/encTrans/vidScheduler.d 


# Each subdirectory must supply rules for building sources it contributes
src/encTrans/%.o: ../src/encTrans/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/encTrans" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


