
#ifndef ENCTRANS_HPP_
#define ENCTRANS_HPP_

#include "osa.h"
#include <osa_sem.h>
#include <cuda.h>
#include "cuda_runtime_api.h"

#include "osa.h"
#include "osa_thr.h"
#include "osa_buf.h"
#include "osa_sem.h"
#include "linux/videodev2.h"
//#include "StlGlDefines.h"

using namespace std;
using namespace cv;

#define ENT_CHN_MAX		(4)

typedef struct _enctran_enc_param
{
	unsigned int bitrate;
	int minQP;
	int maxQP;
	int minQI;
	int maxQI;
	int minQB;
	int maxQB;
}ENCTRAN_encPrm;
typedef struct _enctran_init_param
{
	int iTransLevel;
	bool defaultEnable[ENT_CHN_MAX];
	ENCTRAN_encPrm encPrm[ENT_CHN_MAX];
}ENCTRAN_InitPrm;

class CEncTrans
{
public:
	CEncTrans(){
		memset(m_enable, 0, sizeof(m_enable));
		memset(m_semScheduler, 0, sizeof(m_semScheduler));
		memset(m_bufQue, 0, sizeof(m_bufQue));
		memset(m_bufSem, 0, sizeof(m_bufSem));
	};
	~CEncTrans(){};
	int create();
	int destroy();
	int init(ENCTRAN_InitPrm *pPrm);
	void run();
	void stop();

	typedef enum{
		CFG_Enable = 0,
		CFG_TransLevel,
		CFG_EncPrm,
		CFG_keyFrame,
		CFG_Max
	}CFG;

	int dynamic_config(CEncTrans::CFG type, int iPrm, void* pPrm);
	void pushData(Mat frame, int chId, int pixFmt = V4L2_PIX_FMT_YUV420M);
	void scheduler(int chId);

public:
	ENCTRAN_InitPrm m_initPrm;
	bool m_enable[ENT_CHN_MAX];
	ENCTRAN_encPrm m_encPrm[ENT_CHN_MAX];
	int m_curBitrate[ENT_CHN_MAX];
	OSA_BufHndl *m_bufQue[ENT_CHN_MAX];
	OSA_SemHndl *m_bufSem[ENT_CHN_MAX];
	int m_curTransLevel;
	int m_curTransMask;

protected:
	OSA_MutexHndl m_mutex;
	OSA_SemHndl *m_semScheduler[ENT_CHN_MAX];

private:
};

#endif /* DISPLAYER_HPP_ */
