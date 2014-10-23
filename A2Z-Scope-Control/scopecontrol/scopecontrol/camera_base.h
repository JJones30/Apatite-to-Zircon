//scope_base.h
//Defines a basic scope class, that all scopes should derive from
//Doesn't really do much

#pragma once

#include "stdafx.h"

#include <cv.h>

#pragma warning(disable: 4100)

namespace sc
{
	class CameraBase
	{
	public: //TODO: Make all these throw exceptions
		virtual bool getFrame(cv::Mat& outImage) { return false; };
		virtual bool connect() { return false; };
		virtual bool disconnect() { return false; };
		virtual bool is_connected() { return false; };

		virtual void get_image_size(int& width, int& height) { return; };

		virtual bool addNewFrameCallback(void* callbackFunc, void* callbackObj) { return false; };
		virtual bool startVideoStream() { return false; };

		//Converts from an array of bytes provided by the camera to a cv Mat.
		virtual void byteDataToMat(BYTE* data, cv::Mat& outImage) { return; };
	};
}
