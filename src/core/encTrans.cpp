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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>
#include <opencv2/opencv.hpp>

//#include <gl.h>
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include <cuda.h>
#include <cuda_gl_interop.h>
#include "cuda_runtime_api.h"
#include "osa.h"
#include "StlGlDefines.h"
#include "gst_capture.h"
#include "cuda_convert.cuh"
#include "sync422_trans.h"
#include "encTrans.hpp"

//void processFrame_osd(int cap_chid,unsigned char *src)
void CEncTrans::process(Mat img, int chId, int pixFmt)
{
	OSA_assert(pixFmt == V4L2_PIX_FMT_YUV420M);
	//return CRender::display(img, chId, code);
	if(m_enable[chId]){
		gstCapturePushData(record_handle[chId],(char *)img.data,img.cols*img.rows*img.channels());
	}
	return ;
}

int CEncTrans::init(ENCTRAN_InitPrm *pPrm)
{
	int ret = 0;

	if(pPrm != NULL){
		memcpy(&m_initPrm, pPrm, sizeof(m_initPrm));
	}else{
		memset(&m_initPrm, 0, sizeof(m_initPrm));
		m_initPrm.defaultEnable[TV_DEV_ID] = true;
		m_initPrm.defaultEnable[HOT_DEV_ID] = true;
		m_initPrm.iTransLevel = 1;
		for(int i = 0; i<ENT_CHN_MAX; i++){
			m_initPrm.encPrm[i].bitrate = BITRATE_4M;
			m_initPrm.encPrm[i].minQP = MIN_QP_4M;
			m_initPrm.encPrm[i].maxQP = MAX_QP;
			m_initPrm.encPrm[i].minQI = MIN_I_4M;
			m_initPrm.encPrm[i].maxQI = MAX_I;
			m_initPrm.encPrm[i].minQB = -1;
			m_initPrm.encPrm[i].maxQB = -1;
		}
	}
	memcpy(m_enable, m_initPrm.defaultEnable, sizeof(m_enable));
	memcpy(m_encPrm, m_initPrm.encPrm, sizeof(m_encPrm));
	m_curTransLevel = m_initPrm.iTransLevel;
	m_curTransMask = (m_initPrm.defaultEnable[0] | (m_initPrm.defaultEnable[1]<<1));

	ret = sync422_ontime_ctrl(ctrl_prm_framerate, TV_DEV_ID, TV_FPS); OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_framerate, HOT_DEV_ID, HOT_FPS);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, m_curTransLevel);OSA_assert(ret == OSA_SOK);
	ret = sync422_ontime_ctrl(ctrl_prm_chlMask, 0, m_curTransMask);OSA_assert(ret == OSA_SOK);
	OSA_mutexLock(&m_mutex);
	for(int i=0; i<QUE_CHID_COUNT; i++){
		m_curBitrate[i]=ChangeBitRate(record_handle[i], m_encPrm[i].bitrate);
		ChangeQP_range(record_handle[i],
				m_encPrm[i].minQP, m_encPrm[i].maxQP,
				m_encPrm[i].minQI, m_encPrm[i].maxQI,
				m_encPrm[i].minQB, m_encPrm[i].maxQB);
	}
	OSA_mutexUnlock(&m_mutex);

	return ret;
}

void CEncTrans::run()
{

}
void CEncTrans::stop()
{

}

int CEncTrans::create()
{
	int ret = 0;

	OSA_mutexCreate(&m_mutex);
	ret = sync422_spi_create(0,0);// 0 0
	initGstCap();

	return ret;
}

int CEncTrans::destroy()
{
	int ret = 0;
	OSA_mutexLock(&m_mutex);
	UninitGstCap();
	sync422_spi_destory(0);

	OSA_mutexDelete(&m_mutex);

	return ret;
}

#define SPEED(r) ((r==GST_ENCBITRATE_2M) ? 0 : ((r==GST_ENCBITRATE_4M) ? 1: 2))
#define BITRATE(r) ((r==0) ? GST_ENCBITRATE_2M : ((r==1) ? GST_ENCBITRATE_4M: GST_ENCBITRATE_8M))
int CEncTrans::dynamic_config(CEncTrans::CFG type, int iPrm, void* pPrm)
{
	int ret = OSA_SOK;
	bool bEnable;
	if(type == CFG_Enable){
		if(iPrm >= QUE_CHID_COUNT || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		bEnable = *(bool*)pPrm;
		m_enable[iPrm] = bEnable;
	}
	if(type == CFG_EncPrm){
		if(iPrm >= QUE_CHID_COUNT || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(&m_encPrm[iPrm], pPrm, sizeof(ENCTRAN_encPrm));
		//OSA_mutexLock(&m_mutex);
		m_curBitrate[iPrm]=ChangeBitRate(record_handle[iPrm], m_encPrm[iPrm].bitrate);
		ChangeQP_range(record_handle[iPrm],
				m_encPrm[iPrm].minQP, m_encPrm[iPrm].maxQP,
				m_encPrm[iPrm].minQI, m_encPrm[iPrm].maxQI,
				m_encPrm[iPrm].minQB, m_encPrm[iPrm].maxQB);
		OSA_printf("%s %i: CH%d %dbps QP %d-%d QI %d-%d QB %d-%d", __func__, __LINE__,
				iPrm,m_encPrm[iPrm].bitrate,
				m_encPrm[iPrm].minQP, m_encPrm[iPrm].maxQP,
				m_encPrm[iPrm].minQI, m_encPrm[iPrm].maxQI,
				m_encPrm[iPrm].minQB, m_encPrm[iPrm].maxQB);
		//ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, SPEED(m_encPrm[iPrm].bitrate));
		//OSA_mutexUnlock(&m_mutex);
		OSA_assert(ret == OSA_SOK);
	}
	if(type == CFG_TransLevel){
		if(iPrm<0 || iPrm>2)
			return -1;
		ret = sync422_ontime_ctrl(ctrl_prm_uartrate, 0, iPrm);
		OSA_printf("%s %i: %d-%d", __func__, __LINE__, iPrm, BITRATE(iPrm));
		OSA_assert(ret == OSA_SOK);
	}
	if(type == CFG_keyFrame){
		if(iPrm >= QUE_CHID_COUNT || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		m_curBitrate[iPrm]=ChangeBitRate(record_handle[iPrm], m_curBitrate[iPrm]);
	}
	return 0;
}
#undef SPEED
#undef BITRATE

