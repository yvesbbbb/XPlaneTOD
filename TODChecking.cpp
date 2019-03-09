// Downloaded from https://developer.x-plane.com/code-sample/timedprocessing/

/*
 * TimedProcessing.c
 *
 * This example plugin demonstrates how to use the timed processing callbacks
 * to continuously record sim data to disk.
 *
 * This technique can be used to record data to disk or to the network.  Unlike
 * UDP data output, we can increase our frequency to capture data every single
 * sim frame.  (This example records once per second.)
 *
 * Use the timed processing APIs to do any periodic or asynchronous action in
 * your plugin.
 *
 *****    8 mars 2019
 *****    modified to pause the airplane 10NM before the TOD calculated by the FMS
 *****    TimedProcessing.cpp renamed to TODchecking.cpp
 *
 */

#if APL
#if defined(__MACH__)
#include <Carbon/Carbon.h>
#endif
#endif

#include <string>
#include <ctime> 
#include <stdio.h>
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"


/* File to write data to. */
static FILE *	gOutputFile; 

/* Data ref we will record. */
static XPLMDataRef	gDataRefTOD = NULL;

/* distance to check in NM before TOD */
static int NM_beforeTOD = 10;

/* Pause command */
static XPLMCommandKeyID CMD_Pause = xplm_key_pause;

/* Time for log */
static time_t current_time;
static struct tm * ti;


#if APL && __MACH__
static int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen);
#endif


static float	MyFlightLoopCallback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void *               inRefcon);


static void TimeStamp();


PLUGIN_API int XPluginStart(
	char *		outName,
	char *		outSig,
	char *		outDesc)
{
	char	outputPath[255];

#if APL && __MACH__
	char outputPath2[255];
	int Result = 0;
#endif

	strcpy(outName, "TOD Checking");
	strcpy(outSig, "xplanesdk.examples.TimedProcessing.TODchecking");
	strcpy(outDesc, "A plugin to pause airplane 10NM before TOD.");

	/* Open a file to write to.  We locate the X-System directory
	 * and then concatenate our file name.  This makes us save in
	 * the X-System directory.  Open the file. */
	XPLMGetSystemPath(outputPath);
	strcat(outputPath, "TODchecking.txt");

#if APL && __MACH__
	Result = ConvertPath(outputPath, outputPath2, sizeof(outputPath));
	if (Result == 0)
		strcpy(outputPath, outputPath2);
	else
		XPLMDebugString("TimedProccessing - Unable to convert path\n");
#endif

	gOutputFile = fopen(outputPath, "w");

	/* Find the data ref we want to record (TOD). */
	gDataRefTOD = XPLMFindDataRef("sim/cockpit2/radios/indicators/fms_distance_to_tod_pilot");

	TimeStamp();

	/* Register our callback for once a second.  Positive intervals
	 * are in seconds, negative are the negative of sim frames.  Zero
	 * registers but does not schedule a callback for time. */
	XPLMRegisterFlightLoopCallback(
		MyFlightLoopCallback,	/* Callback */
		1.0,					/* Interval */
		NULL);					/* refcon not used. */

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	/* Unregister the callback */
	XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback, NULL);

	/* Close the file */
	fclose(gOutputFile);
}

PLUGIN_API void XPluginDisable(void)
{
	/* Stamp time */
	TimeStamp();
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(
	XPLMPluginID	inFromWho,
	int				inMessage,
	void *			inParam)
{
}

float	MyFlightLoopCallback(
	float                inElapsedSinceLastCall,
	float                inElapsedTimeSinceLastFlightLoop,
	int                  inCounter,
	void *               inRefcon)
{
	/* The actual callback.  First we read the sim's elapsed time and the TOD. */

	int elapsed = (int)XPLMGetElapsedTime();
	int tod = (int)XPLMGetDataf(gDataRefTOD);

	if (elapsed % 300 == 0)   // on every 5 minutes, stamp TOD data
	{
		fprintf(gOutputFile, "%i | %i | %i\n", elapsed, tod, NM_beforeTOD);
		fflush(gOutputFile);
	}

	// compare tod and NM_beforeTOD
	if (tod == NM_beforeTOD) {

		XPLMCommandKeyStroke(CMD_Pause);  // pause at the NM_beforeTOD
		NM_beforeTOD++;  // kind of reset to continue after the keyboard press <P> (UNPAUSED)
		// you also can disable the plugin with the Menu : Plugin Admin

	}

	/* Return 1.0 to indicate that we want to be called again in 1 second. */
	return 1.0;
}

void TimeStamp() {

	time(&current_time);
	ti = localtime(&current_time);
	fprintf(gOutputFile, "\nCurrent time : %s\n", asctime(ti));
	fflush(gOutputFile);   // flush to disk

}

#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
	CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath, kCFStringEncodingMacRoman);
	if (inStr == NULL)
		return -1;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle, 0);
	CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
		return -1;
	CFRelease(outStr);
	CFRelease(url);
	CFRelease(inStr);
	return 0;
}
#endif
