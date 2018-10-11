#include "mmtdProcess.hpp"

#define CONFIG_MMT_FILE		"ConfigMmtFile.yml"

CMMTDProcess::CMMTDProcess(IProcess *proc)
	:CProcessBase(proc),m_nCount(MAX_MMTD_TARGET_COUNT),m_curChId(0), m_bEnable(false),m_nDrop(0),
	 m_fovId(0), m_ezoomx(1), m_bHide(false)
{
	memset(m_target, 0, sizeof(m_target));
	memset(m_units, 0, sizeof(m_units));
	for(int chId=0; chId<MAX_CHAN; chId++){
		for(int i=0; i<MAX_TGT_NUM; i++){
			m_units[chId][i].thickness = 2;
			m_units[chId][i].lineType = 8;
		}
	}
	m_mmtd = new CMMTD();
	OSA_assert(m_mmtd != NULL);
	m_mmtd->SetTargetNum(m_nCount);
	ReadCfgMmtFromFile();
}

CMMTDProcess::~CMMTDProcess()
{
	if(m_mmtd != NULL)
		delete m_mmtd;
	m_mmtd = NULL;
}

inline Rect tRectCropScale(cv::Size imgSize, float fs)
{
	float fscaled = 1.0/fs;
	cv::Size2f rdsize(imgSize.width/2.0, imgSize.height/2.0);
	cv::Point point((int)(rdsize.width-rdsize.width*fscaled+0.5), (int)(rdsize.height-rdsize.height*fscaled+0.5));
	cv::Size sz((int)(imgSize.width*fscaled+0.5), (int)(imgSize.height*fscaled+0.5));
	return Rect(point, sz);
}

inline int meancr(unsigned char *img, int width)
{
	return ((img[0]+img[1]+img[width]+img[width+1])>>2);
}
static void mResize(Mat src, Mat dst, int zoomx)
{
	int width = src.cols;
	int height = src.rows;
	int zoomxStep = zoomx>>1;
	uint8_t  *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(src.data)+(height/2-(height/(zoomx<<1)))*width+(width/2-(width/(zoomx<<1)));
	pDst8_t = (uint8_t*)(dst.data);

	for(int y = 0; y < height; y++)
	{
		int halfIy = y>>zoomxStep;
		for(int x = 0; x < width; x++){
			pDst8_t[y*width+x] = meancr(pSrc8_t+halfIy*width+(x>>zoomxStep), width);
		}
	}
}

inline void mResizeX2(Mat src, Mat dst)
{
	int width = src.cols;
	int height = src.rows;
	uint8_t *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(src.data)+(height>>2)*width+(width>>2);
	pDst8_t = (uint8_t*)(dst.data);

	for(int y = 0; y < (height>>1); y++)
	{
		for(int x = 0; x < (width>>1); x++){
			pDst8_t[y*2*width+x*2] = pSrc8_t[y*width+x];
			pDst8_t[y*2*width+x*2+1] = (pSrc8_t[y*width+x]+pSrc8_t[y*width+x+1])>>1;
			pDst8_t[(y*2+1)*width+x*2] = (pSrc8_t[y*width+x]+pSrc8_t[(y+1)*width+x])>>1;
			pDst8_t[(y*2+1)*width+x*2+1] = (pSrc8_t[y*width+x]+pSrc8_t[y*width+x+1]+pSrc8_t[(y+1)*width+x]+pSrc8_t[(y+1)*width+x+1])>>2;
		}
	}
}

