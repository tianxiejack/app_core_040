/*
 * main_core.cpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
 */
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include <opencv/cv.hpp>
#include <linux/videodev2.h>
#include <unistd.h>
#include "thread.h"
#include "crCore.hpp"
#include "ChosenCaptureGroup.h"

#define WHITECOLOR 		0x008080FF
#define YELLOWCOLOR 	0x009110D2
#define CRAYCOLOR		0x0010A6AA
#define GREENCOLOR		0x00223691
#define MAGENTACOLOR	0x00DECA6A
#define REDCOLOR		0x00F05A51
#define BLUECOLOR		0x006EF029
#define BLACKCOLOR		0x00808010
#define BLANKCOLOR		0x00000000

static ICore_1001 *core = NULL;

void processFrame_core(int cap_chid,unsigned char *src, struct v4l2_buffer capInfo, int format)
{
	if(core != NULL)
		core->processFrame(cap_chid, src, capInfo, format);
}

enum {
	KEYDIR_UP = 0, KEYDIR_DOWN, KEYDIR_LEFT, KEYDIR_RIGHT
};
void Track_armRefine(int dir, int step = 1)
{
	switch(dir)
	{
	case KEYDIR_UP:
		{
			cv::Point raf(0, -1*step);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_DOWN:
		{
			cv::Point raf(0, step);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_LEFT:
		{
			cv::Point raf(-1*step, 0);
			core->setTrackPosRef(raf);
		}
		break;
	case KEYDIR_RIGHT:
		{
			cv::Point raf(step, 0);
			core->setTrackPosRef(raf);
		}
		break;
	default:
		break;
	}
}

void Axis_move(int dir, int step = 1)
{
	cv::Point2f curPos = core->m_stats.chn[core->m_stats.mainChId].axis;
	switch(dir)
	{
	case KEYDIR_UP:
		{
			cv::Point pos(curPos.x+0.5, curPos.y-0.5-1*step);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_DOWN:
		{
			cv::Point pos(curPos.x+0.5, curPos.y+0.5+1*step);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_LEFT:
		{
			cv::Point pos(curPos.x-0.5-1*step, curPos.y+0.5);
			core->setAxisPos(pos);
		}
		break;
	case KEYDIR_RIGHT:
		{
			cv::Point pos(curPos.x+0.5+1*step, curPos.y+0.5);
			core->setAxisPos(pos);
		}
		break;
	default:
		break;
	}
}

static int iMenu = 0;
static void keyboard_event(unsigned char key, int x, int y)
{
	cv::Size winSize;
	static int chId = 0;
	static int fovId[2] = {0,0};
	static bool mmtdEnable = false;
	static bool trkEnable = false;

	winSize.width = 80; winSize.height = 60;

	char strMenus[2][1024] ={
			"----------------------------------------\n"
			"|---Main Menu -------------------------|\n"
			"----------------------------------------\n"
			" [t] Main Channel Choice TV And Fov++   \n"
			" [f] Main Channel Choice FLR And Fov++  \n"
			" [a] Enable/Disable Track               \n"
			" [b] Enable/Disable MMTD                \n"
			" [c] Enable/Disable Enh                 \n"
			" [d] Change EZoomx (X1/X2/X4)           \n"
			" [e] Enable/Disable OSD                 \n"
			" [g] Change OSD Color                   \n"
			" [h] Force Track To Coast               \n"
			" [i][k][j][l] Refine Track Pos          \n"
			" [m] Setup Axis                         \n"
			" [n] Start/Pause Encoder transfer       \n"
			" [u] Change EncTrans level (0/1/2)      \n"
			" [1].[5] Enable Track By MMTD           \n"
			" [esc][q]Quit                           \n"
			"--> ",

			"----------------------------------------\n"
			"|---Axis Menu -------------------------|\n"
			"----------------------------------------\n"
			" [t] Main Channel Choice TV And Fov++   \n"
			" [f] Main Channel Choice FLR And Fov++  \n"
			" [i] Move Up                            \n"
			" [k] Move Down                          \n"
			" [j] Move Left                          \n"
			" [l] Move Right                         \n"
			" [s] Save to file                       \n"
			" [esc][q]Back                           \n"
			"--> ",
	};

	switch(key)
	{
	case 't':
		if(chId == 0)
			fovId[chId] = (fovId[chId]<4-1) ? (fovId[chId]+1) : 0;
		chId = 0;
		core->setMainChId(chId, fovId[chId], 0, winSize);
		break;
	case 'f':
		if(chId == 1)
			fovId[chId] = (fovId[chId]<4-1) ? (fovId[chId]+1) : 0;
		chId = 1;
		winSize.width *= 1080.f/1920;
		winSize.height *= 1024.f/1080;
		core->setMainChId(chId, fovId[chId], 0, winSize);
		break;
	case 'u':
		static int speedLevel = 0;
		speedLevel++;
		if(speedLevel>2)
			speedLevel=0;
		core->setEncTransLevel(speedLevel);
		break;
	case 'a':
		trkEnable ^= 1;
		core->enableTrack(trkEnable, winSize, true);
		break;
	case 'b':
		mmtdEnable ^=1;
		core->enableMMTD(mmtdEnable, 5);
		break;
	case 'c':
		static bool enhEnable[2] = {false, false};
		enhEnable[chId] ^= 1;
		core->enableEnh(enhEnable[chId]);
		break;
	case 'd':
		static int ezoomx[2] = {1, 1};
		ezoomx[chId] = (ezoomx[chId] == 4) ? 1 : ezoomx[chId]<<1;
		core->setEZoomx(ezoomx[chId]);
		break;
	case 'e':
		static bool osdEnable = true;
		osdEnable ^= 1;
		core->enableOSD(osdEnable);
		break;
	case 'g':
		static int colorTab[] = {WHITECOLOR,YELLOWCOLOR,CRAYCOLOR,GREENCOLOR,MAGENTACOLOR,REDCOLOR,BLUECOLOR,BLACKCOLOR};
		static int icolor = 0;
		icolor = (icolor<sizeof(colorTab)/sizeof(int)-1) ? (icolor+1) : 0;
		core->setOSDColor(colorTab[icolor]);
		break;
	case 'h':
		static bool coastEnable = false;
		coastEnable ^= 1;
		core->setTrackCoast(((coastEnable) ? -1: 0));
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		if(chId == 1){
			winSize.width *= 1080.f/1920;
			winSize.height *= 1024.f/1080;
		}
		if(core->enableTrackByMMTD(key-'1', &winSize, false)==OSA_SOK){
			trkEnable = true;
			mmtdEnable = false;
			core->enableMMTD(mmtdEnable, 0);
		}
		break;
	case 'i'://move up
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_UP);
		if(iMenu == 1)
			Axis_move(KEYDIR_UP);
		break;
	case 'k'://move down
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_DOWN);
		if(iMenu == 1)
			Axis_move(KEYDIR_DOWN);
		break;
	case 'j'://move left
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_LEFT);
		if(iMenu == 1)
			Axis_move(KEYDIR_LEFT);
		break;
	case 'l'://move right
		if(iMenu == 0 && trkEnable)
			Track_armRefine(KEYDIR_RIGHT);
		if(iMenu == 1)
			Axis_move(KEYDIR_RIGHT);
		break;
	case 'm':
		printf("%s",strMenus[iMenu]);
		iMenu = 1;
		break;
	case 'n':
		static bool encEnable[2] = {true, true};
		encEnable[chId] ^=1;
		core->enableEncoder(chId, encEnable[chId]);
		break;
	case 's':
		if(iMenu == 1)
			core->saveAxisPos();
		break;
	case 'q':
	case 27:
		if(iMenu == 0)
			glutLeaveMainLoop();
		else
			iMenu = 0;
		break;
	default:
		printf("%s",strMenus[iMenu]);
		break;
	}
}


/*********************************************************
 *
 * test main
 */
static void *thrdhndl_glutloop(void *context)
{
	for(;;){
		int key = getc(stdin);
		if(key == 10)
			continue;
		if(key == 'q' || key == 'Q' || key == 27){
			if(iMenu == 0)
				break;
			else
				iMenu = 0;
		}
		keyboard_event(key, 0, 0);
	}
	if(*(bool*)context)
		glutLeaveMainLoop();
	//exit(0);
	return NULL;
}
static void *thrdhndl_keyevent(void *context)
{
	for(;;){
		int key = getchar();//getc(stdin);
		if(iMenu == 0 && (key == 'q' || key == 'Q' || key == 27)){
			break;
		}
		//OSA_printf("key = %d\n\r", key);
		keyboard_event(key, 0, 0);
	}
	if(*(bool*)context)
		glutLeaveMainLoop();
	//exit(0);
	return NULL;
}

static void *thrdhndl_timer( void * p )
{
	struct timeval timeout;
	char strTmp[64];
	char strFov[2][16];
	cv::Point posTmp;
	bool bHide = false;
	CORE1001_STATS stats;
	while( *(bool*)p )
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 100*1000;
		select( 0, NULL, NULL, NULL, &timeout );
		memcpy(&stats, &core->m_stats, sizeof(stats));
		struct tm curTmt;
		time_t curTm;
		time(&curTm);
		memcpy(&curTmt,localtime(&curTm),sizeof(curTmt));
		if(bHide){
			putText(core->m_dc[0], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[1], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[0], strFov[0], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			putText(core->m_dc[1], strFov[1], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(0),1);
			cv::circle(core->m_dc[0], posTmp, 16, cvScalar(0), 4);
		}
		sprintf(strTmp, "%04d-%02d-%02d %02d:%02d:%02d",
				curTmt.tm_year+1900, curTmt.tm_mon, curTmt.tm_mday,
				curTmt.tm_hour, curTmt.tm_hour, curTmt.tm_sec);
		sprintf(strFov[0], "FOV: %d", stats.chn[0].fovId);
		sprintf(strFov[1], "FOV: %d", stats.chn[1].fovId);
		putText(core->m_dc[0], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[1], strTmp, cv::Point(50,50), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[0], strFov[0], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		putText(core->m_dc[1], strFov[1], cv::Point(50,50*2), CV_FONT_HERSHEY_COMPLEX, 1.2,cvScalar(255),1);
		//posTmp = cv::Point(stats.chn[0].axis.x, stats.chn[0].axis.y);
		posTmp = cv::Point(stats.trackPos.x, stats.trackPos.y);
		cv::circle(core->m_dc[0], posTmp, 16, cvScalar(255), 2);
		bHide = true;
	}
	return NULL;
}

int main_core(int argc, char **argv)
{
	bool bRender = false;
	static bool bLoop = true;

	if(argc>=2){
		bRender = atoi(argv[1]);
	}
	core = (ICore_1001 *)ICore::Qury(COREID_1001);
	CORE1001_INIT_PARAM initParam;
	memset(&initParam, 0, sizeof(initParam));
	initParam.nChannels = QUE_CHID_COUNT;
	initParam.renderFPS = DIS_FPS;
	initParam.chnInfo[TV_DEV_ID].imgSize = cv::Size(TV_WIDTH, TV_HEIGHT);
	initParam.chnInfo[TV_DEV_ID].fps = TV_FPS;
	initParam.chnInfo[TV_DEV_ID].format = V4L2_PIX_FMT_YUYV;
	initParam.chnInfo[HOT_DEV_ID].imgSize = cv::Size(HOT_WIDTH, HOT_HEIGHT);
	initParam.chnInfo[HOT_DEV_ID].fps = HOT_FPS;
	initParam.chnInfo[HOT_DEV_ID].format = V4L2_PIX_FMT_GREY;
	initParam.bRender = bRender;
	initParam.bEncoder = true;
	core->init(&initParam, sizeof(initParam));
	start_thread(thrdhndl_timer, &bLoop);

	ChosenCaptureGroup *grop[2];
	grop[0] = ChosenCaptureGroup::GetTVInstance();
	grop[1] = ChosenCaptureGroup::GetHOTInstance();

	if(bRender){
		start_thread(thrdhndl_keyevent, &bRender);
		glutKeyboardFunc(keyboard_event);
		glutMainLoop();
	}else{
		thrdhndl_keyevent(&bRender);
	}
	bLoop = false;
	core->uninit();
	ICore::Release(core);
	core = NULL;

	return 0;
}


