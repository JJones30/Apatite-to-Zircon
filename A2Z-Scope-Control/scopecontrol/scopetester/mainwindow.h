//Main window class

#pragma once


//scope control
#include "stage_base.h"
#include "stagecontroller.h"

#include "camera_base.h"
#include "camera.h" 

#include "virtual_stage.h"

//#include "../MountFinder/MountFinder/MountFinder.h"

//scope related
#include "objective.h"

//Qt stuff in stdafx
#include <QtCore/qobject.h>
#include <QProgressBar>
//Qt-related stuff:
#include "status_indicator.h"
//#include "editobjectives_window.h"

//other
#include <string>
#include <memory>
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <unordered_map>
#include "stdafx.h"

#define MM_TO_STAGE_UNITS 10000 //the stage takes move positions in tenths of microns, but that could change someday

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

	//static void __stdcall PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused);
	static void __stdcall PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength); //TODO: check if this can be private or protected

public slots:
	void cameraToggle();
	void stageToggle();

	//Automatically pulls the value from the text box upon editingFinished() signal
	void XYSpeedEdit();
	void ZIncrementEdit();
	void SlideSizeEdit();

	//Takes a frame of the current view and saves it to file as a picture
	void SaveCurrentFrame();

	//Find the origin of our coordinates system
	void FindSlideOrigin();

	//Begin slide traversal
	void SlideTraversal();

	//Seek to given positions
	void SeekPosition();

	//Changes zoom level
	void UpdateZoom10();
	void UpdateZoom20();
	void UpdateZoom40();

	//Update our main window's objectives when the user saves the edit objectives window
	void updateObjectives(std::vector<Objective> newObjectives);

protected:
	void timerEvent(QTimerEvent *event); //Handle timers

	void keyPressEvent(QKeyEvent* event); //Handle keypresses
	void keyReleaseEvent(QKeyEvent* event); //Handle key releases



private:

	//Constructor parts
	void _addMainWidgets();
	void _buildMenuBar();
	void _buildUserControlOptionBox();
	void _buildScopeOptionBox();
	void _initializeScope();

	void _buildEditObjectivesWindow();

	//Timer related
	void _onDebugTimer(); //What to do when our camera timer ticks (AKA get a frame and display it in _camImage)
	void _onPositionTimer(); //How often to update coordinates system
	void _onTraversalTimer(); //Callback to traversal algorithm
	void _onResetTimer(); //Callback to traversal algorithm

	//Key handling related
	void _handleArrowKeyPress(QKeyEvent* event);
	void _handleArrowKeyRelease(QKeyEvent* event);

	//Logging and whatnot
	void _postStatus(std::string status);
	void _previewCam();

	//Handy utility stuff
	//static void _Mat2QImage(const cv::Mat3b &src, QImage **dest);
	void _Mat2QImage(const cv::Mat3b src);
	void FocusImage();
	void ProgressUpdate();
	void SetZoom(double &zoomX, double &zoomY, bool zoomChanged = false);
	void PythonCallout();
	void UpdateDepth();

	int _objectiveAndLightingToIndex(int objectiveIndex, bool lighting);




	//Qt objects
	QLabel* _camImage;
	QImage* _dest;

	QStatusBar* _statusBar;
	SimpleStatusIndicator* _camStatusIndicator;
	SimpleStatusIndicator* _stageStatusIndicator;
	
	QMenuBar* _menuBar;
	
	QMenu* _fileMenu;
	QAction* _exitAction;

	QMenu* _microscopeMenu;
	QAction* _toggleCameraAction;
	QAction* _toggleStageAction;
	QAction* _editObjectivesAction;


	QDockWidget* _userControlOptionDock;

	QGroupBox* _userControlOptionBox;
	QGridLayout* _userControlOptionBoxLayout;
	QLineEdit* _stageXSpeedEdit;
	QLineEdit* _stageYSpeedEdit;
	QLineEdit* _slideWidthEdit;
	QLineEdit* _slideHeightEdit;
	QLineEdit* _stageZIncrementEdit;
	QLineEdit* _desiredXEdit;
	QLineEdit* _desiredYEdit;


	QDockWidget* _scopeOptionDock;

	QGroupBox* _scopeOptionBox;
	QGridLayout* _scopeOptionBoxLayout;
	QComboBox* _currentObjectiveEdit;
	QComboBox* _currentUnits;

	QPushButton* _takePicButton;

	QProgressBar* _progressBar;

	QLabel* _stageXPosLabel;
	QLabel* _stageYPosLabel;
	QLabel* _stageZPosLabel;
	QLabel* _stageZoomLabel;
	QLabel* _progressLabel;
	QLabel* _estTime;


	//Window for editing objectives
