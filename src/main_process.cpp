/*
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include "mmtdProcess.hpp"
#include "GeneralProcess.hpp"
#include "Displayer.hpp"
#include "ChosenCaptureGroup.h"
#include "encTrans.hpp"
#include "osa_image_queue.h"
#include "cuda_convert.cuh"

static CEncTrans enctran;
static CRender *render = NULL;
static IProcess *proc = NULL;
static CMMTDProcess *mmtd = NULL;
static CGeneralProc *general = NULL;

static int curChannelFlag = TV_DEV_ID;
static int curFovIdFlag[QUE_CHID_COUNT] = {0, 0};
static bool enableEnhFlag[QUE_CHID_COUNT] = {false,false};
static int ezoomxFlag[QUE_CHID_COUNT] = {1, 1};
static int colorYUVFlag = WHITECOLOR;

/************************************************************
 *
 *
 */
static int setEncTransLevel(int iLevel)
{
	ENCTRAN_encPrm encPrm;
	int iret = OSA_SOK;
	switch(iLevel)
	{
	case 0:
		encPrm.bitrate = BITRATE_2M;
		encPrm.maxQP = MAX_QP; encPrm.minQP = MIN_QP_2M;
		encPrm.maxQI = MAX_QP; encPrm.minQI = MIN_I_2M;
		encPrm.maxQB = -1; encPrm.minQB = -1;
		enctran.dynamic_config(CEncTrans::CFG_TransLevel, 0, NULL);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, TV_DEV_ID, &encPrm);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, HOT_DEV_ID, &encPrm);
		break;
	case 1:
		encPrm.bitrate = BITRATE_4M;
		encPrm.maxQP = MAX_QP; encPrm.minQP = MIN_QP_4M;
		encPrm.maxQI = MAX_QP; encPrm.minQI = MIN_I_4M;
		encPrm.maxQB = -1; encPrm.minQB = -1;
		enctran.dynamic_config(CEncTrans::CFG_TransLevel, 1, NULL);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, TV_DEV_ID, &encPrm);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, HOT_DEV_ID, &encPrm);
		break;
	case 2:
		encPrm.bitrate = BITRATE_8M;
		encPrm.maxQP = MAX_QP; encPrm.minQP = MIN_QP_8M;
		encPrm.maxQI = MAX_QP; encPrm.minQI = MIN_I_8M;
		encPrm.maxQB = -1; encPrm.minQB = -1;
		enctran.dynamic_config(CEncTrans::CFG_TransLevel, 2, NULL);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, TV_DEV_ID, &encPrm);
		enctran.dynamic_config(CEncTrans::CFG_EncPrm, HOT_DEV_ID, &encPrm);
		break;
	default:
		iret = OSA_EFAIL;
		break;
	}
	return iret;
}

static int setMainChId(int chId, int fovId, int ndrop, UTC_SIZE acqSize)
{
	VPCFG_MainChPrm mcPrm;
	int iret = OSA_SOK;
	if(chId<0||chId>=QUE_CHID_COUNT)
		return OSA_EFAIL;
	if(fovId<0||fovId>=MAX_NFOV_PER_CHAN)
		return OSA_EFAIL;
	mcPrm.fovId = fovId;
	mcPrm.iIntervalFrames = ndrop;
	if(render!= NULL)
		render->dynamic_config(CRender::DS_CFG_ChId, 0, &chId);
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, 0, &acqSize, sizeof(acqSize));
	proc->dynamic_config(CTrackerProc::VP_CFG_MainChId, chId, &mcPrm, sizeof(mcPrm));
	proc->dynamic_config(CMMTDProcess::VP_CFG_MainChId, chId, &mcPrm, sizeof(mcPrm));
	curChannelFlag = chId;
	curFovIdFlag[chId] = fovId;
	return iret;
}

static int enableTrack(bool enable, UTC_SIZE acqSize)
{
	int iret = OSA_SOK;
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, 0, &acqSize, sizeof(acqSize));
	proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, enable);
	return iret;
}

