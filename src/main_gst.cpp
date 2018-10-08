/*
 * main.cpp
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */

#include"ChosenCaptureGroup.h"
#include "main.h"
#include "StlGlDefines.h"
#include "gst_capture.h"
#include "cuda_convert.cuh"
#include "sync422_trans.h"
static unsigned char *memI420 = NULL;

void processFrame_gst(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	Mat img, i420;
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
		return;
	if(1)
	{
		if(cap_chid==TV_DEV_ID)
		{
			img = Mat(TV_HEIGHT,TV_WIDTH,CV_8UC2,src);
			i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, memI420);
			gpuConvertYUYVtoI420(img.data, i420.data, TV_WIDTH,TV_HEIGHT);
			gstCapturePushData(record_handle[TV_DEV_ID],  (char *)i420.data ,
					i420.cols*i420.rows*i420.channels());
		}
		else if(cap_chid==HOT_DEV_ID)
		{
			img = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,src);
			i420= Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1);
			memcpy(i420.data, img.data, img.rows*img.cols);
			memset(i420.data+img.rows*img.cols, 0x80, img.rows*img.cols/2);

			gstCapturePushData(record_handle[HOT_DEV_ID],  (char *)i420.data,
					i420.cols*i420.rows*i420.channels());
		}
	}
}

int main_gst(int argc, char **argv)
{
	int ret = OSA_SOK;
	printf("sync422_spi_create()...\n");
	sync422_spi_create(0,0);// 0 0
	ret = sync422_ontime_ctrl(ctrl_prm_framerate, TV_DEV_ID, TV_FPS); OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_framerate, HOT_DEV_ID, HOT_FPS);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, 2);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_chlMask, 0, 0x03);OSA_assert(ret == OSA_SOK);
	printf("sync422_spi_create()... finish\n\n");
	printf("initGstCap()...\n");
	initGstCap();
	printf("initGstCap()... finish\n\n");
	ChosenCaptureGroup *grop[2];
	grop[0] = ChosenCaptureGroup :: GetTVInstance();
	grop[1] = ChosenCaptureGroup::GetHOTInstance();
	int iCur = 0;
	bool loop=true;
	int retRate[2]={-1,-1};

	cudaHostAlloc((void**)&memI420, 1920*1280*3/2, cudaHostAllocDefault);

	int iKey='2';
	while(loop)
	{
		switch(iKey)
		{
		case '1':
			retRate[TV_DEV_ID]=ChangeBitRate(record_handle[TV_DEV_ID],1*1000000);
			retRate[HOT_DEV_ID]=ChangeBitRate(record_handle[HOT_DEV_ID],1*1000000);
			break;
		case '2':
			retRate[TV_DEV_ID]=ChangeBitRate(record_handle[HOT_DEV_ID],BITRATE_2M);
			retRate[HOT_DEV_ID]=ChangeBitRate(record_handle[TV_DEV_ID],BITRATE_2M);
			ChangeQP_range(record_handle[HOT_DEV_ID],MIN_QP_2M, MAX_QP, MIN_I_2M, MAX_I, -1, -1);
			ChangeQP_range(record_handle[TV_DEV_ID],MIN_QP_2M, MAX_QP, MIN_I_2M, MAX_I, -1, -1);
			ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, 0);OSA_assert(ret == OSA_SOK);
					break;
		case '4':
			retRate[TV_DEV_ID]=ChangeBitRate(record_handle[HOT_DEV_ID],BITRATE_4M);
			retRate[HOT_DEV_ID]=ChangeBitRate(record_handle[TV_DEV_ID],BITRATE_4M);
			ChangeQP_range(record_handle[HOT_DEV_ID],MIN_QP_4M, MAX_QP, MIN_I_4M, MAX_I, -1, -1);
			ChangeQP_range(record_handle[TV_DEV_ID],MIN_QP_4M, MAX_QP, MIN_I_4M, MAX_I, -1, -1);
			ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, 1);OSA_assert(ret == OSA_SOK);
						break;
		case '8':
			retRate[TV_DEV_ID]=ChangeBitRate(record_handle[HOT_DEV_ID],BITRATE_8M);
			retRate[HOT_DEV_ID]=ChangeBitRate(record_handle[TV_DEV_ID],BITRATE_8M);
			ChangeQP_range(record_handle[HOT_DEV_ID],MIN_QP_8M, MAX_QP, MIN_I_8M, MAX_I, -1, -1);
			ChangeQP_range(record_handle[TV_DEV_ID],MIN_QP_8M, MAX_QP, MIN_I_8M, MAX_I, -1, -1);
			ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, 2);OSA_assert(ret == OSA_SOK);
						break;
		case '9':
			loop=false;
					break;
		default :
			printf("1  2  4  8        9:exit!\n  ");
					break;
		}
		printf("TV rate=%d     HOT rate=%d\n",retRate[TV_DEV_ID],retRate[HOT_DEV_ID]);
		iKey=getchar();
	}

	UninitGstCap();
	sync422_spi_destory(0);
	cudaFreeHost(memI420);
	return 0;
}
