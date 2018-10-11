/*
 * crCore.cpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
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
#include "thread.h"
#include "crCore.hpp"


namespace cr_local
{
static CEncTrans enctran;
static CRender *render = NULL;
static IProcess *proc = NULL;
static CMMTDProcess *mmtd = NULL;
static CGeneralProc *general = NULL;

static int curChannelFlag = TV_DEV_ID;
static int curFovIdFlag[QUE_CHID_COUNT] = {0, 0};
static bool enableTrackFlag = false;
static bool enableMMTDFlag = false;
static bool enableEnhFlag[QUE_CHID_COUNT] = {false,false};
static bool enableOSDFlag = true;
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

static int enableTrack(bool enable, UTC_SIZE winSize, bool bFixSize)
{
	int iret = OSA_SOK;
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize, &winSize, sizeof(winSize));
	proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, enable);
	enableTrackFlag = enable;
	return iret;
}

static int enableTrack(bool enable, UTC_RECT_float winRect, bool bFixSize)
{
	int iret = OSA_SOK;
	proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, enable, &winRect, sizeof(winRect));
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize);
	enableTrackFlag = enable;
	return iret;
}

static int enableMMTD(bool enable, int nTarget)
{
	int iret = OSA_SOK;
	if(enable)
		proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDTargetCount, nTarget);
	proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDEnable, enable);
	enableMMTDFlag = enable;
	return iret;
}

static int enableTrackByMMTD(int index, cv::Size *winSize, bool bFixSize)
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

	if(winSize != NULL){
		acqrc.width = winSize->width;
		acqrc.height = winSize->height;
	}

	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize);
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, true, &acqrc, sizeof(acqrc));
}

static int enableEnh(bool enable)
{
	enableEnhFlag[curChannelFlag] = enable;
	return OSA_SOK;
}

static int enableOSD(bool enable)
{
	enableOSDFlag = enable;
	return OSA_SOK;
}

static int setAxisPos(cv::Point pos)
{
	cv::Point2f setPos(pos.x, pos.y);
	int ret = proc->dynamic_config(CTrackerProc::VP_CFG_Axis, 0, &setPos, sizeof(setPos));
	OSA_assert(ret == OSA_SOK);
	ret = proc->dynamic_config(CTrackerProc::VP_CFG_SaveAxisToArray, 0);
	return ret;
}

static int saveAxisPos()
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_SaveAxisToFile, 0);
}

static int setEZoomx(int value)//1/2/4
{
	if(value != 1 && value != 2 && value != 4)
		return OSA_EFAIL;
	ezoomxFlag[curChannelFlag] = value;
	return OSA_SOK;
}

static int setTrackCoast(int nFrames)
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkCoast, nFrames);
}

static int setTrackPosRef(cv::Point2f ref)
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkPosRef, 0, &ref, sizeof(ref));
}

static int setOSDColor(int value)
{
	colorYUVFlag = value;
}


/************************************************************************
 *      process unit
 *
 */

static OSA_BufHndl *imgQRender[QUE_CHID_COUNT] = {NULL,};
static OSA_BufHndl *imgQEnc[QUE_CHID_COUNT] = {NULL,};
static OSA_SemHndl *imgQEncSem[QUE_CHID_COUNT] = {NULL,};
static unsigned char *memsI420[QUE_CHID_COUNT] = {NULL,};
static cv::Mat imgOsd[QUE_CHID_COUNT];

