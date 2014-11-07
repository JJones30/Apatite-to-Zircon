//Main window class source file

#include "stdafx.h"

#include "mainwindow.h"

MainWindow::MainWindow()
{
	//Qt-Related stuff
	_addMainWidgets();
	_buildMenuBar();

	_buildUserControlOptionBox();

	_buildEditObjectivesWindow();

	//Not
	_initializeScope();

	_debugTimerID = startTimer(_debugUpdateDelay); //and save the ID
	_positionTimerID = startTimer(0); //timer to update current position; 0 == update when idle

	//Set the focus to the central widget
	centralWidget()->setFocus();
}

MainWindow::~MainWindow()
{
	//Disconnect from camera and stage if necessary
	if (_cam->is_connected())
	{
		_cam->disconnect();
	}

	if (_stage->is_connected())
	{
		_stage->close();
	}
	if (_enableVirtualScope){
		delete(_stage);
	}
	else
	{
		delete(_stage);
		delete(_cam);
	}
}

void MainWindow::_addMainWidgets()
{
	//Add the central widget, a video feed from the camera
	_camImage = new QLabel(this);
	_camImage->setFocusPolicy(Qt::ClickFocus);
	setCentralWidget(_camImage);

	//Add the status bar at the bottom
	_statusBar = new QStatusBar(this);
	_statusBar->setSizeGripEnabled(false);
	setStatusBar(_statusBar);

	//Add indicators to the status bar
	_camStatusIndicator = new SimpleStatusIndicator;
	_camStatusIndicator->setText("Camera");
	_camStatusIndicator->disable();
	_statusBar->addPermanentWidget(_camStatusIndicator);

	_stageStatusIndicator = new SimpleStatusIndicator;
	_stageStatusIndicator->setText("Stage");
	_stageStatusIndicator->disable();
	_statusBar->addPermanentWidget(_stageStatusIndicator);

	//Add the menu bar to the top
	_menuBar = new QMenuBar(this);
	setMenuBar(_menuBar);

	//Add the user control options box
	_userControlOptionDock = new QDockWidget("User Control Options", this);
	_userControlOptionDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	_userControlOptionDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	addDockWidget(Qt::RightDockWidgetArea, _userControlOptionDock);

	//Add the scope options box
	_scopeOptionDock = new QDockWidget("Microscope Options", this);
	_scopeOptionDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	_scopeOptionDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	addDockWidget(Qt::RightDockWidgetArea, _scopeOptionDock);
}

void MainWindow::_buildScopeOptionBox()
{
	_scopeOptionBox = new QGroupBox(_scopeOptionDock);
	_scopeOptionBoxLayout = new QGridLayout;

	QLabel* currentObjectiveLabel = new QLabel("Current Objective");
	_currentObjectiveEdit = new QComboBox();

	//Loop through our objectives and add them all to the qcombobox
	//This is index based, so don't mess with the index in the combobox or the _objectives vector or they won't match up
	for (Objective& obj : _objectives)
	{
		QString objName = QString::number(obj.power);

		_currentObjectiveEdit->addItem(objName);
	}

	//TODO: Add a connection from that combobox to the objective changing function

	_scopeOptionBoxLayout->addWidget(currentObjectiveLabel, 0, 0);
	_scopeOptionBoxLayout->addWidget(_currentObjectiveEdit, 0, 1);

	_scopeOptionBox->setLayout(_scopeOptionBoxLayout);
	_scopeOptionDock->setWidget(_scopeOptionBox);
}

