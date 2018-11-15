#ifndef GLOBALDATA_H_
#define GLOBALDATA_H_

#include <iostream>

using namespace std;


#define ASSI_TIME 5000 //ms

typedef struct
{
	bool osd_enEnh;
	bool osd_enLaser;
	bool osd_enLA;
	bool osd_LMC;
	bool osd_AxisMode;
	bool osd_enZoom;
	bool osd_dispZoom;
	bool osd_dispFREnh;
	bool osd_dispTVEnh;
	bool osd_enSecondTrk;
	unsigned short osd_WorkMode;
	unsigned short osd_enDisp;
	unsigned short osd_laserStat;
	unsigned short osd_laserCode;
	unsigned short osd_TVFovAngle;
	unsigned short osd_FRFovAngle;
	unsigned short osd_Distance;
	unsigned short osd_panAngle;
	unsigned short osd_secondTrk_X;
	unsigned short osd_secondTrk_Y;	
	unsigned int osd_Time;
	int chId;
	int osd_tiltAngle;
}OSD_CMD;

class CGlobalData
{
private:
	CGlobalData();

	static CGlobalData *_Instance;
	
public:

	OSD_CMD osd_core;
	
	static pthread_mutex_t mtx;
	static CGlobalData* getInstance();
	
};


#endif /* GLOBALDATA_H_ */