static int init(OSA_SemHndl *notify = NULL, bool bEncoder = false, bool bRender = false, bool bHideOSD = false)
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

	if(bEncoder)
	{
		ENCTRAN_InitPrm enctranInit;
		memset(&enctranInit, 0, sizeof(enctranInit));
		enctranInit.iTransLevel = 1;
		enctranInit.defaultEnable[0] = true;
		enctranInit.defaultEnable[1] = true;
		enctranInit.encPrm[0].bitrate = BITRATE_4M;
		enctranInit.encPrm[0].minQP = MIN_QP_4M;
		enctranInit.encPrm[0].maxQP = MAX_QP;
		enctranInit.encPrm[0].minQI = MIN_I_4M;
		enctranInit.encPrm[0].maxQI = MAX_I;
		enctranInit.encPrm[0].minQB = -1;
		enctranInit.encPrm[0].maxQB = -1;
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
		imgQEnc[0] = enctran.m_bufQue[0];
		imgQEnc[1] = enctran.m_bufQue[1];
		imgQEncSem[0] = enctran.m_bufSem[0];
		imgQEncSem[1] = enctran.m_bufSem[1];
	}

	if(bRender)
	{
		DS_InitPrm dsInit;
		memset(&dsInit, 0, sizeof(dsInit));
		dsInit.bFullScreen = true;
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
		imgQRender[0] = &render->m_bufQue[0];
		imgQRender[1] = &render->m_bufQue[1];
	}

	mmtd = new CMMTDProcess();
	mmtd->m_bHide = bHideOSD;
	general = new CGeneralProc(notify,mmtd);
	general->m_dc[TV_DEV_ID] = imgOsd[TV_DEV_ID];
	general->m_dc[HOT_DEV_ID] = imgOsd[HOT_DEV_ID];
	general->m_imgSize[TV_DEV_ID].width = TV_WIDTH;
	general->m_imgSize[TV_DEV_ID].height = TV_HEIGHT;
	general->m_imgSize[HOT_DEV_ID].width = HOT_WIDTH;
	general->m_imgSize[HOT_DEV_ID].height = HOT_HEIGHT;
	general->m_bHide = bHideOSD;
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
	cuConvertUinit();
}
#define ZeroCpy	(0)
static void processFrame(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	Mat img;

	if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
		return;
	//struct timespec ns0;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns0);
	Mat i420;
	OSA_BufInfo* info = NULL;
	unsigned char *mem = NULL;

	if(ZeroCpy){
		if(imgQEnc[cap_chid] != NULL)
			info = image_queue_getEmpty(imgQEnc[cap_chid]);
	}
	if(info != NULL){
		info->chId = cap_chid;
		info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000ul
				+ (uint64)capInfo.timestamp.tv_usec*1000ul;
		info->timestamp = (uint64_t)getTickCount();
		mem = (unsigned char *)info->virtAddr;
	}
	if(mem == NULL)
		mem = memsI420[cap_chid];
	if(cap_chid==TV_DEV_ID)
	{
		//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
		enctran.scheduler(cap_chid);
		img	= Mat(TV_HEIGHT,TV_WIDTH,CV_8UC2, src);
		i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, mem);
		proc->process(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], img);
		if(enableOSDFlag){
			if(enableEnhFlag[cap_chid])
				cuConvertEnh_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
			else
				cuConvert_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		}else{
			if(enableEnhFlag[cap_chid])
				cuConvertEnh_async(cap_chid, img, i420, ezoomxFlag[cap_chid]);
			else
				cuConvert_async(cap_chid, img, i420, ezoomxFlag[cap_chid]);
		}
		if(!ZeroCpy)
			enctran.pushData(i420, cap_chid, V4L2_PIX_FMT_YUV420M);
	}
	else if(cap_chid==HOT_DEV_ID)
	{
		//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
		enctran.scheduler(cap_chid);
		img = Mat(HOT_HEIGHT,HOT_WIDTH,CV_8UC1,src);
		i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, mem);
		proc->process(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], img);
		if(enableOSDFlag){
			if(enableEnhFlag[cap_chid])
				cuConvertEnh_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
			else
				cuConvert_async(cap_chid, img, imgOsd[cap_chid], i420, ezoomxFlag[cap_chid], colorYUVFlag);
		}else{
			if(enableEnhFlag[cap_chid])
				cuConvertEnh_async(cap_chid, img, i420, ezoomxFlag[cap_chid]);
			else
				cuConvert_async(cap_chid, img, i420, ezoomxFlag[cap_chid]);
		}
		if(!ZeroCpy)
			enctran.pushData(i420, cap_chid, V4L2_PIX_FMT_YUV420M);
	}

	if(info != NULL){
		info->channels = i420.channels();
		info->width = i420.cols;
		info->height = i420.rows;
		info->format = V4L2_PIX_FMT_YUV420M;
		image_queue_putFull(imgQEnc[cap_chid], info);
		OSA_semSignal(imgQEncSem[cap_chid]);
	}

	if(cap_chid == curChannelFlag && imgQRender[cap_chid] != NULL)
	{
		Mat bgr;
		info = image_queue_getEmpty(imgQRender[cap_chid]);
		if(info != NULL)
		{
			bgr = Mat(img.rows,img.cols,CV_8UC3, info->physAddr);
			cuConvertConn_yuv2bgr_i420(cap_chid, bgr, CUT_FLAG_devAlloc);
			info->chId = cap_chid;
			info->channels = bgr.channels();
			info->width = bgr.cols;
			info->height = bgr.rows;
			info->format = V4L2_PIX_FMT_BGR24;
			info->timestampCap = (uint64)capInfo.timestamp.tv_sec*1000000000ul
					+ (uint64)capInfo.timestamp.tv_usec*1000ul;
			info->timestamp = (uint64_t)getTickCount();
			image_queue_putFull(imgQRender[cap_chid], info);
		}
	}

	//struct timespec ns1;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns1);
	//printf("[%ld.%ld] ch%d timestamp %ld.%ld flags %08X\n", ns1.tv_sec, ns1.tv_nsec/1000000,
	//		cap_chid, ns0.tv_sec, ns0.tv_nsec/1000000, info.flags);
}

};//namespace cr_local