void MainWindow::_buildUserControlOptionBox()
{
	//Add _userControlOptionBox
	_userControlOptionBox = new QGroupBox(_userControlOptionDock);
	_userControlOptionBoxLayout = new QGridLayout;
	_userControlOptionBoxLayout->setSizeConstraint(QLayout::SetMinimumSize); //this doesn't currently do much :(

	QLabel* stageXYSpeedLabel = new QLabel("XY speed (mm/sec)");
	_stageXYSpeedEdit = new QLineEdit(QString::number(_stageXYSpeed));
	_userControlOptionBoxLayout->addWidget(stageXYSpeedLabel, 0, 0);
	_userControlOptionBoxLayout->addWidget(_stageXYSpeedEdit, 0, 1);
	connect(_stageXYSpeedEdit, SIGNAL(editingFinished()), this, SLOT(XYSpeedEdit()));

	QLabel* stageZIncrementLabel = new QLabel("Z speed (mm/keypress)");
	_stageZIncrementEdit = new QLineEdit(QString::number(_stageZIncrement));
	_userControlOptionBoxLayout->addWidget(stageZIncrementLabel, 1, 0);
	_userControlOptionBoxLayout->addWidget(_stageZIncrementEdit, 1, 1);
	connect(_stageZIncrementEdit, SIGNAL(editingFinished()), this, SLOT(ZIncrementEdit()));

	//Current Coordinates
	QLabel* xPosLabel = new QLabel("Current xy-pos: ");
	_stageXPosLabel = new QLabel(QString::number(_xPos));
	_stageYPosLabel = new QLabel(QString::number(_yPos));
	_userControlOptionBoxLayout->addWidget(xPosLabel, 2, 0);
	_userControlOptionBoxLayout->addWidget(_stageXPosLabel, 2, 1);
	_userControlOptionBoxLayout->addWidget(_stageYPosLabel, 2, 2);

	QLabel* zoomLevelLabel = new QLabel("Current Zoom Level: ");
	_stageZoomLabel = new QLabel(QString::number(1));
	_userControlOptionBoxLayout->addWidget(zoomLevelLabel, 3, 0);
	_userControlOptionBoxLayout->addWidget(_stageZoomLabel, 3, 1);

	//Add take picture button
	_takePicButton = new QPushButton("Zoom 10x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 4, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(UpdateZoom10()));

	//Add take picture button
	_takePicButton = new QPushButton("Zoom 20x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 5, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(UpdateZoom20()));

	//Add take picture button
	_takePicButton = new QPushButton("Zoom 40x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 6, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(UpdateZoom40()));

	QLabel* utilityLabel = new QLabel("Utility Buttons");
	_userControlOptionBoxLayout->addWidget(utilityLabel, 7, 0);

	//Add take picture button
	_takePicButton = new QPushButton("Take Picture!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 8, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(SaveCurrentFrame()));

	//Add take picture button
	_takePicButton = new QPushButton("Zero Coordinates!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 9, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(FindSlideOrigin()));

	//Add take picture button
	_takePicButton = new QPushButton("Begin Traversal!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 10, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(SlideTraversal()));

	_userControlOptionBox->setLayout(_userControlOptionBoxLayout);
	 
	/*_optionsBoxLayout = new QVBoxLayout;
	_optionsBoxLayout->addWidget(_userControlOptionBox);
	_optionsBox->setLayout(_optionsBoxLayout);*/
	_userControlOptionDock->setWidget(_userControlOptionBox);
}

void MainWindow::_buildMenuBar()
{
	//---------------------File Menu------------------------------
	//Create exit action
	_exitAction = new QAction("Exit", this);
	_exitAction->setShortcuts(QKeySequence::Quit);
	connect(_exitAction, SIGNAL(triggered()), this, SLOT(close()));

	//Add file menu to the menu bar
	_fileMenu = _menuBar->addMenu("File");
	_fileMenu->addAction(_exitAction);

	//---------------------Microscope Menu------------------------------
	//Camera toggle action
	_toggleCameraAction = new QAction("Connect to camera", this); //doesn't need a shortcut
	connect(_toggleCameraAction, SIGNAL(triggered()), this, SLOT(cameraToggle()));

	//Stage toggle action
	_toggleStageAction = new QAction("Connect to stage", this);
	connect(_toggleStageAction, SIGNAL(triggered()), this, SLOT(stageToggle()));

	//Edit objectives action
	_editObjectivesAction = new QAction("Edit objectives", this);

	//add scope menu to menubar
	_microscopeMenu = _menuBar->addMenu("Microscope");
	_microscopeMenu->addAction(_toggleCameraAction);
	_microscopeMenu->addAction(_toggleStageAction);
	_microscopeMenu->addAction(_editObjectivesAction);
}

void MainWindow::_buildEditObjectivesWindow()
{
	_editObjectivesWindow = new EditObjectivesWidget(_objectives);
	_editObjectivesWindow->hide();
	connect(_editObjectivesAction, SIGNAL(triggered()), _editObjectivesWindow, SLOT(show()));

}

void MainWindow::_initializeScope()
{
	//Connect to the camera
	if (_enableVirtualScope)
	{
		//Do virtual scope things

		//cout << "Initializing virtual scope..." << endl;
		_stage = new sc::VirtualScope("virtual_config.xml");
		//cout << "Virtual scope created!" << endl;
		_postStatus("Virtual scope created.");
	}
	else
	{
		//Do real scope things!

		//Create the objects
		_cam = new sc::LumeneraCamera(1);

		_stage = new sc::StageController("COM3",
			sc::STAGE_TYPE::ASI_MS2000,
			"C:\\Users\\Ray Donelick\\Documents\\Visual Studio 2013\\Projects\\scopecontrol\\Debug\\asi_ms2000_config.txt");

		//Camera autoconnect
		if (_camAutoconnect)
		{
			//Connect to the camera
			cameraToggle();

			//Start camera image update timer
			
		}

		//Stage autoconnect
		if (_stageAutoconnect)
		{
			stageToggle();
		}
	}
}

void MainWindow::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == _debugTimerID)
	{
		_onDebugTimer();
	}
	else if(event->timerId() == _positionTimerID)
	{
		_onPositionTimer();
	}
	else if (event->timerId() == _traversalTimerID)
	{
		_onTraversalTimer();
	}
}

void MainWindow::_onPositionTimer()
{
	double zoomX;
	double zoomY;
	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y; 
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y;
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y;
		break;
	default: zoomX = 1; zoomY = 1;
	}
	//Fix double precision to look correct
	if (((_xPos > 0) && (_xPos < VERY_SMALL)) || ((_xPos < 0) && (_xPos > -VERY_SMALL)))
		_xPos = 0;

	if (((_yPos > 0) && (_yPos < VERY_SMALL)) || ((_yPos < 0) && (_yPos > -VERY_SMALL)))
		_yPos = 0;

	//Update current coordinates
	_stageXPosLabel->setText(QString::number(_xPos * zoomX));
	_stageYPosLabel->setText(QString::number(_yPos * zoomY));
	_stageZoomLabel->setText(QString::number(_zoomLevel));
}

void MainWindow::_onTraversalTimer()
{
	double zoomX;
	double zoomY;
	double curX;
	double curY;
	int yPixels = 0;
	int xPixels = 0;

	_stage->where(curX, curY); //get current X position

	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y; xPixels = 132000; yPixels = 134000;
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y; xPixels = 85000; yPixels = 87000;
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y; xPixels = 32000; yPixels = 34000;
		break;
	default: zoomX = 1; zoomY = 1;
	}

	SaveCurrentFrame();

	if (!_stage->move(curX + 1 * _stageXYSpeed * _travRev * MM_TO_STAGE_UNITS, curY))
		_postStatus("Failed to move stage!");
	else
		_xPos += _stageXYSpeed * _travRev * -1;

	Sleep(500);
	// TODO: DEBLUR HERE

	int x = _xPos * zoomX;
	int y = _yPos * zoomY;
	if ((x > xPixels) || (x < 0))
	{
		_stage->where(curX, curY);

		if (!_stage->move(curX, curY + 1 * _stageXYSpeed * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
		else
			_yPos += _stageXYSpeed;

		Sleep(500);
		_travRev *= -1;
		if (y > yPixels)
		{
			killTimer(_traversalTimerID);
		}
	}
}

void MainWindow::_onDebugTimer()
{
	if (_stage->is_connected())
		_statusBar->showMessage("Stage is connected!");
	else
		_statusBar->showMessage("Stage is disconnected!");
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	//If it's an arrow key, send it off to that function
	if ((event->key() == Qt::Key_Right ||
		event->key() == Qt::Key_Left ||
		event->key() == Qt::Key_Up ||
		event->key() == Qt::Key_Down ||
		event->key() == Qt::Key_PageDown ||
		event->key() == Qt::Key_PageUp) &&
		!event->isAutoRepeat())
	{
		_handleArrowKeyPress(event);
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
	//If it's an arrow key, send it off to that function
	if ((event->key() == Qt::Key_Right || 
		 event->key() == Qt::Key_Left ||
		 event->key() == Qt::Key_Up ||
		 event->key() == Qt::Key_Down ||
		 event->key() == Qt::Key_PageDown ||
		 event->key() == Qt::Key_PageUp) &&
		 !event->isAutoRepeat())
	{
		_handleArrowKeyRelease(event);
	}
}

//This includes page up and page down for Z
void MainWindow::_handleArrowKeyPress(QKeyEvent* event)
{
	double curX;
	double curY;
	_stage->where(curX, curY); //get current X position
	//Catch right arrow presses and ignore events sent when its held down
	if (event->key() == Qt::Key_Right && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX + 1 * _stageXYSpeed * _reverseX * MM_TO_STAGE_UNITS, curY))
			_postStatus("Failed to move stage!");
		else
			_xPos += _stageXYSpeed;
	}

	//Catch left arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Left && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX + 1 * _stageXYSpeed * MM_TO_STAGE_UNITS, curY))
			_postStatus("Failed to move stage!");
		else
			_xPos += _stageXYSpeed * _reverseX;
	}

	//Catch up arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Up && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX, curY + 1 * _stageXYSpeed * _reverseY * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
		else
			_yPos += _stageXYSpeed * _reverseY;
	}

	//Catch down arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Down && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX, curY + 1 * _stageXYSpeed * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
		else
			_yPos += _stageXYSpeed;
	}

	//Catch page up presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_PageUp && event->isAutoRepeat() == false)
	{
		_postStatus("Pageup'd");
		//Instead of setting the speed, we move up in increments (setting speed could be dangerous if it crashes while the key is held down)
		double curZ;
		_stage->where(curZ); //get current Z position
		if (!_stage->move(curZ + 1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
		{
			_postStatus("Failed to move stage!");
		}
	}

	//Catch page down presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_PageDown && event->isAutoRepeat() == false)
	{
		//Instead of setting the speed, we move up in increments (setting speed could be dangerous if it crashes while the key is held down)
		double curZ;
		_stage->where(curZ); //get current Z position
		if (!_stage->move(curZ + -1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
		{
			_postStatus("Failed to move stage!");
		}
	}
}

//This includes page up and page down for Z
void MainWindow::_handleArrowKeyRelease(QKeyEvent* event)
{
	//Catch right and left arrow releases and ignore events sent when its held down
	/*
	if ((event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
		&& event->isAutoRepeat() == false)
	{
		_stage->set_speed_x(0);
	}

	else if ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
		&& event->isAutoRepeat() == false)
	{
		_stage->set_speed_y(0);
	}
	*/
	//Nothing happens on releasing Z control keys because it moves in increments instead of how long you hold it down for
}

//void MainWindow::PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused)
void MainWindow::PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength)
{
	MainWindow* caller = (MainWindow*) pContext;

	//get the image size from the camera
	int imageWidth, imageHeight;
	caller->_cam->get_image_size(imageWidth, imageHeight);

	//outImage = cv::Mat(_lffFormat.height, _lffFormat.width, CV_8UC3, data);

	cv::Mat rawFrame;
	//caller->_cam->byteDataToMat(pData, rawFrame);
	rawFrame = cv::Mat(imageHeight, imageWidth, CV_8UC1, pData);

	cv::Mat colorFrame;
	cv::cvtColor(rawFrame, colorFrame, CV_GRAY2RGB);

	//Resize the frame to the desired size
	cv::Mat frame;
	//std::cout << "Resizing frame..." << std::endl;
	cv::resize(colorFrame, frame, cv::Size(caller->_camImageDisplayWidth, caller->_camImageDisplayHeight), 0, 0, cv::INTER_AREA);
	//std::cout << "Resized!" << std::endl;

	//std::cout << "Got a frame, size: " << frame.size() << std::endl;

	//Convert it to a QImage and put it in _camImage
	caller->_camImage->setPixmap(QPixmap::fromImage(_Mat2QImage(frame)));

	//std::cout << caller->_camImage->pixmap()->size().width() << std::endl;

}

void MainWindow::_previewCam()
{
	//get the image size from the camera
	int imageWidth, imageHeight;
	_cam->get_image_size(imageWidth, imageHeight);

	//outImage = cv::Mat(_lffFormat.height, _lffFormat.width, CV_8UC3, data);

	cv::Mat rawFrame;
	cv::Mat frame;

	_cam->getFrame(rawFrame);

	//Resize the frame to the desired size

	//std::cout << "Resizing frame..." << std::endl;
	cv::resize(rawFrame, frame, cv::Size(_camImageDisplayWidth, _camImageDisplayHeight), 0, 0, cv::INTER_AREA);
	//std::cout << "Resized!" << std::endl;

	//std::cout << "Got a frame, size: " << frame.size() << std::endl;

	//Convert it to a QImage and put it in _camImage
	_camImage->setPixmap(QPixmap::fromImage(_Mat2QImage(frame)));
}

void MainWindow::cameraToggle()
{
	if (_cam->is_connected()) //disconnect
	{
		if (_cam->disconnect()) //successful
		{
			_postStatus("Camera disconnected.");
			_camStatusIndicator->disable();
			_toggleCameraAction->setText("Connect to camera"); //TODO: add logs to these
		}
		else //failed for some reason?
		{
			_postStatus("Failed to disconnect camera.");
		}
	}

	else //connect to the cam
	{
		if (_cam->connect())
		{
			_postStatus("Camera connected.");
			_camStatusIndicator->enable();
			_toggleCameraAction->setText("Disconnect from camera");

			//Add a callback function for video frames sent from the camera
			if (!_cam->addNewFrameCallback(&PreviewCallback, (void *) this))
			{
				//std::cerr << "Failed to add video frame callback!" << std::endl; //TODO: add a log print
				_postStatus("Failed to add video frame callback!");
			}


			if (!_cam->startVideoStream())
			{
				//std::cerr << "Failed to start video stream!" << std::endl; //TODO: same thing here
				_postStatus("Failed to start video stream!");
			}
		}
		else //connection failed for some reason
		{
			_postStatus("Failed to connect to camera");//TODO: logs here too
		}
	}
}

void MainWindow::stageToggle()
{
	if (_stage->is_connected()) //disconnect
	{
		if (_stage->close()) //successful
		{
			_postStatus("Stage disconnected.");
			_stageStatusIndicator->disable();
			_toggleStageAction->setText("Connect to stage"); //TODO: add logs to these
		}
		else //failed for some reason?
		{
			_postStatus("Failed to disconnect stage.");
		}
	}

	else //connect to the stage
	{
		if (_stage->connect())
		{
			_postStatus("Stage connected.");
			_stageStatusIndicator->enable();
			_toggleStageAction->setText("Disconnect from stage");
		}
		else //connection failed for some reason
		{
			_postStatus("Failed to connect to stage");//TODO: logs here too
		}
	}
}

void MainWindow::XYSpeedEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageXYSpeed = _stageXYSpeedEdit->text().toDouble();
}

void MainWindow::ZIncrementEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageZIncrement = _stageZIncrementEdit->text().toDouble();
}

void MainWindow::SaveCurrentFrame()
{
	SYSTEMTIME st;
	GetSystemTime(&st);

	//Create time string for picture name
	/*
	char* timeStamp = new char[96]; //96 is 32*3 or 3 ints
	itoa(st.wHour, timeStamp, 10);
	itoa(st.wMinute, &timeStamp[strlen(timeStamp)], 10 /*base 10);
	itoa(st.wSecond, &timeStamp[strlen(timeStamp)], 10 /*base 10);
	*/

	double zoomX;
	double zoomY;
	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y;
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y;
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y;
		break;
	default: zoomX = 1; zoomY = 1;
	}

	int xPos = zoomX * _xPos;
	int yPos = zoomY * _yPos;
	char* position = new char[48];
	itoa(xPos, position, 10 /*base 10*/);
	strncpy(&position[strlen(position)], "_\0", 2);
	itoa(yPos, &position[strlen(position)], 10 /*base 10*/);

	//Create save path
	char* folderPath = "C:/Users/Ray Donelick/Pictures/Test Images";
	int folderlen = strlen(folderPath);
	int posLen = strlen(position);
	char* filename = new char[folderlen + posLen + 1 + 4 + 1]; //1 for '/', 4 for '.jpg', 1 for '\0'
	strncpy(filename, folderPath, folderlen);
	strcpy(filename + folderlen, "/");
	strncpy(filename + folderlen + 1, position, posLen);
	strcpy(filename + folderlen + posLen + 1, ".jpg\0");

	//Write out picture
	_cam->getFrame(outImage);
	cv::imwrite(filename, outImage);

	//Manage memory
	delete[] filename;
	delete[] position;
}

