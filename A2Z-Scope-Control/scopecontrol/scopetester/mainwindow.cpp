//Main window class source file

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

	_stage->where(_xOffset, _yOffset, _zOffset);
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

	//X and Y speed controls
	QLabel* stageXYSpeedLabel = new QLabel("XY speed (mm/sec)");
	_stageXSpeedEdit = new QLineEdit(QString::number(_stageXSpeed));
	_stageYSpeedEdit = new QLineEdit(QString::number(_stageYSpeed));
	_userControlOptionBoxLayout->addWidget(stageXYSpeedLabel, 0, 0);
	_userControlOptionBoxLayout->addWidget(_stageXSpeedEdit, 0, 1);
	_userControlOptionBoxLayout->addWidget(_stageYSpeedEdit, 0, 2);
	connect(_stageXSpeedEdit, SIGNAL(editingFinished()), this, SLOT(XYSpeedEdit()));
	connect(_stageYSpeedEdit, SIGNAL(editingFinished()), this, SLOT(XYSpeedEdit()));

	//Z speed controls
	QLabel* stageZIncrementLabel = new QLabel("Z speed (mm/keypress)");
	_stageZIncrementEdit = new QLineEdit(QString::number(_stageZIncrement));
	_userControlOptionBoxLayout->addWidget(stageZIncrementLabel, 1, 0);
	_userControlOptionBoxLayout->addWidget(_stageZIncrementEdit, 1, 1);
	connect(_stageZIncrementEdit, SIGNAL(editingFinished()), this, SLOT(ZIncrementEdit()));

	//Current Coordinates
	QLabel* xPosLabel = new QLabel("Current xyz-pos: ");
	_stageXPosLabel = new QLabel(QString::number(0));
	_stageYPosLabel = new QLabel(QString::number(0));
	_stageZPosLabel = new QLabel(QString::number(0));
	_userControlOptionBoxLayout->addWidget(xPosLabel, 2, 0);
	_userControlOptionBoxLayout->addWidget(_stageXPosLabel, 2, 1);
	_userControlOptionBoxLayout->addWidget(_stageYPosLabel, 2, 2);
	_userControlOptionBoxLayout->addWidget(_stageZPosLabel, 2, 3);

	//A label to keep track of current optical zoom
	QLabel* zoomLevelLabel = new QLabel("Current Zoom Level: ");
	_stageZoomLabel = new QLabel(QString::number(1));
	_userControlOptionBoxLayout->addWidget(zoomLevelLabel, 3, 0);
	_userControlOptionBoxLayout->addWidget(_stageZoomLabel, 3, 1);

	//Create three buttons to specify optical zoom
	QPushButton* firstZoom = new QPushButton("Zoom 10x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(firstZoom, 4, 0);
	connect(firstZoom, SIGNAL(released()), this, SLOT(UpdateZoom10()));

	QPushButton* secondZoom = new QPushButton("Zoom 20x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(secondZoom, 5, 0);
	connect(secondZoom, SIGNAL(released()), this, SLOT(UpdateZoom20()));

	QPushButton* thirdZoom = new QPushButton("Zoom 40x", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(thirdZoom, 6, 0);
	connect(thirdZoom, SIGNAL(released()), this, SLOT(UpdateZoom40()));

	//The beginning of automation utilities
	QLabel* utilityLabel = new QLabel("Utility Buttons");
	_userControlOptionBoxLayout->addWidget(utilityLabel, 7, 0);

	//Add take picture button
	_takePicButton = new QPushButton("Take Picture!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(_takePicButton, 8, 0);
	connect(_takePicButton, SIGNAL(released()), this, SLOT(SaveCurrentFrame()));

	//A button to zero coordinates to the current position
	QPushButton* zeroButton = new QPushButton("Zero Coordinates!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(zeroButton, 9, 0);
	connect(zeroButton, SIGNAL(released()), this, SLOT(FindSlideOrigin()));

	//A button to begin the traversal algorithm
	QPushButton* traverseButton = new QPushButton("Begin Traversal!", _scopeOptionDock);
	QLabel* completeLabel = new QLabel("% Complete: ");
	QLabel* _progressLabel = new QLabel("N/A");
	_userControlOptionBoxLayout->addWidget(traverseButton, 10, 0);
	_userControlOptionBoxLayout->addWidget(completeLabel, 10, 1);
	_userControlOptionBoxLayout->addWidget(_progressLabel, 10, 2);
	connect(traverseButton, SIGNAL(released()), this, SLOT(SlideTraversal()));

	//Adds an interface for automatic navigation to a specified coordinate
	QLabel* desiredX = new QLabel("Desired X/Y");
	QPushButton* goButton = new QPushButton("Go To", _scopeOptionDock);
	_desiredXEdit = new QLineEdit(QString::number(_desiredX));
	_desiredYEdit = new QLineEdit(QString::number(_desiredY));
	_userControlOptionBoxLayout->addWidget(desiredX, 11, 0);
	_userControlOptionBoxLayout->addWidget(_desiredXEdit, 11, 1);
	_userControlOptionBoxLayout->addWidget(_desiredYEdit, 11, 2);
	_userControlOptionBoxLayout->addWidget(goButton, 11, 3);
	connect(goButton, SIGNAL(released()), this, SLOT(SeekPosition()));

	//Allows for dynamic sizing of the traversal algorithm
	QLabel* stageWidthLabel = new QLabel("Slide width/height");
	QPushButton* setButton = new QPushButton("Set", _scopeOptionDock);
	_slideWidthEdit = new QLineEdit(QString::number(_xPixels));
	_slideHeightEdit = new QLineEdit(QString::number(_yPixels));
	_userControlOptionBoxLayout->addWidget(stageWidthLabel, 12, 0);
	_userControlOptionBoxLayout->addWidget(_slideWidthEdit, 12, 1);
	_userControlOptionBoxLayout->addWidget(_slideHeightEdit, 12, 2);
	_userControlOptionBoxLayout->addWidget(setButton, 12, 3);
	connect(setButton, SIGNAL(released()), this, SLOT(SlideSizeEdit()));


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
	
	SetZoom(zoomX, zoomY);

	double x;
	double y;
	//Update current coordinates
	_stage->where(x, y);
	_stageXPosLabel->setText(QString::number((x - _xOffset) / -MM_TO_STAGE_UNITS * zoomX));
	_stageYPosLabel->setText(QString::number((y - _yOffset) / MM_TO_STAGE_UNITS * zoomY));
	_stageZPosLabel->setText(QString::number(_zPos * _stageZIncrement * 100));
	_stageZoomLabel->setText(QString::number(_zoomLevel));
}

void MainWindow::_onTraversalTimer()
{
	double zoomX;
	double zoomY;
	double curX;
	double curY;
	double curZ;
	bool finished = false;
	int zoomChange = 0;
	int noChangeInY = 1;

	SetZoom(zoomX, zoomY);

	_stage->where(curX, curY);
	int x = zoomX * (curX - _xOffset) / -MM_TO_STAGE_UNITS;
	int y = zoomY * (curY - _yOffset) / MM_TO_STAGE_UNITS;
	if (((x > _xPixels) || (x < -_stageError/2)) && (_zCount % 	_MAXDEPTH == 1 && _zPos == 0))
	{
		if (!_stage->move(curX + 1 * 0.002 * -1 * MM_TO_STAGE_UNITS, curY + 1 * _stageYSpeed * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");

		Sleep(300);
		_travRev *= -1;
		_zCount--;
		_stage->where(curX, curY);
		x = zoomX * (curX - _xOffset) / -MM_TO_STAGE_UNITS;
		y = zoomY * (curY - _yOffset) / MM_TO_STAGE_UNITS;
		if (y >= _yPixels)
		{
			killTimer(_traversalTimerID);
			_zCount = 0;
			_travRev = -1;
			finished = true;
		}
		_stage->where(curX, curY);
		noChangeInY = 0;
	}
	if (!finished)
	{
		if (_zCount == 0)
		{
			SaveCurrentFrame();
			++_zCount;
		}

		if (_zCount % _MAXDEPTH == 0 && _zPos == 0)
		{
			if (noChangeInY)
			{
				FocusImage();
				_traverseCount++;
				ProgressUpdate();
			}
			else
			{
				noChangeInY = 1;
			}

			if (!_stage->move(curX + 1 * _stageXSpeed * _travRev * MM_TO_STAGE_UNITS, curY))
				_postStatus("Failed to move stage!");

			Sleep(300);
			_stage->move(_zOffset);
			Sleep(300);
			SaveCurrentFrame();
			++_zCount;
		}
		else if (_zCount % _MAXDEPTH < (_MAXDEPTH/2 + 1) && _zCount % _MAXDEPTH > 0)
		{
			_stage->where(curZ);

			if (!_stage->move(curZ + 1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			else
				_zPos++;

			Sleep(300);
			SaveCurrentFrame();
			++_zCount;
		}
		else if (_zPos != 0 && _zCount % _MAXDEPTH == (_MAXDEPTH/2 + 1))
		{
			_stage->where(curZ);

			if (!_stage->move(curZ - 1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			else
				_zPos--;
			Sleep(300);
		}
		else if (_zCount % _MAXDEPTH > _MAXDEPTH/2)
		{
			_stage->where(curZ);

			if (!_stage->move(curZ - 1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			else
				_zPos--;

			Sleep(300);
			SaveCurrentFrame();
			++_zCount;
		}
		else
		{
			_stage->where(curZ);

			if (!_stage->move(curZ + 1 * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			else
				_zPos++;
			Sleep(300);
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
		if (!_stage->move(curX + 1 * _stageXSpeed * _reverseX * MM_TO_STAGE_UNITS, curY))
			_postStatus("Failed to move stage!");
	}

	//Catch left arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Left && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX + 1 * _stageXSpeed * MM_TO_STAGE_UNITS, curY))
			_postStatus("Failed to move stage!");
	}

	//Catch up arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Up && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX, curY + 1 * _stageYSpeed * _reverseY * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
	}

	//Catch down arrow presses and ignore events sent when its held down
	else if (event->key() == Qt::Key_Down && event->isAutoRepeat() == false)
	{
		if (!_stage->move(curX, curY + 1 * _stageYSpeed * MM_TO_STAGE_UNITS))
			_postStatus("Failed to move stage!");
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
		else
		{
			_zPos++;
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
		else
		{
			_zPos--;
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

void MainWindow::SetZoom(double &zoomX, double &zoomY, bool zoomChanged)
{
	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y, _mmX = MM_40X_X, _mmY = MM_40X_Y; 
		if (zoomChanged)
		{
			_xPixels = 132000; _yPixels = 134000;
		}
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y, _mmX = MM_20X_X, _mmY = MM_20X_Y;
		if (zoomChanged)
		{
			_xPixels = 85000; _yPixels = 87000;
		}
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y, _mmX = MM_10X_X, _mmY = MM_10X_Y;
		if (zoomChanged)
		{
			_xPixels = 32000; _yPixels = 34000;
		}
		break;
	default: zoomX = 1; zoomY = 1;
	}

	/*
	//Fix double precision to look correct
	if (((_xPos > 0) && (_xPos < VERY_SMALL)) || ((_xPos < 0) && (_xPos > -VERY_SMALL)))
		_xPos = 0;

	if (((_yPos > 0) && (_yPos < VERY_SMALL)) || ((_yPos < 0) && (_yPos > -VERY_SMALL)))
		_yPos = 0;

	if (((_zPos > 0) && (_zPos < VERY_SMALL)) || ((_zPos < 0) && (_zPos > -VERY_SMALL)))
		_zPos = 0;
	*/

}

void MainWindow::XYSpeedEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageXSpeed = _stageXSpeedEdit->text().toDouble();
	_stageYSpeed = _stageYSpeedEdit->text().toDouble();
}

void MainWindow::ZIncrementEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageZIncrement = _stageZIncrementEdit->text().toDouble();
}

void MainWindow::SlideSizeEdit()
{
	double zoomX, zoomY;

	SetZoom(zoomX, zoomY);

	int numXImages = _slideWidthEdit->text().toInt();
	int numYImages = _slideHeightEdit->text().toInt();

	_totalImageCount = numXImages * numYImages;
	_totalImageCount > 0 ? _totalImageCount  : _totalImageCount = 1;

	_xPixels = (_stageXSpeed / _mmX) * _camImageDisplayWidth * numXImages - _stageError;
	_yPixels = (_stageYSpeed / _mmY) * _camImageDisplayHeight * numYImages - _stageError;

	//_xPixels = _slideWidthEdit->text().toInt();
	//_yPixels = _slideHeightEdit->text().toInt();
}

void MainWindow::ProgressUpdate()
{
	_progressLabel->setText(QString::number(_traverseCount / _totalImageCount));
}

void MainWindow::SeekPosition()
{
	_desiredX = _desiredXEdit->text().toDouble();
	_desiredY = _desiredYEdit->text().toDouble();
	double curX;
	double curY;
	double zoomX;
	double zoomY;

	SetZoom(zoomX, zoomY);

	_desiredX /= zoomX;
	_desiredY /= zoomY;


	_stage->where(curX, curY);
	if (!_stage->move(_xOffset + _desiredX * MM_TO_STAGE_UNITS * -1, curY))
		_postStatus("Failed to move stage!");

	Sleep(2500);

	_stage->where(curX, curY);
	if (!_stage->move(curX, _yOffset +_desiredY * MM_TO_STAGE_UNITS))
		_postStatus("Failed to move stage!");

}

void MainWindow::SaveCurrentFrame()
{
	double zoomX;
	double zoomY;
	
	SetZoom(zoomX, zoomY);

	//Create position string for picture name
	double curX;
	double curY;
	_stage->where(curX, curY);
	int xPos = zoomX * (curX - _xOffset) / -MM_TO_STAGE_UNITS;
	int yPos = zoomY * (curY - _yOffset) / MM_TO_STAGE_UNITS;
	int zPos = _zPos * _stageZIncrement * 1000;
	char* position = new char[72];
	itoa(xPos, position, 10 /*base 10*/);
	strncpy(&position[strlen(position)], "_\0", 2);
	itoa(yPos, &position[strlen(position)], 10 /*base 10*/);

	//Create save path
	char* startPath = "C:\\Users\\Ray Donelick\\Pictures\\Test Images\\";
	int startPathLen = strlen(startPath);
	int oldposLen = strlen(position);
	char* folderPath = new char[startPathLen + oldposLen + 1];
	strncpy(folderPath, startPath, startPathLen);
	strncpy(folderPath + startPathLen, position, strlen(position));
	strncpy(&position[oldposLen], "_\0", 2);
	itoa(zPos, &position[strlen(position)], 10 /*base 10*/);
	int newposLen = strlen(position);
	char* filename = new char[startPathLen + oldposLen + newposLen + 1 + 4 + 1]; //1 for '/', 4 for '.jpg', 1 for '\0'
	strncpy(filename, folderPath, startPathLen + oldposLen);
	strcpy(filename + startPathLen + oldposLen, "\0");
	const char* dir_path = filename;
	boost::filesystem::path dir(dir_path);
	boost::filesystem::create_directory(dir);
	strcpy(filename + startPathLen + oldposLen, "\\\0");
	strncpy(filename + startPathLen + oldposLen + 1, position, newposLen);
	strcpy(filename + startPathLen + oldposLen + 1 + newposLen, ".jpg\0");

	//Write out picture
	_cam->getFrame(_outImage);
	cv::cvtColor(_outImage, _outImage, CV_RGB2GRAY);
	cv::imwrite(filename, _outImage);

	//Manage memory
	delete[] filename;
	delete[] position;
	delete[] folderPath;
}

void MainWindow::FocusImage()
{
	double zoomX;
	double zoomY;
	
	SetZoom(zoomX, zoomY);

	double curX;
	double curY;
	_stage->where(curX, curY);
	int xPos = zoomX * (curX - _xOffset) / -MM_TO_STAGE_UNITS;
	int yPos = zoomY * (curY - _yOffset) / MM_TO_STAGE_UNITS;
	int zPos = _zPos * 1000;
	char* position = new char[72];
	itoa(xPos, position, 10 /*base 10*/);
	strncpy(&position[strlen(position)], "_\0", 2);
	itoa(yPos, &position[strlen(position)], 10 /*base 10*/);

	char* commonCmd = "\"\"C:\\Users\\Ray Donelick\\Downloads\\ij148\\ImageJ\\ImageJ.exe\" -macro FocuserMacro.ijm ";
	int startPathLen = strlen(commonCmd);
	int posLen = strlen(position);
	char* fullCmd = new char[startPathLen + posLen + 2];
	strncpy(fullCmd, commonCmd, startPathLen);
	strncpy(fullCmd + startPathLen, position, strlen(position) + 1);
	strcpy(&fullCmd[strlen(fullCmd)], "\"\0");

	int okay = std::system(fullCmd);
	if (!okay)
		std::cerr << okay << " Everything is not Okay!" << std::endl;

	delete[] position;
	delete[] fullCmd;
}

void MainWindow::FindSlideOrigin()
{
	_stage->where(_xOffset, _yOffset, _zOffset);
	_zPos = 0;
}

void MainWindow::SlideTraversal()
{
	_traversalTimerID = startTimer(0);
	_zPos = 0;
}

void MainWindow::UpdateZoom10()
{
	double x, y;
	_zoomLevel = ZOOM_10;
	SetZoom(x, y, true);
}

void MainWindow::UpdateZoom20()
{
	double x, y;
	_zoomLevel = ZOOM_20;
	SetZoom(x, y, true);
}

void MainWindow::UpdateZoom40()
{
	double x, y;
	_zoomLevel = ZOOM_40;
	SetZoom(x, y, true);
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