class Core_1001 : public ICore_1001
{
	Core_1001():m_notifySem(NULL),m_bRun(false){memset(&m_stats, 0, sizeof(m_stats));};
	virtual ~Core_1001(){uninit();};
	friend ICore* ICore::Qury(int coreID);
	OSA_SemHndl m_updateSem;
	OSA_SemHndl *m_notifySem;
	bool m_bRun;
	void update();
	static void *thrdhndl_update(void *context)
	{
		Core_1001 *core = (Core_1001 *)context;
		while(core->m_bRun){
			OSA_semWait(&core->m_updateSem, OSA_TIMEOUT_FOREVER);
			if(!core->m_bRun)
				break;
			core->update();
			if(core->m_notifySem != NULL)
				OSA_semSignal(core->m_notifySem);
		}
		return NULL;
	}
public:
	static unsigned int ID;
	virtual int init(void *pParam, int paramSize)
	{
		OSA_assert(sizeof(CORE1001_INIT_PARAM) == paramSize);
		CORE1001_INIT_PARAM *initParam = (CORE1001_INIT_PARAM*)pParam;
		m_notifySem = initParam->notify;
		OSA_semCreate(&m_updateSem, 1, 0);
		int ret = cr_local::init(&m_updateSem, initParam->bEncoder, initParam->bRender, initParam->bHideOSD);
		memset(&m_stats, 0, sizeof(m_stats));
		update();
		for(int chId=0; chId<QUE_CHID_COUNT; chId++)
			m_dc[chId] = cr_local::imgOsd[chId];
		m_bRun = true;
		start_thread(thrdhndl_update, this);
		return ret;
	}
	virtual int uninit()
	{
		m_bRun = false;
		OSA_semSignal(&m_updateSem);
		int ret = cr_local::uninit();
		memset(&m_stats, 0, sizeof(m_stats));
		OSA_semDelete(&m_updateSem);
		return ret;
	}
	virtual void processFrame(int chId, unsigned char *data, struct v4l2_buffer capInfo, int format)
	{
		cr_local::processFrame(chId, data, capInfo, format);
	}