//	EditObjectivesWidget* _editObjectivesWindow;

	//Big important things
	sc::StageBase* _stage;
	sc::CameraBase* _cam;
	std::vector<Objective> _objectives;
	cv::Mat _outImage;
	CvMat* _cloneImage;

	//misc
	int _debugUpdateDelay = 0; //delay between asking for frames from the camera
	int _debugTimerID; //the timer ID for the camera timer
	int _positionTimerID; //the timer ID for the coordinates update timer
	int _traversalTimerID;
	int _resetTimerID;
	
	//Stage related things
	//Keyboard control stuff
	bool _enableKeyboardControl = true;
	double _stageXSpeed = .45; //speed at which the stage moves in the X plane when arrow keys are pressed, in mm/sec
	double _stageYSpeed = .30; //speed at which the stage moves in the Y plane when arrow keys are pressed, in mm/sec
	double _stageZIncrement = .01; //amount the stage moves in Z when a key is pressed, in mm/key press
	int _reverseX = -1;
	int _reverseY = -1;
	int _reverseZ = 1;
	int _travRev = -1;
	double _xOffset = 0;
	double _yOffset = 0;
	double _zOffset = 0;
	double _zPos = 0;
	double _desiredX = 0;
	double _desiredY = 0;
	int _zoomLevel = 1;
	int _zCount = 0;
	int _traverseCount = 0;
	int _totalImageCount = 1;
	int _lastX;
	int _lastY;
	int _currentX;
	int _currentY;
	const double VERY_SMALL = 0.000000001;
	bool _offTheSlide = false;
	int _MAXDEPTH = 11;
	int _prevDepth = _MAXDEPTH;
	int _threshold = 15;
	bool _deleted = false;

	//GUI related things
	int _camImageDisplayWidth = 1600;
	int _camImageDisplayHeight = 1200;

	const double MM_40X_X = 0.17;
	const double MM_40X_Y = 0.125;

	const double ZOOM_40X_X = _camImageDisplayWidth / MM_40X_X;
	const double ZOOM_40X_Y = _camImageDisplayHeight / MM_40X_Y;

	const double MM_20X_X = 0.255;
	const double MM_20X_Y = 0.19;

	const double ZOOM_20X_X = _camImageDisplayWidth / MM_20X_X;
	const double ZOOM_20X_Y = _camImageDisplayHeight / MM_20X_Y;

	const double MM_10X_X = 0.7;
	const double MM_10X_Y = 0.5;

	const double ZOOM_10X_X = _camImageDisplayWidth / MM_10X_X;
	const double ZOOM_10X_Y = _camImageDisplayHeight / MM_10X_Y;

	const int ZOOM_10 = 10;
	const int ZOOM_20 = 20;
	const int ZOOM_40 = 40;
	const int SLEEP_TIME = 300;

	const int _stageError = 100;

	int _yPixels = 0;
	int _xPixels = 0;

	double _mmX;
	double _mmY;

	time_t _startTime;
	time_t _currentTime;

	//Settings read from config file
	bool _camAutoconnect = true; //whether or not to autoconnect to the camera on program start
	bool _stageAutoconnect = true; //whether or not to autoconnect to the stage on program start

	bool _enableVirtualScope = false; //whether to use virtual or real
	std::string _vsConfigFile; //virtual scope config file
};