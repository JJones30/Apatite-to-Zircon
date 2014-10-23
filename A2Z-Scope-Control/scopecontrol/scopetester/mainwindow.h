//Main window class

#pragma once


//scope control
#include "stage_base.h"
#include "stagecontroller.h"

#include "camera_base.h"
#include "camera.h" 

#include "virtual_stage.h"

#include "../MountFinder/MountFinder/MountFinder.h"

//scope related
#include "objective.h"

//Qt stuff in stdafx
#include <QtCore/qobject.h>
//Qt-related stuff:
#include "status_indicator.h"
#include "editobjectives_window.h"

//other
#include <string>
#include <memory>

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

	//Takes a frame of the current view and saves it to file as a picture
	void SaveCurrentFrame();

	//Find the origin of our coordinates system
	void MainWindow::FindSlideOrigin();

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

	//Key handling related
	void _handleArrowKeyPress(QKeyEvent* event);
	void _handleArrowKeyRelease(QKeyEvent* event);

	//Logging and whatnot
	void _postStatus(std::string status);


	//Handy utility stuff
	static QImage _Mat2QImage(const cv::Mat3b &src);

	int _objectiveAndLightingToIndex(int objectiveIndex, bool lighting);




	//Qt objects
	QLabel* _camImage;

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
	QLineEdit* _stageXYSpeedEdit;
	QLineEdit* _stageZIncrementEdit;


	QDockWidget* _scopeOptionDock;

	QGroupBox* _scopeOptionBox;
	QGridLayout* _scopeOptionBoxLayout;
	QComboBox* _currentObjectiveEdit;

	QPushButton* _takePicButton;

	QLabel* _stageXPosLabel;
	QLabel* _stageYPosLabel;


	//Window for editing objectives
	EditObjectivesWidget* _editObjectivesWindow;

	//Big important things
	std::unique_ptr<sc::StageBase> _stage;
	std::unique_ptr<sc::CameraBase> _cam;
	std::vector<Objective> _objectives;

	//misc
	int _debugUpdateDelay = 0; //delay between asking for frames from the camera
	int _debugTimerID; //the timer ID for the camera timer
	int _positionTimerID; //the timer ID for the coordinates update timer
	
	//Stage related things
	//Keyboard control stuff
	bool _enableKeyboardControl = true;
	double _stageXYSpeed = .05; //speed at which the stage moves in the XY plane when arrow keys are pressed, in mm/sec
	double _stageZIncrement = .01; //amount the stage moves in Z when a key is pressed, in mm/key press
	int _reverseX = -1;
	int _reverseY = -1;
	int _reverseZ = 1;
	double _xPos = 0;
	double _yPos = 0;
	const double VERY_SMALL = 0.000000001;

	//GUI related things
	int _camImageDisplayWidth = 800;
	int _camImageDisplayHeight = 600;

	//Settings read from config file
	bool _camAutoconnect = true; //whether or not to autoconnect to the camera on program start
	bool _stageAutoconnect = true; //whether or not to autoconnect to the stage on program start

	bool _enableVirtualScope = false; //whether to use virtual or real
	std::string _vsConfigFile; //virtual scope config file
};