	virtual int setMainChId(int chId, int fovId, int ndrop, cv::Size acqSize)
	{
		UTC_SIZE sz;
		sz.width = acqSize.width; sz.height = acqSize.height;
		int ret = cr_local::setMainChId(chId, fovId, ndrop, sz);
		update();
		return ret;
	}
	virtual int enableTrack(bool enable, cv::Size winSize, bool bFixSize)
	{
		UTC_SIZE sz;
		sz.width = winSize.width; sz.height = winSize.height;
		int ret = cr_local::enableTrack(enable, sz, bFixSize);
		update();
		return ret;
	}
	virtual int enableTrack(bool enable, Rect2f winRect, bool bFixSize)
	{
		UTC_RECT_float rc;
		rc.x = winRect.x; rc.y = winRect.y;
		rc.width = winRect.width; rc.height = winRect.height;
		int ret = cr_local::enableTrack(enable, rc, bFixSize);
		update();
		return ret;
	}
	virtual int enableMMTD(bool enable, int nTarget)
	{
		int ret = cr_local::enableMMTD(enable, nTarget);
		update();
		return ret;
	}
	virtual int enableTrackByMMTD(int index, cv::Size *winSize, bool bFixSize)
	{
		int ret = cr_local::enableTrackByMMTD(index, winSize, bFixSize);
		update();
		return ret;
	}
	virtual int enableEnh(bool enable)
	{
		int ret = cr_local::enableEnh(enable);
		update();
		return ret;
	}
	virtual int enableOSD(bool enable)
	{
		int ret = cr_local::enableOSD(enable);
		update();
		return ret;
	}
	virtual int setAxisPos(cv::Point pos)
	{
		int ret = cr_local::setAxisPos(pos);
		update();
		return ret;
	}
	virtual int saveAxisPos()
	{
		int ret = cr_local::saveAxisPos();
		update();
		return ret;
	}
	virtual int setTrackPosRef(cv::Point2f ref)
	{
		int ret = cr_local::setTrackPosRef(ref);
		update();
		return ret;
	}
	virtual int setTrackCoast(int nFrames)
	{
		int ret = cr_local::setTrackCoast(nFrames);
		update();
		return ret;
	}
	virtual int setEZoomx(int value)
	{
		int ret = cr_local::setEZoomx(value);
		update();
		return ret;
	}
	virtual int setOSDColor(int yuv)
	{
		int ret = cr_local::setOSDColor(yuv);
		update();
		return ret;
	}
	virtual int setEncTransLevel(int iLevel)
	{
		int ret = cr_local::setEncTransLevel(iLevel);
		update();
		return ret;
	}
};
unsigned int Core_1001::ID = COREID_1001;
void Core_1001::update()
{
	//OSA_printf("%s %d: enter.", __func__, __LINE__);
	m_stats.mainChId = cr_local::curChannelFlag;
	m_stats.acqWinSize.width = cr_local::general->m_sizeAcqWin.width;
	m_stats.acqWinSize.width = cr_local::general->m_sizeAcqWin.width;
	m_stats.enableTrack = cr_local::enableTrackFlag;
	m_stats.enableMMTD = cr_local::enableMMTDFlag;
	m_stats.iTrackorStat = cr_local::general->m_iTrackStat;
	UTC_RECT_float curRC = cr_local::general->m_rcTrk;
	m_stats.trackPos.x = curRC.x+curRC.width/2;
	m_stats.trackPos.y = curRC.y+curRC.height/2;
	m_stats.trackWinSize.width = curRC.width;
	m_stats.trackWinSize.height = curRC.height;
	for(int i=0; i<CORE_TGT_NUM_MAX; i++){
		m_stats.tgts[i].valid = cr_local::mmtd->m_target[i].valid;
		m_stats.tgts[i].Box = cr_local::mmtd->m_target[i].Box;
	}

	for(int chId = 0; chId < QUE_CHID_COUNT; chId++){
		CORE1001_CHN_STATS *chn = &m_stats.chn[chId];
		chn->imgSize = cr_local::general->m_imgSize[chId];
		chn->fovId = cr_local::curFovIdFlag[chId];
		chn->axis.x = cr_local::general->m_AxisCalibX[chId][chn->fovId];
		chn->axis.y = cr_local::general->m_AxisCalibY[chId][chn->fovId];
		chn->enableEnh = cr_local::enableEnhFlag[chId];
		chn->iEZoomx = cr_local::ezoomxFlag[chId];
		chn->enableEncTrans = cr_local::enctran.m_enable[chId];
	}
}

ICore* ICore::Qury(int coreID)
{
	ICore *core = NULL;
	if(Core_1001::ID == coreID){
		core = new Core_1001;
	}

	return core;
}
void ICore::Release(ICore* core)
{
	if(core != NULL)
		delete core;
}


