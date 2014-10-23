//Interface f

#pragma once

#include "stdafx.h"

#include "scope_base.h"

#include "Lucamapi.h"
#include "Lucamerr.h"

#include <cv.h>

#ifdef SCOPECONTROL_EXPORTS
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport) 
#endif


namespace sc
{
	class LumeneraCamera : public CameraBase
	{
	public:

		DLL_API LumeneraCamera();

		DLL_API bool connect(int cameraNum);
		DLL_API bool disconnect();
		DLL_API bool getFrame(cv::Mat& outImage);

	private:
		//Connection info
		bool _isConnected;
		HANDLE _hCam;
		LUCAM_FRAME_FORMAT _lffFormat;
		LUCAM_SNAPSHOT _lsSettings;
		LUCAM_CONVERSION _lcConversion;
		ULONG _cameraId;

		//Settings
		bool _enableFastFrames = false;
		float _videoFrameRate;

		//Frame info
		int _width;
		int _height;
	};
}