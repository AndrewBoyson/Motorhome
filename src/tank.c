#include <stdint.h>
#include <stdio.h>     //fopen printf
#include <errno.h>     //errno
#include <string.h>    //strerror
#include <time.h>      //struct tm

#include "lib/settings.h"
#include "lib/log.h"
#include "lib/file.h"
#include "can-this.h"

void TankInit()
{
}

static int recordFresh(time_t t, int16_t temp, int16_t mV, int16_t supply)
{
	FILE* fp = fopen("fresh", "a");
	if (fp                                                         == NULL          ) { LogErrno("Record fresh - fopen: "   );             return 1; }
	
	struct tm tm;
	if (gmtime_r(&t, &tm)                                          == 0             ) { LogErrno("Record fresh - gmtime-r: "); fclose(fp); return 1; }
	char sTime[20];
	if (strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", &tm)    < 0             ) { LogErrno("Record fresh - strftime: "); fclose(fp); return 1; }
	
	if (fprintf(fp, "%s, %d, %d, %d\r\n", sTime, temp, mV, supply)  < 0             ) { LogErrno("Record fresh - fprintf: " ); fclose(fp); return 1; }
	
	if (fclose(fp)                                                                  ) { LogErrno("Record fresh - fclose: "  );             return 1; }
	return 0;
}
static int recordLpg(time_t t, int16_t resistance16ths)
{
	FILE* fp = fopen("lpg", "a");
	if (fp                                                         == NULL          ) { LogErrno("Record lpg - fopen: "   );             return 1; }
	
	struct tm tm;
	if (gmtime_r(&t, &tm)                                          == 0             ) { LogErrno("Record lpg - gmtime-r: "); fclose(fp); return 1; }
	char sTime[20];
	if (strftime(sTime, sizeof(sTime), "%Y-%m-%d %H:%M:%S", &tm)    < 0             ) { LogErrno("Record lpg - strftime: "); fclose(fp); return 1; }
	
	if (fprintf(fp, "%s, %d\r\n", sTime, resistance16ths)           < 0             ) { LogErrno("Record lpg - fprintf: " ); fclose(fp); return 1; }
	
	if (fclose(fp)                                                                  ) { LogErrno("Record lpg - fclose: "  );             return 1; }
	return 0;
}

static void plotFresh()
{
	#define SIGNIFICANT_MV_CHANGE 5
	#define SIGNIFICANT_TEMP_CHANGE 5
	#define SIGNIFICANT_SUPPLY_CHANGE 5
	int16_t mv = CanThisGetTankFreshMv();
	int16_t temp = CanThisGetTankFreshTemperature();
	int16_t supply = CanThisGetTankFreshSupplyMv();
	static int16_t lastMv = 0;
	static int16_t lastTemp = 0;
	static int16_t lastSupply = 0;
	time_t now = time(0);
	char significantChange = lastMv     > mv     + SIGNIFICANT_MV_CHANGE     || lastMv     < mv     - SIGNIFICANT_MV_CHANGE     ||
	                         lastTemp   > temp   + SIGNIFICANT_TEMP_CHANGE   || lastTemp   < temp   - SIGNIFICANT_TEMP_CHANGE   ||
							 lastSupply > supply + SIGNIFICANT_SUPPLY_CHANGE || lastSupply < supply - SIGNIFICANT_SUPPLY_CHANGE;
	if (significantChange)
	{
		recordFresh(now, temp, mv, supply);
		lastMv     = mv;
		lastTemp   = temp;
		lastSupply = supply;
	}
}
static void plotLpg()
{
	static int32_t totalResistance16ths = 0;
	static int16_t count = 0;
	
	totalResistance16ths += CanThisGetTankLpgResistance16ths();
	count++;
	if (count >= 256)
	{
		int16_t resistance16ths = totalResistance16ths / 256;
		static int16_t lastResistance16ths = 0;
		time_t now = time(0);
		char significantChange = resistance16ths > lastResistance16ths + 4 || resistance16ths < lastResistance16ths - 4;
		if (significantChange)
		{
			recordLpg(now, resistance16ths);
			lastResistance16ths = resistance16ths;
		}
		totalResistance16ths = 0;
		count = 0;
	}
}

void TankPoll(void)
{
	plotFresh();
	plotLpg();
}