static int enableMMTD(bool enable, int nTarget)
{
	int iret = OSA_SOK;
	if(enable)
		proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDTargetCount, nTarget);
	proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDEnable, enable);
	return iret;
}

static int enableTrackByMMTD(int index)
{
	if(index<0 || index>=MAX_TGT_NUM)
		return OSA_EFAIL;

	if(!mmtd->m_target[index].valid)
		return OSA_EFAIL;

	UTC_RECT_float acqrc;
	acqrc.x = mmtd->m_target[index].Box.x;
	acqrc.y = mmtd->m_target[index].Box.y;
	acqrc.width = mmtd->m_target[index].Box.width;
	acqrc.height = mmtd->m_target[index].Box.height;

	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, true, &acqrc, sizeof(acqrc));
}

static int enableEnh(bool enable)
{
	enableEnhFlag[curChannelFlag] = enable;
	return OSA_SOK;
}

static int setEZoomx(int value)//1/2/4
{
	if(value != 1 && value != 2 && value != 4)
		return OSA_EFAIL;
	ezoomxFlag[curChannelFlag] = value;
	return OSA_SOK;
}

static int setOSDColor(int value)
{
	colorYUVFlag = value;
}


/************************************************************************
 *      process unit
 *
 */

static OSA_BufHndl *imgQ[QUE_CHID_COUNT] = {NULL,};
//static cv::Size imgSize[QUE_CHID_COUNT] = {Size(TV_WIDTH, TV_HEIGHT), Size(HOT_WIDTH, HOT_HEIGHT)};
static unsigned char *memsI420[QUE_CHID_COUNT] = {NULL,};
static cv::Mat imgOsd[QUE_CHID_COUNT];
static int enableRotest = 0;
static unsigned char *rotestMem = NULL;