int CMMTDProcess::process(int chId, int fovId, int ezoomx, Mat frame)
{
	int iRet = CProcessBase::process(chId, fovId, ezoomx, frame);

	if(m_fovId!= fovId){
		m_mmtd->ClearAllMMTD();
		m_fovId= fovId;
		m_nDrop = 3;
	}

	if(m_nDrop>0){
		m_nDrop --;
		return iRet;
	}

	if(m_bEnable && m_curChId == chId)
	{
		Rect roi(0, 0, frame.cols, frame.rows);
		if(ezoomx>1){
			//cv::Size imgSize(frame.cols, frame.rows);
			//roi = tRectCropScale(imgSize, ezoomx);
		}
		if(m_ezoomx!=ezoomx){
			m_mmtd->ClearAllMMTD();
			m_ezoomx=ezoomx;
		}
		if(m_ezoomx<=1){
			m_mmtd->MMTDProcessRect(frame, m_target, roi, frame, 0);
		}else{
			//int64 tks = getTickCount();
			cv::Mat rsMat = cv::Mat(frame.rows, frame.cols, CV_8UC1);
			if(m_ezoomx == 4){
				cv::Mat rsMat0 = cv::Mat(frame.rows, frame.cols, CV_8UC1);
				mResizeX2(frame, rsMat0);
				mResizeX2(rsMat0, rsMat);
			}
			else if(m_ezoomx == 8){
				cv::Mat rsMat0 = cv::Mat(frame.rows, frame.cols, CV_8UC1);
				mResizeX2(frame, rsMat);
				mResizeX2(rsMat, rsMat0);
				mResizeX2(rsMat0, rsMat);
			}else{
				mResizeX2(frame, rsMat);
			}
			//OSA_printf("chId = %d, resize: time = %f sec \n", chId, ( (getTickCount() - tks)/getTickFrequency()) );
			m_mmtd->MMTDProcessRect(rsMat, m_target, roi, frame, 0);
		}
		for(int i; i<m_nCount; i++)
		{
			if(m_target[i].valid && !m_bHide){
				m_units[chId][i].bNeedDraw = true;
			}
		}
	}

	return iRet;
}

int CMMTDProcess::OnOSD(int chId, Mat dc)
{
	int ret = CProcessBase::OnOSD(chId, dc);
	float scalex = dc.cols/1920.0;
	float scaley = dc.rows/1080.0;
	int winWidth = 40*scalex;
	int winHeight = 40*scaley;
	bool bFixSize = false;

	for(int i=0; i<MAX_TGT_NUM; i++){
		if(m_units[chId][i].bHasDraw){
			rectangle(dc, m_units[chId][i].rc, cvScalar(0), m_units[chId][i].thickness, m_units[chId][i].lineType );
			putText(dc, m_units[chId][i].txt, m_units[chId][i].txtPos, CV_FONT_HERSHEY_COMPLEX, scalex,cvScalar(0));
			m_units[chId][i].bHasDraw = false;
		}
		if(m_units[chId][i].bNeedDraw){
			sprintf(m_units[chId][i].txt, "%d", i+1);
			m_units[chId][i].rcPos.x = m_target[i].Box.x + m_target[i].Box.width/2;
			m_units[chId][i].rcPos.y = m_target[i].Box.y + m_target[i].Box.height/2;
			if(!bFixSize){
				winWidth = m_target[i].Box.width;
				winHeight = m_target[i].Box.height;
			}
			m_units[chId][i].rc = Rect(m_units[chId][i].rcPos.x-winWidth/2, m_units[chId][i].rcPos.y-winHeight/2, winWidth, winHeight);
			rectangle(dc, m_units[chId][i].rc, cvScalar(255), m_units[chId][i].thickness, m_units[chId][i].lineType );
			m_units[chId][i].txtPos = Point(m_units[chId][i].rcPos.x+winWidth/2+2, m_units[chId][i].rcPos.y-winHeight/2-8);
			putText(dc, m_units[chId][i].txt, m_units[chId][i].txtPos, CV_FONT_HERSHEY_COMPLEX, scalex,cvScalar(255));
			m_units[chId][i].bHasDraw = true;
			m_units[chId][i].bNeedDraw = false;
		}
	}
}

