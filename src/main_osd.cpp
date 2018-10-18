/*
 * main_osd.cpp
 *
 *  Created on: 2018年8月28日
 *      Author: fsmdn121
 */
/*
 * main.cpp
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include <math.h>
#include"ChosenCaptureGroup.h"
#include "main.h"
#include "StlGlDefines.h"
#include "gst_capture.h"
#include "cuda_convert.cuh"
#include "sync422_trans.h"
#include "Displayer.hpp"
#include "osa_image_queue.h"

extern RecordHandle * record_handle[QUE_CHID_COUNT];
extern void initGstCap(void *notifys[]);
extern void UninitGstCap();

static int curChannel = TV_DEV_ID;
static Mat osd_test_TV;
static Mat osd_test_HOT;
static unsigned char *memI420Tv = NULL;
static unsigned char *memI420Flr = NULL;
static unsigned char *memOsdTv = NULL;
static unsigned char *memOsdFlr = NULL;
static OSA_BufHndl *imgQ[QUE_CHID_COUNT];
static int retRate[QUE_CHID_COUNT]={-1,-1};
static CRender *render = NULL;

void processFrame_osd(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	Mat img, i420, bgr;
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
		return;

	if(cap_chid == curChannel)
	{
		if(cap_chid==TV_DEV_ID)
		{
			putText(osd_test_TV, "osd text test",Point(TV_WIDTH-300,50), CV_FONT_HERSHEY_COMPLEX, 1.0,cvScalar(255));
			img = Mat(TV_HEIGHT,TV_WIDTH,CV_8UC2,src);
			i420= Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, memI420Tv);
			cuConvert_async(cap_chid, img, osd_test_TV, i420, 1, GREENCOLOR);
			//OSD_gpuConvertYUYVtoI420(osd_test_TV.data,img.data, i420.data, TV_WIDTH,TV_HEIGHT);
			//gpuConvertYUYVtoI420(img.data, i420.data, TV_WIDTH,TV_HEIGHT);
			gstCapturePushData(record_handle[TV_DEV_ID],  (char *)i420.data ,
					i420.cols*i420.rows*i420.channels());
		}
		else if(cap_chid==HOT_DEV_ID)
		{
			putText(osd_test_HOT, "osd text test",Point(HOT_WIDTH-300,50), CV_FONT_HERSHEY_COMPLEX, 1.0,cvScalar(255));
			img = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,src);
			i420= Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, memI420Flr);
			cuConvert_async(cap_chid, img, osd_test_HOT, i420, 1, BLUECOLOR);
			//OSD_gpuConvertGRAYtoI420(osd_test_HOT.data,img.data, i420.data, HOT_WIDTH,HOT_HEIGHT);
			//memcpy(i420.data, img.data, img.rows*img.cols);
			//memset(i420.data+img.rows*img.cols, 0x80, img.rows*img.cols/2);
			gstCapturePushData(record_handle[HOT_DEV_ID],  (char *)i420.data,
					i420.cols*i420.rows*i420.channels());
		}

		if(cap_chid == curChannel)
		{
			OSA_BufInfo* info = image_queue_getEmpty(imgQ[cap_chid]);
			if(info != NULL)
			{
				bgr = Mat(img.rows,img.cols,CV_8UC3, info->physAddr);
				cuConvertConn_yuv2bgr_i420(cap_chid, bgr, CUT_FLAG_devAlloc);
				info->chId = cap_chid;
				info->channels = bgr.channels();
				info->width = bgr.cols;
				info->height = bgr.rows;
				info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000ul
						+ (uint64)capInfo.timestamp.tv_usec*1000ul;
				info->timestamp = (uint64_t)getTickCount();
				image_queue_putFull(imgQ[cap_chid], info);
			}
		}
	}
}

static void keyboard_event(unsigned char key, int x, int y)
{
	int ret = OSA_SOK;
	switch(key)
	{
	case '0':
		curChannel = TV_DEV_ID;
		render->dynamic_config(CRender::DS_CFG_ChId, 0, &curChannel);
		break;
	case '1':
		curChannel = HOT_DEV_ID;
		render->dynamic_config(CRender::DS_CFG_ChId, 0, &curChannel);
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
	case 'q':
	case 27:
		glutLeaveMainLoop();
		break;
	default:
		break;
	}
	OSA_printf("TV rate=%d     HOT rate=%d\n",retRate[TV_DEV_ID],retRate[HOT_DEV_ID]);
}

int main_osd(int argc, char **argv)
{
	int ret = OSA_SOK;
	cuConvertInit(QUE_CHID_COUNT);

	DS_InitPrm dsInit;
	memset(&dsInit, 0, sizeof(DS_InitPrm));
	render = CRender::createObject();
	render->create();
	OSA_printf("render create success!");
	dsInit.bFullScreen = true;
	dsInit.keyboardfunc = keyboard_event;
	dsInit.nChannels = QUE_CHID_COUNT;
	dsInit.memType = memtype_glpbo;
	dsInit.nQueueSize = 2;
	dsInit.disFPS = DIS_FPS;
	dsInit.channelsSize[0].w = TV_WIDTH;
	dsInit.channelsSize[0].h = TV_HEIGHT;
	dsInit.channelsSize[0].c = 3;
	dsInit.channelsSize[1].w = HOT_WIDTH;
	dsInit.channelsSize[1].h = HOT_HEIGHT;
	dsInit.channelsSize[1].c = 3;
	render->init(&dsInit);
	render->run();
	imgQ[0] = &render->m_bufQue[0];
	imgQ[1] = &render->m_bufQue[1];

	sync422_spi_create(0,0);// 0 0
	ret = sync422_ontime_ctrl(ctrl_prm_framerate, TV_DEV_ID, TV_FPS); OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_framerate, HOT_DEV_ID, HOT_FPS);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, 2);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_chlMask, 0, 0x03);OSA_assert(ret == OSA_SOK);

	initGstCap(NULL);
	retRate[TV_DEV_ID]=ChangeBitRate(record_handle[HOT_DEV_ID],BITRATE_2M);
	retRate[HOT_DEV_ID]=ChangeBitRate(record_handle[TV_DEV_ID],BITRATE_2M);
	ChangeQP_range(record_handle[HOT_DEV_ID],MIN_QP_2M, MAX_QP, MIN_I_2M, MAX_I, -1, -1);
	ChangeQP_range(record_handle[TV_DEV_ID],MIN_QP_2M, MAX_QP, MIN_I_2M, MAX_I, -1, -1);

	cudaHostAlloc((void**)&memI420Tv, 1920*1280*3/2, cudaHostAllocDefault);
	cudaHostAlloc((void**)&memI420Flr, 1920*1280*3/2, cudaHostAllocDefault);
	cudaHostAlloc((void**)&memOsdTv, TV_WIDTH*TV_HEIGHT, cudaHostAllocDefault);
	cudaHostAlloc((void**)&memOsdFlr, HOT_WIDTH*HOT_HEIGHT, cudaHostAllocDefault);

	osd_test_TV = Mat(TV_HEIGHT,TV_WIDTH,CV_8UC1,memOsdTv);
	osd_test_HOT = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,memOsdFlr);

	memset(osd_test_TV.data, 0, osd_test_TV.cols*osd_test_TV.rows*osd_test_TV.channels());
	memset(osd_test_HOT.data, 0, osd_test_HOT.cols*osd_test_HOT.rows*osd_test_HOT.channels());

	cv::line(osd_test_TV, Point(0, TV_HEIGHT/2+1), Point(TV_WIDTH/2, TV_HEIGHT/2+1), cvScalar(255) ,2);
	cv::line(osd_test_TV, Point(0, TV_HEIGHT/4), Point(TV_WIDTH/2, TV_HEIGHT/4), cvScalar(255) ,2);
	cv::line(osd_test_TV, Point(0, 0), Point(TV_WIDTH/2, TV_HEIGHT/2), cvScalar(255) ,2);

	cv::line(osd_test_HOT, Point(0, HOT_HEIGHT/2+1), Point(HOT_WIDTH/2, HOT_HEIGHT/2+1), cvScalar(255) ,2);
	cv::line(osd_test_HOT, Point(0, HOT_HEIGHT/4), Point(HOT_WIDTH/2, HOT_HEIGHT/4), cvScalar(255) ,2);
	cv::line(osd_test_HOT, Point(0, 0), Point(HOT_WIDTH/2, HOT_HEIGHT/2), cvScalar(255) ,2);

	ChosenCaptureGroup *grop[2];
	grop[0] = ChosenCaptureGroup::GetTVInstance();
	grop[1] = ChosenCaptureGroup::GetHOTInstance();

	glutMainLoop();

	UninitGstCap();
	sync422_spi_destory(0);
	cudaFreeHost(memI420Tv);
	cudaFreeHost(memI420Flr);
	cudaFreeHost(memOsdTv);
	cudaFreeHost(memOsdFlr);
	render->stop();
	render->destroy();
	CRender::destroyObject(render);
	cuConvertUinit();

	return 0;
}