static int init(bool bRender = false)
{
	int ret = OSA_SOK;
	cuConvertInit(QUE_CHID_COUNT);
	ret = cudaHostAlloc((void**)&memsI420[TV_DEV_ID], 1920*1280*3/2, cudaHostAllocDefault);
	ret = cudaHostAlloc((void**)&memsI420[HOT_DEV_ID], 1920*1280*3/2, cudaHostAllocDefault);
	unsigned char *mem = NULL;
	ret = cudaHostAlloc((void**)&mem, TV_WIDTH*TV_HEIGHT*4, cudaHostAllocDefault);
	imgOsd[TV_DEV_ID] = Mat(TV_HEIGHT, TV_WIDTH, CV_8UC1, mem);
	memset(imgOsd[TV_DEV_ID].data, 0, imgOsd[TV_DEV_ID].cols*imgOsd[TV_DEV_ID].rows*imgOsd[TV_DEV_ID].channels());
	ret = cudaHostAlloc((void**)&mem, HOT_WIDTH*HOT_HEIGHT*4, cudaHostAllocDefault);
	imgOsd[HOT_DEV_ID] = Mat(HOT_HEIGHT, HOT_WIDTH, CV_8UC1, mem);
	memset(imgOsd[HOT_DEV_ID].data, 0, imgOsd[HOT_DEV_ID].cols*imgOsd[HOT_DEV_ID].rows*imgOsd[HOT_DEV_ID].channels());

	ret = cudaHostAlloc((void**)&rotestMem, 1920*1280*2, cudaHostAllocDefault);
	memset(rotestMem, 0xff, 1920*1280*2);

	ENCTRAN_InitPrm enctranInit;
	memset(&enctranInit, 0, sizeof(enctranInit));
	enctranInit.nChannels = QUE_CHID_COUNT;
	enctranInit.iTransLevel = 1;
	enctranInit.defaultEnable[0] = true;
	enctranInit.defaultEnable[1] = true;
	enctranInit.imgSize[0] = cv::Size(TV_WIDTH, TV_HEIGHT);
	enctranInit.encPrm[0].fps = TV_FPS;
	enctranInit.encPrm[0].bitrate = BITRATE_4M;
	enctranInit.encPrm[0].minQP = MIN_QP_4M;
	enctranInit.encPrm[0].maxQP = MAX_QP;
	enctranInit.encPrm[0].minQI = MIN_I_4M;
	enctranInit.encPrm[0].maxQI = MAX_I;
	enctranInit.encPrm[0].minQB = -1;
	enctranInit.encPrm[0].maxQB = -1;
	enctranInit.imgSize[1] = cv::Size(HOT_WIDTH, HOT_HEIGHT);
	enctranInit.encPrm[1].fps = HOT_FPS;
	enctranInit.encPrm[1].bitrate = BITRATE_4M;
	enctranInit.encPrm[1].minQP = MIN_QP_4M;
	enctranInit.encPrm[1].maxQP = MAX_QP;
	enctranInit.encPrm[1].minQI = MIN_I_4M;
	enctranInit.encPrm[1].maxQI = MAX_I;
	enctranInit.encPrm[1].minQB = -1;
	enctranInit.encPrm[1].maxQB = -1;
	enctran.create();
	enctran.init(&enctranInit);
	enctran.run();

	if(bRender){
		DS_InitPrm dsInit;
		memset(&dsInit, 0, sizeof(dsInit));
		dsInit.bFullScreen = true;
		//dsInit.keyboardfunc = keyboardfunc;
		dsInit.nChannels = QUE_CHID_COUNT;
		dsInit.memType = memtype_cudev;
		dsInit.nQueueSize = 2;
		dsInit.disFPS = DIS_FPS;
		dsInit.channelsSize[0].w = TV_WIDTH;
		dsInit.channelsSize[0].h = TV_HEIGHT;
		dsInit.channelsSize[0].c = 3;
		dsInit.channelsSize[1].w = HOT_WIDTH;
		dsInit.channelsSize[1].h = HOT_HEIGHT;
		dsInit.channelsSize[1].c = 3;
		render = CRender::createObject();
		render->create();
		render->init(&dsInit);
		render->run();
		imgQ[0] = &render->m_bufQue[0];
		imgQ[1] = &render->m_bufQue[1];
	}

	mmtd = new CMMTDProcess();
	general = new CGeneralProc(NULL, mmtd);
	general->m_dc[TV_DEV_ID] = imgOsd[TV_DEV_ID];
	general->m_dc[HOT_DEV_ID] = imgOsd[HOT_DEV_ID];
	general->m_imgSize[TV_DEV_ID].width = TV_WIDTH;
	general->m_imgSize[TV_DEV_ID].height = TV_HEIGHT;
	general->m_imgSize[HOT_DEV_ID].width = HOT_WIDTH;
	general->m_imgSize[HOT_DEV_ID].height = HOT_HEIGHT;
	proc = general;
	general->creat();
	general->init();
	general->run();

	ChosenCaptureGroup *grop[2];
	grop[0] = ChosenCaptureGroup::GetTVInstance();
	grop[1] = ChosenCaptureGroup::GetHOTInstance();

	return ret;
}

static int uninit()
{
	general->stop();
	general->destroy();
	enctran.stop();
	enctran.destroy();
	if(render != NULL){
		render->stop();
		render->destroy();
		CRender::destroyObject(render);
	}
	delete general;
	delete mmtd;
	cudaFreeHost(imgOsd[TV_DEV_ID].data);
	cudaFreeHost(imgOsd[HOT_DEV_ID].data);
	cudaFreeHost(memsI420[TV_DEV_ID]);
	cudaFreeHost(memsI420[HOT_DEV_ID]);
	cudaFreeHost(rotestMem);
	cuConvertUinit();
}

