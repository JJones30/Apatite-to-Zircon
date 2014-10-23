//// Tester.cpp : Defines the entry point for the console application.
////
//
#include "stdafx.h"
//
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//	return 0;
//}
//

// Scopetester.cpp

//#define VIRTUAL


#include <iostream>
#include <cv.h>
#include <opencv2/highgui/highgui.hpp>
#include "stage_base.h"

#ifdef VIRTUAL
#include "virtual_stage.h"
#else
#include "camera.h" 
#include "scopecontrol.h"
#include "scope_base.h"
#endif

using namespace std;

//Trackbars
cv::Mat frame;
int trackbar_x;
int trackbar_y;

sc::StageBase* stage;
sc::CameraBase* camera;

void on_x(int, void*)
{
	stage->move_sync(trackbar_x, trackbar_y);

	//((sc::LumeneraCamera*)stage)->getFrame(frame);
	camera->getFrame(frame);

	cv::imshow("Frame Display", frame);
}

void on_y(int, void*)
{
	stage->move_sync(trackbar_x, trackbar_y);

	//((sc::LumeneraCamera*)stage)->getFrame(frame);
	camera->getFrame(frame);

	imshow("Frame Display", frame);
}





int main()
{
	using namespace sc;
	cout << "Initializing stage!" << endl;
	cout << "Current dir: " << system("echo %CD%") << endl;

#ifdef VIRTUAL
	stage = new VirtualScope("virtual_config.xml");
#else
	cout << "Initing camera" << endl;
	camera = new LumeneraCamera();


	cout << "Initing stage" << endl;
	stage = new StageController("COM3", sc::STAGE_TYPE::ASI_MS2000, "C:\\Users\\Ray Donelick\\Documents\\Visual Studio 2013\\Projects\\scopecontrol\\Debug\\asi_ms2000_config.txt");

	cout << "Stage initialized." << endl;

	cout << "Camera created, connecting" << endl;
	((LumeneraCamera*)camera)->connect(1);
#endif
	//stage = new StageController

	cout << "Sending MOVE..." << endl;
	//stage->move_sync(1000, 600);

	cout << "Getting position" << endl;
	double x, y, z;
	stage->where(x, y, z);
	cout << "Position: X=" << x << ", Y=" << y << ", Z=" << z << endl;

	cout << "Getting position again" << endl;
	stage->where(x, y, z);
	cout << "Position: X=" << x << ", Y=" << y << ", Z=" << z << endl;

	cv::Mat frame;
	cout << "Getting a frame" << endl;
	//((VirtualScope*)stage)->getFrame(frame);
	camera->getFrame(frame);

	cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
	cv::imshow("Frame Display", frame);
	double maxX, maxY;
	//((VirtualScope*)stage)->max_position(maxX, maxY);
	maxX = 6;
	maxY = 6;
	//cv::createTrackbar("X", "Frame Display", &trackbar_x, (int)maxX, on_x);
	//cv::createTrackbar("Y", "Frame Display", &trackbar_y, (int)maxY, on_y);

	while (cv::waitKey(1) == -1)
	{
		camera->getFrame(frame);
		cv::imshow("Frame Display", frame);
	}

	cout << "Closing stage." << endl;
	stage->close();
	cout << "Stage closed." << endl;

	delete stage;

	return 0;
}

