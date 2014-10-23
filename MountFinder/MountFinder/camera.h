//Interface f

#pragma once

#include "stdafx.h"

#include "camera_base.h"

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

		DLL_API LumeneraCamera(int cameraNum);

		DLL_API bool connect();
		DLL_API bool disconnect();
		DLL_API bool getFrame(cv::Mat& outImage);
		DLL_API bool is_connected();

		DLL_API void get_image_size(int& width, int& height);

		//Adds a callback function for raw preview images sent from the camera
		//The callback function should look like:
		//static void __stdcall PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused);
		// where pContext is the callbackObj (an object you want access to, usually going to be this)
		// pData is the preview data
		// datalength is the length of the data
		// unused is... unused! Surprising eh? It's required anyways, blame Lumenera corp.
		bool addNewFrameCallback(void* callbackFunc, void* callbackObj);

		//Instructs the camera to start sending a video stream
		//Requires that a callback function for new frames has been added! See above function
		bool startVideoStream();

		//Converts from an array of bytes provided by the camera to a cv Mat.
		void LumeneraCamera::byteDataToMat(BYTE* data, cv::Mat& outImage); //TODO: Move this crap out of lucam

	private:
		bool _getCamInfo();

		//Connection info
		//IF YOU ADD ANYTHING HERE REMEMBER TO ZERO IT OUT ON disconnect() 
		int _cameraNum;
		bool _isConnected;
		HANDLE _hCam;
		LUCAM_FRAME_FORMAT _lffFormat;
		LUCAM_SNAPSHOT _lsSettings;
		LUCAM_CONVERSION _lcConversion;
		ULONG _cameraId;

		//Settings
		bool _enableFastFrames = false;
		float _videoFrameRate;

		int _previewCallbackRegNumber = -1;

		//Frame info
		int _width;
		int _height;
	};
}