void processFrame_process(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	Mat img;

	if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
		return;

	if(cap_chid == curChannelFlag && enableRotest){
		src = rotestMem; enableRotest = 0;
	}
	//struct timespec ns0;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns0);
	Mat i420;
	if(cap_chid==TV_DEV_ID)
	{
		img	= Mat(TV_HEIGHT,TV_WIDTH,CV_8UC2, src);
		i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, memsI420[cap_chid]);
		proc->process(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], img);
		if(enableEnhFlag[cap_chid])
			cuConvertEnh_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		else
			cuConvert_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		enctran.pushData(i420, cap_chid, V4L2_PIX_FMT_YUV420M);
	}
	else if(cap_chid==HOT_DEV_ID)
	{
		img = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,src);
		i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, memsI420[cap_chid]);
		proc->process(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], img);
		if(enableEnhFlag[cap_chid])
			cuConvertEnh_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		else
			cuConvert_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		enctran.pushData(i420, cap_chid, V4L2_PIX_FMT_YUV420M);
	}

	if(cap_chid == curChannelFlag && imgQ[cap_chid] != NULL)
	{
		Mat bgr;
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

	//struct timespec ns1;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns1);
	//printf("[%ld.%ld] ch%d timestamp %ld.%ld flags %08X\n", ns1.tv_sec, ns1.tv_nsec/1000000,
	//		cap_chid, ns0.tv_sec, ns0.tv_nsec/1000000, info.flags);
}

static void keyboard_event(unsigned char key, int x, int y)
{
	UTC_SIZE winSize;
	static int chId = TV_DEV_ID;
	static int fovId[QUE_CHID_COUNT] = {0,0};
	static bool mmtdEnable = false;
	static bool trkEnable = false;
	winSize.width = 80; winSize.height = 60;

	switch(key)
	{
	case 't':
		chId = TV_DEV_ID;
		fovId[chId] = (fovId[chId]<MAX_NFOV_PER_CHAN-1) ? (fovId[chId]+1) : 0;
		setMainChId(chId, fovId[chId], 0, winSize);
		break;
	case 'f':
		chId = HOT_DEV_ID;
		fovId[chId] = (fovId[chId]<MAX_NFOV_PER_CHAN-1) ? (fovId[chId]+1) : 0;
		if(chId != TV_DEV_ID){
			winSize.width *= HOT_WIDTH*1.f/TV_WIDTH;
			winSize.height *= HOT_HEIGHT*1.f/TV_HEIGHT;
		}
		setMainChId(chId, fovId[chId], 0, winSize);
		break;
	case 'u':
		static int speedLevel = 0;
		speedLevel++;
		if(speedLevel>2)
			speedLevel=0;
		setEncTransLevel(speedLevel);
		break;
	case 'a':
		trkEnable ^= 1;
		enableTrack(trkEnable, winSize);
		break;
	case 'b':
		mmtdEnable ^=1;
		enableMMTD(mmtdEnable, 5);
		break;
	case 'c':
		static bool enhEnable[QUE_CHID_COUNT] = {false, false};
		enhEnable[chId] ^= 1;
		enableEnh(enhEnable[chId]);
		break;
	case 'd':
		static int ezoomx[QUE_CHID_COUNT] = {1, 1};
		ezoomx[chId] = (ezoomx[chId] == 4) ? 1 : ezoomx[chId]<<1;
		setEZoomx(ezoomx[chId]);
		break;
	case 'e':
		static int colorTab[] = {WHITECOLOR,YELLOWCOLOR,CRAYCOLOR,GREENCOLOR,MAGENTACOLOR,REDCOLOR,BLUECOLOR,BLACKCOLOR};
		static int icolor = 0;
		icolor = (icolor<sizeof(colorTab)/sizeof(int)-1) ? (icolor+1) : 0;
		setOSDColor(colorTab[icolor]);
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		if(enableTrackByMMTD(key-'1')==OSA_SOK){
			trkEnable = true;
			mmtdEnable = false;
			enableMMTD(mmtdEnable, 0);
		}
		break;
	case 'r':
		enableRotest = 1;
		break;
	case 'q':
	case 27:
		glutLeaveMainLoop();
		break;
	default:
		break;
	}
}


/*********************************************************
 *
 * test main
 */
int main_process(int argc, char **argv)
{
	init(true);
	glutKeyboardFunc(keyboard_event);
	glutMainLoop();
	uninit();

	return 0;
}


