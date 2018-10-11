/*
 * main.cpp
 *
 *  Created on: 2018年8月23日
 *      Author: fsmdn121
 */
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include "ChosenCaptureGroup.h"
#include "Displayer.hpp"
#include "main.h"
#include "StlGlDefines.h"
#include "cuda_convert.cuh"
#include "osa_image_queue.h"


static int curChannel = TV_DEV_ID;
static OSA_BufHndl *imgQ[QUE_CHID_COUNT];
static CRender *render = NULL;

#define NOCAMERA	0

void processFrame_cap(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	static uint64 timestamp[QUE_CHID_COUNT] = {0ul,};
	char WindowName[64]={0};
	Mat img;

	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		OSA_printf("%d V4L2_BUF_FLAG_ERROR", cap_chid);
		return;
	}

	if(curChannel == cap_chid)
	{
		if(cap_chid==TV_DEV_ID)
		{
			//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
			static int a = 0;
			if(!a)
			{
				Mat temp;
				OSA_BufInfo* info = image_queue_getEmpty(imgQ[cap_chid]);
				if(info != NULL)
				{
					img = Mat(TV_HEIGHT,TV_WIDTH,CV_8UC3, info->physAddr);
					temp = Mat(TV_HEIGHT,TV_WIDTH,CV_8UC2,src);
					if(NOCAMERA)
					{
						cv::line(temp, Point(0, TV_HEIGHT/2+1), Point(TV_WIDTH/2, TV_HEIGHT/2+1), cvScalar(255) ,2);
						cv::line(temp, Point(0, TV_HEIGHT/4), Point(TV_WIDTH/2, TV_HEIGHT/4), cvScalar(255) ,2);
						cv::line(temp, Point(0, 0), Point(TV_WIDTH/2, TV_HEIGHT/2+200), cvScalar(255) ,2);
					}
					cuConvert_yuv2bgr_yuyv_async(cap_chid, temp, img, CUT_FLAG_devAlloc);
					info->chId = cap_chid;
					info->channels = img.channels();
					info->width = img.cols;
					info->height = img.rows;
					info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000ul
							+ (uint64)capInfo.timestamp.tv_usec*1000ul;
					info->timestamp = (uint64_t)getTickCount();
					//OSA_printf("[%d] %s. ch%d %ld.%ld", OSA_getCurTimeInMsec(), __func__,
					//			cap_chid, capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
					//info->timestampEnter = getTickCount()/(getTickFrequency()*0.000000001);
					//if(info->timestampEnter - timestamp[cap_chid] > 60001000ul)
					//{
					//	OSA_printf("[%d]%s: capInterval %lu ns", OSA_getCurTimeInMsec(),__func__,
					//			info->timestampEnter - timestamp[cap_chid]);
					//}
					//timestamp[cap_chid] = info->timestampEnter;
					//OSA_printf("tm cap = %.6f %.6f", info->timestampCap/getTickFrequency(), info->timestamp/getTickFrequency());
					image_queue_putFull(imgQ[cap_chid], info);
				}
				else
				{
					OSA_printf("[%d]%s. ch%d %ld.%ld queue overflow!\n", OSA_getCurTimeInMsec(), __func__,cap_chid,
							capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
				}
			}
			//a ^=1;
		}
		else if(cap_chid==HOT_DEV_ID)
		{
			//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
			static int b = 0;
			if(!b)
			{
				OSA_BufInfo* info = image_queue_getEmpty(imgQ[cap_chid]);
				if(info != NULL)
				{
					img = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,info->physAddr);
					if(NOCAMERA)
					{
						Mat temp = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,src);
						cv::line(temp, Point(0, HOT_HEIGHT/2+1), Point(HOT_WIDTH/2, HOT_HEIGHT/2+1), cvScalar(255) ,2);
						cv::line(temp, Point(0, HOT_HEIGHT/4), Point(HOT_WIDTH/2, HOT_HEIGHT/4), cvScalar(255) ,2);
						cv::line(temp, Point(0, 0), Point(HOT_WIDTH/2, HOT_HEIGHT/2+200), cvScalar(255) ,2);
					}
					cudaMemcpy(info->physAddr, src, HOT_HEIGHT*HOT_WIDTH, cudaMemcpyHostToDevice);
					info->chId = cap_chid;
					info->channels = img.channels();
					info->width = img.cols;
					info->height = img.rows;
					info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000
							+ (uint64)capInfo.timestamp.tv_usec*1000;
					info->timestamp = (uint64_t)getTickCount();
					//OSA_printf("[%d] %s. ch%d %ld.%ld", OSA_getCurTimeInMsec(), __func__,
					//		cap_chid, capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
					image_queue_putFull(imgQ[cap_chid], info);
				}
				else
				{
					OSA_printf("[%d]%s. ch%d %ld.%ld queue overflow!\n", OSA_getCurTimeInMsec(), __func__,cap_chid,
							capInfo.timestamp.tv_sec, capInfo.timestamp.tv_usec);
				}
			}
			//b ^= 1;
		}
	}
}

static void keyboard_event(unsigned char key, int x, int y)
{
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
	case 'q':
	case 27:
		glutLeaveMainLoop();
		break;
	default:
		break;
	}
}

int main_cap(int argc, char **argv)
{
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
	dsInit.channelsSize[1].c = 1;
	render->init(&dsInit);
	render->run();

	imgQ[0] = &render->m_bufQue[0];
	imgQ[1] = &render->m_bufQue[1];

	ChosenCaptureGroup *grop[QUE_CHID_COUNT];
	grop[0] = ChosenCaptureGroup :: GetTVInstance();
	grop[1] = ChosenCaptureGroup::GetHOTInstance();

	//OSA_printf("getTickFrequency() = %f", getTickFrequency());

	//glutSetCursor(GLUT_CURSOR_NONE);
	glutMainLoop();

	render->stop();
	render->destroy();
	CRender::destroyObject(render);

	cuConvertUinit();

	return 0;
}
