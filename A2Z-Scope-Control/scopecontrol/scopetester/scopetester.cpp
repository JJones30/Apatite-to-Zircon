// Scopetester.cpp

//#define VIRTUAL


//#include <iostream>
//#include <cv.h>
//#include <opencv2/highgui/highgui.hpp>
//
//#include "camera_base.h"
//#include "stage_base.h"
//
//#ifdef VIRTUAL
//#include "virtual_stage.h"
//#else
//#include "camera.h" 
//#include "stagecontroller.h"
//#endif

//QT
#include <QtWidgets/qapplication.h>

//Qt-derived
#include "mainwindow.h"


int main(int argc, char* argv[])
{
	//Set the visual style of our app
	QApplication::setStyle("windowsxp"); 
	
	//Create the application!
	QApplication app(argc, argv);
	app.setOrganizationName("testApp");

	MainWindow mainwin;

	mainwin.show();

	return app.exec();
}

//stage->move_sync(1000, 600);
//stage->where(x, y, z);