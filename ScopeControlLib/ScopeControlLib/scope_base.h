//scope_base.h
//Defines a basic scope class, that all scopes should derive from
//Doesn't really do much

#pragma once
#include <cv.h>

namespace sc
{
	class CameraBase
	{
	public:
		virtual bool getFrame(cv::Mat& outImage) { return true; };
	};
}
