#ifndef  _GST_CAPTURE_H
#define _GST_CAPTURE_H

#include "StlGlDefines.h"

typedef int (*SendDataCallback)   (int dtype, unsigned char *buf, int size);

typedef enum
{
	FLIP_METHOD_NONE,
	FLIP_METHOD_COUNTERCLOCKWISE,
	FLIP_METHOD_ROTATE_180,
	FLIP_METHOD_CLOCKWISE,
	FLIP_METHOD_HORIZONTAL_FLIP,
	FLIP_METHOD_UPPER_RIGHT_DIAGONAL,
	FLIP_METHOD_VERTICAL_FLIP,
	FLIP_METHOD_UPPER_LEFT_DIAGONAL,
	FLIP_METHOD_END
}FLIP_METHOD;

typedef enum
{
	XIMAGESRC,
	APPSRC
}CAPTURE_SRC;

enum
{
	ENC_MIN_QP,
	ENC_MAX_QP,
	ENC_MIN_QI,
	ENC_MAX_QI,
	ENC_MIN_QB,
	ENC_MAX_QB,
	ENC_QP_PARAMS_COUNT
};

typedef struct _recordHandle
{
	int index;
	void* context;
	unsigned int width;
	unsigned int height;

	unsigned int framerate;
	unsigned int bitrate;
	char format[30];
	char ip_addr[30];
	unsigned int ip_port;
	unsigned short bEnable;
	FLIP_METHOD filp_method;
	CAPTURE_SRC capture_src;
	SendDataCallback sd_cb;
	int Q_PIB[ENC_QP_PARAMS_COUNT];
}RecordHandle;

typedef struct _gstCapture_data
{
	int width;
	int height;
	int framerate;
	int bitrate;
	unsigned int ip_port;
	FLIP_METHOD filp_method;
	CAPTURE_SRC capture_src;
	char* format;
	char* ip_addr;
	SendDataCallback sd_cb;
	int Q_PIB[ENC_QP_PARAMS_COUNT];
}GstCapture_data;

extern RecordHandle * record_handle[QUE_CHID_COUNT];

RecordHandle * gstCaptureInit( GstCapture_data gstCapture_data );

void initGstCap();
void UninitGstCap();
int gstCapturePushData(RecordHandle *handle, char *pbuffer , int datasize);

int gstCaptureUninit(RecordHandle *handle);

int gstCaptureEnable(RecordHandle *handle, unsigned short bEnable);

int ChangeBitRate(RecordHandle *handle,unsigned int bitrate);

int ChangeQP_range(RecordHandle *recordHandle,int minQP, int maxQP, int minQI, int maxQI, int minQB, int maxQB);

/*
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif
*/


#endif