int CMMTDProcess::dynamic_config(int type, int iPrm, void* pPrm, int prmSize)
{
	int iret = OSA_SOK;

	iret = CProcessBase::dynamic_config(type, iPrm, pPrm);

	if(type<VP_CFG_MMTD_BASE || type>VP_CFG_Max)
		return iret;

	//cout << "CMMTDProcess::dynamic_config type " << type << " iPrm " << iPrm << endl;
	switch(type)
	{
	case VP_CFG_MMTDTargetCount:
		m_nCount = iPrm;
		m_mmtd->SetTargetNum(m_nCount);
		break;
	case VP_CFG_MMTDEnable:
		memset(m_target, 0, sizeof(m_target));
		m_bEnable = iPrm;
		m_nDrop = 3;
		m_mmtd->ClearAllMMTD();
		break;
	case VP_CFG_MainChId:
		memset(m_target, 0, sizeof(m_target));
		m_curChId = iPrm;
		m_nDrop = 3;
		m_mmtd->ClearAllMMTD();
		break;
	default:
		iret = OSA_EFAIL;
		break;
	}

	return iret;
}

int CMMTDProcess::ReadCfgMmtFromFile()
{
	string cfgAvtFile;
	cfgAvtFile = CONFIG_MMT_FILE;

	char cfg_avt[16] = "cfg_mmt_";
	int configId_Max=128;
	float cfg_blk_val[128];

	FILE *fp = fopen(cfgAvtFile.c_str(), "rt");

	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);

		if(len < 10)
			return -1;
		else
		{
			FileStorage fr(cfgAvtFile, FileStorage::READ);
			if(fr.isOpened())
			{
				for(int i=0; i<configId_Max; i++){
					sprintf(cfg_avt, "cfg_mmt_%d", i);
					cfg_blk_val[i] = (float)fr[cfg_avt];
					//printf(" update cfg [%d] %f \n", i, cfg_blk_val[i]);
				}
			}else
				return -1;
		}


		int DetectGapparm;
		DetectGapparm = (int)cfg_blk_val[0];
		if((DetectGapparm<0) || (DetectGapparm>15))
			DetectGapparm = 10;
		m_mmtd->SetSRDetectGap(DetectGapparm);

		int MinArea, MaxArea;
		MinArea = (int)cfg_blk_val[1];
		MaxArea = (int)cfg_blk_val[2];
		m_mmtd->SetConRegMinMaxArea(MinArea, MaxArea);

		int stillPixel, movePixel;
		stillPixel = (int)cfg_blk_val[3];
		movePixel = (int)cfg_blk_val[4];
		m_mmtd->SetMoveThred(stillPixel, movePixel);

		float lapScaler;
		lapScaler = cfg_blk_val[5];
		m_mmtd->SetLapScaler(lapScaler);

		int lumThred;
		lumThred = (int)cfg_blk_val[6];
		m_mmtd->SetSRLumThred(lumThred);

		int srModel;
		srModel = (int)cfg_blk_val[7];
		m_mmtd->SetSRDetectMode(srModel);

		int meanThred1, stdThred, meanThred2;
		meanThred1 	= (int)cfg_blk_val[8];
		stdThred 		= (int)cfg_blk_val[9];
		meanThred2 	= (int)cfg_blk_val[10];
		m_mmtd->SetSRDetectParam(meanThred1,stdThred,meanThred2);

		int sortType;
		sortType 	= (int)cfg_blk_val[11];
		m_mmtd->SetTargetSortType(sortType);

		int bClimit;
		bClimit 	= (int)cfg_blk_val[12];
		m_mmtd->SetEnableClimitWH(bClimit);

		int SalientSize;
		SalientSize 	= (int)cfg_blk_val[13];
		m_mmtd->SetSalientSize(cv::Size(SalientSize,SalientSize));
		OSA_printf("DetectGapparm, MinArea, MaxArea,stillPixel, movePixel,lapScaler,lumThred,SalientSize = %d,%d,%d,%d,%d,%f,%d,%d\n",
					DetectGapparm, MinArea, MaxArea,stillPixel, movePixel,lapScaler,lumThred,SalientSize);

		return 0;

	}
	else
		return -1;
}
