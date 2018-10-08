#include "gst_capture.h"
#include"sync422_trans.h"

static char TARGET_IP[32] = "192.168.1.2";
GstCapture_data gstCapture_data[QUE_CHID_COUNT];
RecordHandle * record_handle[QUE_CHID_COUNT];

#define PORT_RTP 7000
static char strFormat[16] = "I420";//"YUY2"//"GRAY8"
void initGstCap()
{
	gstCapture_data[TV_DEV_ID].width = TV_WIDTH;
	gstCapture_data[TV_DEV_ID].height = TV_HEIGHT;
	gstCapture_data[TV_DEV_ID].framerate = 30;
	gstCapture_data[TV_DEV_ID].bitrate = 1500000;
	gstCapture_data[TV_DEV_ID].ip_port = PORT_RTP+TV_DEV_ID;
	gstCapture_data[TV_DEV_ID].filp_method = FLIP_METHOD_VERTICAL_FLIP;
	gstCapture_data[TV_DEV_ID].capture_src = APPSRC;
	//gstCapture_data[TV_DEV_ID].format = "YUY2";
	gstCapture_data[TV_DEV_ID].format = strFormat;//"I420";
	gstCapture_data[TV_DEV_ID].ip_addr =TARGET_IP;
	gstCapture_data[TV_DEV_ID].sd_cb=sync422_ontime_video;
	for(int i=0;i<ENC_QP_PARAMS_COUNT;i++)
		gstCapture_data[TV_DEV_ID].Q_PIB[i]=-1;

	record_handle[TV_DEV_ID] = gstCaptureInit(gstCapture_data[TV_DEV_ID]);


	gstCapture_data[HOT_DEV_ID].width = HOT_WIDTH;
	gstCapture_data[HOT_DEV_ID].height = HOT_HEIGHT;
	gstCapture_data[HOT_DEV_ID].framerate = 30;
	gstCapture_data[HOT_DEV_ID].bitrate = 1500000;
	gstCapture_data[HOT_DEV_ID].ip_port = PORT_RTP+HOT_DEV_ID;
	gstCapture_data[HOT_DEV_ID].filp_method = FLIP_METHOD_VERTICAL_FLIP;
	gstCapture_data[HOT_DEV_ID].capture_src = APPSRC;
	gstCapture_data[HOT_DEV_ID].format = strFormat;//"I420";
	//gstCapture_data[HOT_DEV_ID].format = "GRAY8";
	gstCapture_data[HOT_DEV_ID].ip_addr =TARGET_IP;
	gstCapture_data[HOT_DEV_ID].sd_cb=sync422_ontime_video;

	for(int i=0;i<ENC_QP_PARAMS_COUNT;i++)
		gstCapture_data[HOT_DEV_ID].Q_PIB[i]=-1;

	record_handle[HOT_DEV_ID] = gstCaptureInit(gstCapture_data[HOT_DEV_ID]);

}
void UninitGstCap()
{
   gstCaptureUninit(record_handle[TV_DEV_ID]);
    gstCaptureUninit(record_handle[HOT_DEV_ID]);
}