void MainWindow::FindSlideOrigin()
{
	_xPos = 0;
	_yPos = 0;
	//run_corner_detection(_stage, _cam);
}

void MainWindow::SlideTraversal()
{
	_traversalTimerID = startTimer(0);
	/*
	_manual = true;
	cameraToggle();
	cameraToggle();

	double zoomX;
	double zoomY;
	double curX;
	double curY;
	int reverse = -1;
	int yPixels = 0;
	int xPixels = 0;
	int count = 1;

	_stage->where(curX, curY); //get current X position

	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y; xPixels = 132000; yPixels = 134000;
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y; xPixels = 85000; yPixels = 87000;
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y; xPixels = 32000; yPixels = 34000;
		break;
	default: zoomX = 1; zoomY = 1;
	}


	for (int y = 0; y < yPixels; y = _yPos * zoomY)
	{
		_stage->where(curX, curY);
		for (int x = 0; ((x < xPixels) && (x >= 0)); x = _xPos * zoomX, ++count)
		{
			SaveCurrentFrame();
			
			if (!_stage->move(curX + 1 * _stageXYSpeed * reverse * MM_TO_STAGE_UNITS, curY))
				_postStatus("Failed to move stage!");
			else
				_xPos += _stageXYSpeed * reverse * -1;

			if (count % 50 == 0)
			{
				cameraToggle();
				cameraToggle();
			}
			Sleep(500);
			// TODO: DEBLUR HERE

			_stage->where(curX, curY);
		}
		

		if (!_stage->move(curX, curY + 1 * _stageXYSpeed * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
		else
			_yPos += _stageXYSpeed;

		Sleep(500);
		reverse *= -1;
	}

	_manual = false;
	cameraToggle();
	cameraToggle();
	*/
}

