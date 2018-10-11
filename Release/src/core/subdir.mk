################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/Displayer.cpp \
../src/core/GeneralProcess.cpp \
../src/core/crCore.cpp \
../src/core/cuda_mem.cpp \
../src/core/encTrans.cpp \
../src/core/mmtdProcess.cpp \
../src/core/trackerProcess.cpp 

CU_SRCS += \
../src/core/cuda.cu \
../src/core/cuda_convert.cu 

CU_DEPS += \
./src/core/cuda.d \
./src/core/cuda_convert.d 

OBJS += \
./src/core/Displayer.o \
./src/core/GeneralProcess.o \
./src/core/crCore.o \
./src/core/cuda.o \
./src/core/cuda_convert.o \
./src/core/cuda_mem.o \
./src/core/encTrans.o \
./src/core/mmtdProcess.o \
./src/core/trackerProcess.o 

CPP_DEPS += \
./src/core/Displayer.d \
./src/core/GeneralProcess.d \
./src/core/crCore.d \
./src/core/cuda_mem.d \
./src/core/encTrans.d \
./src/core/mmtdProcess.d \
./src/core/trackerProcess.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/%.o: ../src/core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/core" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp --compile -m64 -ccbin aarch64-linux-gnu-g++  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/core/%.o: ../src/core/%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp -ccbin aarch64-linux-gnu-g++ -gencode arch=compute_50,code=sm_50 -m64 -odir "src/core" -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -I/usr/include/opencv -I/usr/include/GL -I../include -I../src/capture -I../src/encTrans -I../src -I../src/core -I../src/cr_osa/inc -I../src/nvMedia -O3 -Xcompiler -fopenmp --compile --relocatable-device-code=false -gencode arch=compute_50,code=compute_50 -gencode arch=compute_50,code=sm_50 -m64 -ccbin aarch64-linux-gnu-g++  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


