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
		delete(_dest);
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
	QPushButton* takePicButton = new QPushButton("Take Picture!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(takePicButton, 8, 0);
	connect(takePicButton, SIGNAL(released()), this, SLOT(SaveCurrentFrame()));

	//A button to zero coordinates to the current position
	QPushButton* zeroButton = new QPushButton("Zero Coordinates!", _scopeOptionDock);
	_userControlOptionBoxLayout->addWidget(zeroButton, 9, 0);
	connect(zeroButton, SIGNAL(released()), this, SLOT(FindSlideOrigin()));

	//A button to begin the traversal algorithm
	QPushButton* traverseButton = new QPushButton("Begin Traversal!", _scopeOptionDock);
	//QLabel* completeLabel = new QLabel("% Complete: ");
	//_progressLabel = new QLabel("N/A");
	_progressBar = new QProgressBar(_scopeOptionDock);
	_estTime = new QLabel("N/A");
	_userControlOptionBoxLayout->addWidget(traverseButton, 10, 0);
	_userControlOptionBoxLayout->addWidget(_progressBar, 10, 1);
	_userControlOptionBoxLayout->addWidget(_estTime, 10, 2);
	//_userControlOptionBoxLayout->addWidget(completeLabel, 10, 1);
	//_userControlOptionBoxLayout->addWidget(_progressLabel, 10, 2);
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

	_currentUnits = new QComboBox(_scopeOptionDock);
	_currentUnits->addItem("Millimeters");
	_currentUnits->addItem("Pixels");
	_currentUnits->addItem("Frames");
	_userControlOptionBoxLayout->addWidget(_currentUnits, 13, 0);

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
	//_editObjectivesWindow = new EditObjectivesWidget(_objectives);
	//_editObjectivesWindow->hide();
	//connect(_editObjectivesAction, SIGNAL(triggered()), _editObjectivesWindow, SLOT(show()));

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
		_dest = NULL;
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
	else if (event->timerId() == _resetTimerID)
	{
		_onResetTimer();
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


/*
 * Function: _onTraversalTimer
 * Author: Justin Jones HMC '15
 * Description: _onTraversalTimer essentially represents a finite state machine for
 *				traversal of a microscope slide. When the "Begin Traversal" button
 *				on the UI is pressed, it activates a timer callback supported by QT
 *				which continually calls this function at every idle moment. Previously,
 *				this function was run with loops, but since this was using the main thread
 *				no other information was updated (camImage, coordinate system, etc). To
 *				avoid extensive threading this callback work around is used. This allows
 *				information on the screen to update and allows for mouse clicks to occur.
 *				
 *				The code manipulates the microscope in a ox-plow pattern, going along a row 
 *				until the end then dropping to the next row. At each X-Y coordinate, a stack
 *				of images at different z-depths is captured (_MAXDEPTH images to be precise). 
 *				Once a stack has been captured, it is sent to an Java ImageJ library to be
 *				compressed into a focused image. Meanwhile, the scope proceeds to the next X-Y
 *				coordinate. The image processing and capturing happen in parallel. The finite
 *				state machine largely determines the state by the number of changes in the z-axis.
 *				Upon completion, the function resets relevant member variables and cancels the
 *				timer. There are a couple of Sleep commands that are used to allow the mechanical
 *				movement to complete. The traversal completes over whatever number of pixels is
 *				currently set as the slide size and has a progress bar to indicate status.	
 */
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
			_traverseCount = 0;
			finished = true;
			_progressBar->reset();
			Sleep(10000);
			PythonCallout();
		}
		_stage->where(curX, curY);
		noChangeInY = 0;
	}
	if (!finished)
	{
		if (_traverseCount == 0 && _zCount == 0)
		{
			SaveCurrentFrame();
			++_zCount;
		}

		if (_zCount % _MAXDEPTH == 0 && _zPos == 0)
		{
			if (noChangeInY)
			{
				FocusImage();
				if (_traverseCount == 0)
				{
					_currentX = x;
					_currentY = y;
				}
				else
				{
					_lastX = _currentX;
					_lastY = _currentY;
					_currentX = x;
					_currentY = y;
					UpdateDepth();

					if (_traverseCount % 25 == 0)
					{
						_resetTimerID = startTimer(5000);
						killTimer(_traversalTimerID);
					}
				}

				_zCount = 0;

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


void MainWindow::UpdateDepth()
{
	cv::Mat heightMap;
	int max = 0;
	int min = 1920000;
	double maxVal;
	int depthCount[256];
	std::string path = "C:\\Users\\Ray Donelick\\Pictures\\Test Images\\Height Maps\\HeightMap_";
	path.append(std::to_string(_lastX));
	path.append("_");
	path.append(std::to_string(_lastY));
	path.append(".png");
	heightMap = cv::imread(path, 0);
	while (heightMap.rows == 0)
		heightMap = cv::imread(path, 0);

	for (int k = 0; k < 257; ++k)
		depthCount[k] = 0;

	int channel = heightMap.channels();

	int nRows = heightMap.rows;
	int nCols = heightMap.cols * channel;

	if (heightMap.isContinuous())
	{
		nCols *= nRows;
		nRows = 1;
	}

	int i, j;
	uchar* p;
	for (i = 0; i < nRows; ++i)
	{
		p = heightMap.ptr<uchar>(i);
		for (j = 0; j < nCols; ++j)
		{
			depthCount[(int)p[j]] += 1;
		}
	}

	for (int index = 0; index < 257; ++index)
	{
		if (depthCount[index] > max)
		{
			max = depthCount[index];
			maxVal = index;
		}
		if (depthCount[index] < min && depthCount[index] > 0)
		{
			min = depthCount[index];
		}
	}

	int pctDiff = (min * 100) / (max + 1);
	pctDiff == 0 ? pctDiff = 31 : pctDiff;
	if (pctDiff < _threshold)
	{
		_offTheSlide = false;
		_MAXDEPTH = _prevDepth;
		int zeroPt = ((_MAXDEPTH / 2) + 1);
		int depthChange = (maxVal   / (255 / _MAXDEPTH)) - zeroPt;
		if (depthChange > 3)
			depthChange = 3;
		if (depthChange < -3)
			depthChange = -3;
		double curZ;
		_stage->where(curZ); //get current Z position
		if (depthChange > 0)
		{
			if (!_stage->move(curZ + depthChange * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			Sleep(300);
		}
		else if (depthChange < 0)
		{
			if (!_stage->move(curZ + depthChange * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			Sleep(300);
		}
	}
	else if (_offTheSlide && pctDiff < _threshold * 2)
	{
		_MAXDEPTH = 25;
		int zeroPt = ((_MAXDEPTH / 2) + 1);
		int depthChange = (maxVal / (255 / _MAXDEPTH)) - zeroPt;
		if (depthChange > 2)
			depthChange = 2;
		if (depthChange < -2)
			depthChange = -2;
		double curZ;
		_stage->where(curZ); //get current Z position
		if (depthChange > 0)
		{
			if (!_stage->move(curZ + depthChange * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			Sleep(300);
		}
		else if (depthChange < 0)
		{
			if (!_stage->move(curZ + depthChange * _stageZIncrement * _reverseZ * MM_TO_STAGE_UNITS))
				_postStatus("Failed to move stage!");
			Sleep(300);
		}
	}
	else if (_offTheSlide && _zoomLevel == 10 && pctDiff < _threshold * 3.5)
	{
		_MAXDEPTH = 35;
	}
	else
	{
		if (_offTheSlide)
			_MAXDEPTH = _prevDepth;
		_offTheSlide = true;
	}

}


void MainWindow::_onResetTimer()
{
	_traversalTimerID = startTimer(0);
	killTimer(_resetTimerID);
}


/*
 * Function: _onDebugTimer
 * Author: Ravi Kumar HMC '14
 * Description: This function relies on a QT signal to call back at every idle moment and
 *				updates the status bar message indicating the connection to the stage.
 */
void MainWindow::_onDebugTimer()
{
	if (_stage->is_connected())
		_statusBar->showMessage("Stage is connected!");
	else
		_statusBar->showMessage("Stage is disconnected!");
}


/*
 * Function: keyPressEvent
 * Author: Ravi Kumar HMC '14
 * Description: This function relies on a QT signal to call back when a keyevent occurs.
 *				It is only called upon press of a key and is sent to _handleArrowKeyPress.
 *
 * Parameters: event - this tells which key was pressed
 */
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


/*
 * Function: keyReleaseEvent
 * Author: Ravi Kumar HMC '14
 * Description: This function relies on a QT signal to call back when a keyevent occurs.
 *				It is only called upon release of a key and is sent to _handleArrowKeyRelease. 
 *
 * Parameters: event - this tells which key was released
 */
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


/*
 * Function: _handleArrowKeyPress
 * Author: Ravi Kumar HMC '14
 * Last Edit: Justin Jones HMC '15
 * Description: This function sets directional speed when an arrow key is pressed.
 *			    Currently all key presses in any direction are incremental with 
 *			    autorepeat turned off so that holding down a key does not continue movement.
 *			    The function handles X, Y, and Z which are controlled respectively by the left,
 *			    right, up, down arrow keysand page up/down.
 *
 * Parameters: event - this tells which key was pressed so that the function knows which
 *					   directional speed needs to be set
 */
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


/*
 * Function: _handleArrowKeyRelease
 * Author: Ravi Kumar HMC '14
 * Last Edit: Justin Jones HMC '15
 * Description: This function sets directional speed to 0 when an arrow key is released.
 *
 * Update: Possibly irrelevant since autorepeat is currently disabled for all directions
 *		   due to an increase in precision over smaller movements. Justin Jones HMC '15
 *
 * TODO: FIXME
 *
 * Parameters: event - this tells which key was pressed so that the function knows which
 *					   directional speed needs to be set to 0
 */
void MainWindow::_handleArrowKeyRelease(QKeyEvent* event)
{
	//Catch right and left arrow releases and ignore events sent when its held down
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
	//Nothing happens on releasing Z control keys because it moves in increments instead of how long you hold it down for
}


/*
 * Function: PreviewCallback
 * Author: Ravi Kumar HMC '14
 * Description: In camera toggle, a callback is created to update the image on the UI.
 *				The callback returns to this function when it is signaled. This function
 *				takes an image from the camera and posts it to the UI.
 *
 * Parameters: pContext - a void pointer that tells the function that "this" (the MainWindow
 *						 class) called this function
 *			  
 *			   pData - this is the image data in a BYTE array
 *
 *			   dataLength - this is the length of the BYTE array
 */
//void MainWindow::PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused)
void MainWindow::PreviewCallback(VOID *pContext, BYTE *pData, ULONG dataLength)
{
	MainWindow* caller = (MainWindow*) pContext;
	if (caller->_traverseCount%3 == 0 && caller->_zCount == 0)
	{
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
		if (caller->_dest == NULL)
		{
			caller->_dest = new QImage(frame.cols, frame.rows, QImage::Format_ARGB32);
		}

		caller->_Mat2QImage(frame);
		//QImage dest = QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_ARGB32);

		//std::cout << "Got a frame, size: " << frame.size() << std::endl;

		//Convert it to a QImage and put it in _camImage
		caller->_camImage->setPixmap(QPixmap::fromImage(*(caller->_dest)));
	}
	//std::cout << caller->_camImage->pixmap()->size().width() << std::endl;
}


/*
 * Function: _previewCam
 * Author: Ravi Kumar HMC '14
 * Description: I am not sure that this function is used (Justin Jones). 
 * TODO: FIXME
 */
void MainWindow::_previewCam()
{
	//get the image size from the camera

	//outImage = cv::Mat(_lffFormat.height, _lffFormat.width, CV_8UC3, data);


	//Resize the frame to the desired size

	//std::cout << "Resizing frame..." << std::endl;

	//std::cout << "Resized!" << std::endl;

	//std::cout << "Got a frame, size: " << frame.size() << std::endl;

	//Convert it to a QImage and put it in _camImage
	cv::Mat frame;
	//std::cout << "Resizing frame..." << std::endl;
	cv::resize(_outImage, frame, cv::Size(_camImageDisplayWidth, _camImageDisplayHeight), 0, 0, cv::INTER_AREA);
	if (_dest == NULL)
	{
		_dest = new QImage(frame.cols, frame.rows, QImage::Format_ARGB32);
	}

	_Mat2QImage(frame);
	_camImage->setPixmap(QPixmap::fromImage(*_dest));
}


/*
 * Function: SetZoom
 * Author: Ravi Kumar HMC '14
 * Description: This function is called upon initialization the connection from the 
 *				computer to the microscope camera. It can also be used to disconect.
 *				It also spawns a camera frame callback that is used for the UI.
 */
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

			QImage* _dest = NULL;

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


/*
 * Function: stageToggle
 * Author: Ravi Kumar HMC '14
 * Description: This function is called upon initialization to connect the computer
 *				controls to the microscope stage. It can also be used to disconect.
 */
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


/*
 * Function: SetZoom
 * Author: Justin Jones HMC '15
 * Description: This function is called to set the current zoom level information.
 *				Depending on the current optical zoom of the microscope, there are
 *				a different number of pixels. Most utility functions need to call
 *				this to work correctly. If the zoom is not set correctly then the
 *				function defaults to a value of 1 which is essentially nonsense.
 *				This function also updates the approximate slide size (in pixels) 
 *				when a zoom change occurs.
 *
 * Parameters: zoomX - this double is passed by reference and returns a conversion factor
 *					   between pixels and mm for the x direction to be used by utility functions
 *
 *			   zoomY - this double is passed by reference and returns a conversion factor
 *					   between pixels and mm for the y direction to be used by utility functions
 *
 *			   zoomChanged - this bool is set to false by default and is only passed by when the
 *							 zoom level changes. SetZoom will then update the slide size to the
 *							 approximate size of a slide at that zoom level.
 */
void MainWindow::SetZoom(double &zoomX, double &zoomY, bool zoomChanged)
{
	switch (_zoomLevel)
	{
	case 40: zoomX = ZOOM_40X_X; zoomY = ZOOM_40X_Y, _mmX = MM_40X_X, _mmY = MM_40X_Y; 
		if (zoomChanged)
		{
			_slideWidthEdit->setText("132000");
			_slideHeightEdit->setText("134000");
			_stageXSpeedEdit->setText(QString::number(0.075));
			_stageYSpeedEdit->setText(QString::number(0.05));
			_stageZIncrementEdit->setText(QString::number(0.001));
			XYSpeedEdit();
			ZIncrementEdit();
			_MAXDEPTH = 19;
			_prevDepth = _MAXDEPTH;
			_threshold = 30;
		}
		break;
	case 20: zoomX = ZOOM_20X_X; zoomY = ZOOM_20X_Y, _mmX = MM_20X_X, _mmY = MM_20X_Y;
		if (zoomChanged)
		{
			_slideWidthEdit->setText("85000");
			_slideHeightEdit->setText("87000");
			_stageXSpeedEdit->setText(QString::number(0.15));
			_stageYSpeedEdit->setText(QString::number(0.1));
			_stageZIncrementEdit->setText(QString::number(0.003));
			XYSpeedEdit();
			ZIncrementEdit();
			_MAXDEPTH = 15;
			_threshold = 22;
			_prevDepth = _MAXDEPTH;
		}
		break;
	case 10: zoomX = ZOOM_10X_X; zoomY = ZOOM_10X_Y, _mmX = MM_10X_X, _mmY = MM_10X_Y;
		if (zoomChanged)
		{
			_slideWidthEdit->setText("32000");
			_slideHeightEdit->setText("34000");
			_stageXSpeedEdit->setText(QString::number(0.45));
			_stageYSpeedEdit->setText(QString::number(0.30));
			_stageZIncrementEdit->setText(QString::number(0.01));
			XYSpeedEdit();
			ZIncrementEdit();
			_MAXDEPTH = 13;
			_threshold = 18;
			_prevDepth = _MAXDEPTH;
		}
		break;
	default: zoomX = 1; zoomY = 1;
	}
}


/*
 * Function: XYSpeedEdit
 * Author: Ravi Kumar HMC '14
 * Last Edit: Justin Jones HMC '15
 * Description: This function is called when either the X or Y speed is edited
 *				and updates the respective member variables for use in other
 *				functions that rely on stage speed information
 */
void MainWindow::XYSpeedEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageXSpeed = _stageXSpeedEdit->text().toDouble();
	_stageYSpeed = _stageYSpeedEdit->text().toDouble();
}


/*
* Function: ZIncrementEdit
* Author: Ravi Kumar HMC '14
* Description: This function is called when the z-axis speed is edited and
*				updates the impacted member variable for use in other
*				functions that rely on stage speed information
*/
void MainWindow::ZIncrementEdit()
{
	//Pull the text from the relevant lineedit and assign it to the variable! Ezpz
	_stageZIncrement = _stageZIncrementEdit->text().toDouble();
}


/*
 * Function: SlideSizeEdit
 * Author: Justin Jones HMC '15
 * Description: This function is called to parse the current slide size. Size
 *				is determined by two inputs. First, the program must determine
 *				the units. The user has a drop down option menu to choose between
 *				millimeters, pixels, and frames (such as a 5x5 capture). From the
 *				correct case, it reads from the line edit and converts the numbers
 *				to pixels which is what the program understands. This function
 *				is then able to update information for the progress bar. Right now
 *				the switch statement works because there are only three units.
 *				String->compare will declare if less than, equal to, or greater than.
 *				Taking advantage of this, I chose to compare to the word that is
 *				the alphabetic middle of the three.
 */
void MainWindow::SlideSizeEdit()
{
	std::string current = _currentUnits->currentText().toStdString();
	int compare = current.compare("Millimeters");
	int xEdit = _slideWidthEdit->text().toInt();
	int yEdit = _slideHeightEdit->text().toInt();
	double zoomX, zoomY;
	SetZoom(zoomX, zoomY);

	switch (compare)
	{ 
	case -1:	//Pixels
		_totalImageCount = xEdit * yEdit;
		_totalImageCount > 0 ? _totalImageCount : _totalImageCount = 1;
		_progressBar->setRange(0, _totalImageCount);

		_xPixels = (_stageXSpeed / _mmX) * _camImageDisplayWidth * xEdit - _stageError;
		_yPixels = (_stageYSpeed / _mmY) * _camImageDisplayHeight * yEdit - _stageError;
		break;
	case 0:		//Millimeters
		_xPixels = zoomX * _slideWidthEdit->text().toDouble();
		_yPixels = zoomY * _slideHeightEdit->text().toDouble();
		_totalImageCount = _xPixels / (int)((_stageXSpeed / _mmX) * _camImageDisplayWidth) + 1;
		_totalImageCount *= _yPixels / (int)((_stageYSpeed / _mmY) * _camImageDisplayHeight) + 1;
		_totalImageCount > 0 ? _totalImageCount : _totalImageCount = 1;
		_progressBar->setRange(0, _totalImageCount);
		break;
	case 1:		//Frames
		_xPixels = xEdit;
		_yPixels = yEdit;
		_totalImageCount = _xPixels / (int) ((_stageXSpeed / _mmX) * _camImageDisplayWidth) + 1;
		_totalImageCount *= _yPixels / (int) ((_stageYSpeed / _mmY) * _camImageDisplayHeight) + 1;
		_totalImageCount > 0 ? _totalImageCount : _totalImageCount = 1;
		_progressBar->setRange(0, _totalImageCount);
		break;
	}
}


/*
 * Function: ProgressUpdate
 * Author: Justin Jones HMC '15
 * Description: By taking advantage of QT's built in progress bar, this function
 *				only needs to tell the bar how many focused images have been 
 *				captured in order to update the UI.
 */
void MainWindow::ProgressUpdate()
{
	time(&_currentTime);
	_progressBar->setValue(_traverseCount);
	int timeToGo = (int)((difftime(_currentTime, _startTime) * _totalImageCount) / _traverseCount) - difftime(_currentTime, _startTime);
	std::string time;
	std::string seconds = std::to_string(timeToGo % 60);
	timeToGo /= 60;
	int minute = timeToGo % 60;
	std::string minutes;
	std::string hours;
	if (minute > 0)
	{
		minutes = std::to_string(minute);
		int hour = timeToGo % 60;
		if (hour > 0)
		{
			hours = std::to_string(hour);
			time.append(hours);
			time.append(":");
		}
		time.append(minutes);
		time.append(":");
	}
	time.append(seconds);
	_estTime->setText(time.c_str());

}


/*
 * Function: SeekPosition
 * Author: Justin Jones HMC '15
 * Description: This function will move the stage to a desired set of coordinates.
 *				The motivation behind this is to allow the geologists to take a
 *				set of coordinates that other algorithms deliver or that they write
 *				down and be able to return to that position for analysis. Possible
 *				future work allows this function to take input directly from a 
 *				crystal detection algorithm.
 */
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


/*
 * Function: SaveCurrentFrame
 * Author: Justin Jones HMC '15
 * Description: This function captures a single image of where the microscope is
 *				currently looking. This can be done through the "Take Picture" button.
 *				This function also serves as a support function to the traversal
 *				algorithm which must capture 1000s of single images.  
 */
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
	//if (_traverseCount > 0 || _zCount > 0)
	//	_previewCam();

	//Manage memory
	delete[] filename;
	delete[] position;
	delete[] folderPath;
}


/*
 * Function: FocusImage
 * Author: Justin Jones HMC '15
 * Description: This function calls the ImageJ library that takes a folder of images
 *				captured at a single X-Y location and compresses them into a single
 *				in focus image. The majority of this function is parsing the position
 *				into a string to send out with the system command call.
 */
void MainWindow::FocusImage()
{
	double zoomX;
	double zoomY;
	
	SetZoom(zoomX, zoomY);

	//Turn our current postion into a string for naming purposes
	double curX;
	double curY;
	_stage->where(curX, curY);
	int xPos = zoomX * (curX - _xOffset) / -MM_TO_STAGE_UNITS;
	int yPos = zoomY * (curY - _yOffset) / MM_TO_STAGE_UNITS;
	int zPos = _zPos * 1000;											//Currently using a magic number
	char* position = new char[72];										//Using the maximum possible int length
	itoa(xPos, position, 10 /*base 10*/);
	strncpy(&position[strlen(position)], "_\0", 2 /*2 characters*/);
	itoa(yPos, &position[strlen(position)], 10 /*base 10*/);

	//Start with a common shared portion of the cmd and concatenate the postion for naming
	char* commonCmd = "\"\"C:\\Users\\Ray Donelick\\Downloads\\ij148\\ImageJ\\ImageJ.exe\" -macro FocuserMacro.ijm ";
	int startPathLen = strlen(commonCmd);
	int posLen = strlen(position);
	char* fullCmd = new char[startPathLen + posLen + 2];
	strncpy(fullCmd, commonCmd, startPathLen);
	strncpy(fullCmd + startPathLen, position, strlen(position) + 1);	//Add postion argument for imagej macro
	strcpy(&fullCmd[strlen(fullCmd)], "\"\0");							//Add nullbyte to make the string readable

	int okay = std::system(fullCmd);									//Send call out to imagej
	if (!okay)															//Error checking
		std::cerr << okay << " Everything is not Okay!" << std::endl;

	delete[] position;
	delete[] fullCmd;
}


/*
 * Function: PythonCallout
 * Author: Justin Jones HMC '15
 * Description: This function is called at the end of a slide traversal.
 *				It takes all of the infocus images and turns them into a 
 *				single stitched image. This is done through a python image
 *				stitching solution.
 */
void MainWindow::PythonCallout()
{
	std::string cmd = "\"\"C:\\Users\\Ray Donelick\\Anaconda\\python.exe\" \"C:\\Users\\Ray Donelick\\Documents\\Ravi\\Apatite-to-Zircon\\Stitch\\image_stitch.py\" \"C:\\Users\\Ray Donelick\\Pictures\\Test Images\\Focused Images\\#C:\\Users\\Ray Donelick\\Pictures\\Test Images\\Stitched\\Slide.jpg\"\"";
	std::system(cmd.c_str());
}


/*
 * Function: FindSlideOrigin
 * Author: Justin Jones HMC '15
 * Description: This function calls the ImageJ library that takes a folder of images
 *				captured at a single X-Y location and compresses them into a single
 * 				in focus image. The majority of this function is parsing the position
 *				into a string to send out with the system command call.
 */
void MainWindow::FindSlideOrigin()
{
	_stage->where(_xOffset, _yOffset, _zOffset);
	_zPos = 0;
}


/*
 * Function: SlideTraversal
 * Author: Justin Jones HMC '15
 * Description: This function calls the ImageJ library that takes a folder of images
 *				captured at a single X-Y location and compresses them into a single
 * 				in focus image. The majority of this function is parsing the position
 *				into a string to send out with the system command call.
 */
void MainWindow::SlideTraversal()
{
	time(&_startTime);
	_traversalTimerID = startTimer(0);
	_zPos = 0;
}


/*
 * Function: UpdateZoom10
 * Author: Justin Jones HMC '15
 * Description: This function is connected to a UI button and lets the user update
 *				the current optical zoom.
 */
void MainWindow::UpdateZoom10()
{
	double x, y;
	_zoomLevel = ZOOM_10;
	SetZoom(x, y, true);
}


/*
* Function: UpdateZoom20
* Author: Justin Jones HMC '15
* Description: This function is connected to a UI button and lets the user update
*				the current optical zoom.
*/
void MainWindow::UpdateZoom20()
{
	double x, y;
	_zoomLevel = ZOOM_20;
	SetZoom(x, y, true);
}


/*
* Function: UpdateZoom40
* Author: Justin Jones HMC '15
* Description: This function is connected to a UI button and lets the user update
*				the current optical zoom.
*/
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
void MainWindow::_Mat2QImage(const cv::Mat3b src)
{
	for (int y = 0; y < src.rows; y++)
	{
		const cv::Vec3b *srcrow = src[y];
		QRgb *destrow = (QRgb*)(_dest->scanLine(y));

		for (int x = 0; x < src.cols; x++)
		{
			destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
		}
	}
}

/*void MainWindow::_Mat2QImage(const cv::Mat3b &src, QImage **dest)
{
	for (int y = 0; y < src.rows; y++)
	{
		const cv::Vec3b *srcrow = src[y];
		QRgb *destrow = (QRgb*)((*dest)->scanLine(y));

		for (int x = 0; x < src.cols; x++)
		{
			destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
		}
	}
}
*/

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