void MainWindow::UpdateZoom10()
{
	_zoomLevel = ZOOM_10;
}

void MainWindow::UpdateZoom20()
{
	_zoomLevel = ZOOM_20;
}

void MainWindow::UpdateZoom40()
{
	_zoomLevel = ZOOM_40;
}

void MainWindow::updateObjectives(std::vector<Objective> newObjectives)
{
	//Update the actual variable
	_objectives = newObjectives;

	//Update the combo box
	QComboBox* newObjectivesBox = new QComboBox;

	for (Objective& obj : _objectives)
	{
		QString objName = QString::number(obj.power);

		newObjectivesBox->addItem(objName);
	}

	_currentObjectiveEdit = newObjectivesBox;
}

//Logging and whatnot
void MainWindow::_postStatus(const std::string status)
{
	//_statusBar->showMessage(QString::fromStdString(status));
	_statusBar->showMessage(status.c_str());
}

//Handy utility functions
QImage MainWindow::_Mat2QImage(const cv::Mat3b &src)
{
	QImage dest(src.cols, src.rows, QImage::Format_ARGB32);

	for (int y = 0; y < src.rows; y++)
	{
		const cv::Vec3b *srcrow = src[y];
		QRgb *destrow = (QRgb*)dest.scanLine(y);

		for (int x = 0; x < src.cols; x++)
		{
			destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
		}
	}

	return dest;
}

int MainWindow::_objectiveAndLightingToIndex(int objectiveIndex, bool lighting)
{
	if(lighting) //if reflected, second set
	{
		return objectiveIndex + _objectives.size();
	}
	else
	{
		return objectiveIndex;
	}
}