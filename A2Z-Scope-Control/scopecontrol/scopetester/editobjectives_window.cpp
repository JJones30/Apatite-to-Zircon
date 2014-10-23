//objective editing window definitions

#include "stdafx.h"

#include "editobjectives_window.h"


EditObjectivesWidget::EditObjectivesWidget(std::vector<Objective> objectives)
	:_objectives(objectives)
{
	setWindowTitle("Edit Objectives");

	QGridLayout* editObjectivesBoxLayout = new QGridLayout;

	//Make a drop down for picking which objective to edit, copy it from the one on the main screen
	QComboBox* EditObjectivesList = new QComboBox;

	for (Objective& obj : _objectives)
	{
		QString objName = QString::number(obj.power) + "X";

		EditObjectivesList->addItem(objName);
	}

	QLabel* ObjectiveLabel = new QLabel("Objective");
	editObjectivesBoxLayout->addWidget(ObjectiveLabel, 0, 0);
	editObjectivesBoxLayout->addWidget(EditObjectivesList, 0, 1);

	QPushButton* addNewObjectiveButton = new QPushButton("Add new objective");
	connect(addNewObjectiveButton, SIGNAL(clicked()), this, SLOT(showNewObjectivesWindow()));
	editObjectivesBoxLayout->addWidget(addNewObjectiveButton, 0, 2);

	QCheckBox* reflectedCheck = new QCheckBox("Reflected light");
	editObjectivesBoxLayout->addWidget(reflectedCheck, 1, 0);

	//Add a lineedit for everything :(
	QLabel* xOffsetLabel = new QLabel("X Offset");
	QLabel* yOffsetLabel = new QLabel("Y Offset");
	QLabel* zOffsetLabel = new QLabel("Z Offset");

	QLineEdit* xOffsetEdit;
	QLineEdit* yOffsetEdit;
	QLineEdit* zOffsetEdit;

	if (_objectives.size() > 0)
	{
	Objective* currentObjective = &_objectives[_objectiveAndLightingToIndex(EditObjectivesList->currentIndex(), reflectedCheck->isChecked())];

	xOffsetEdit = new QLineEdit(QString::number(currentObjective->xConv));
	yOffsetEdit = new QLineEdit(QString::number(currentObjective->yConv));
	zOffsetEdit = new QLineEdit(QString::number(currentObjective->zConv));
	}
	else
	{
		xOffsetEdit = new QLineEdit();
		yOffsetEdit = new QLineEdit();
		zOffsetEdit = new QLineEdit();
	}

	editObjectivesBoxLayout->addWidget(xOffsetLabel, 2, 0);
	editObjectivesBoxLayout->addWidget(xOffsetEdit, 2, 1);
	editObjectivesBoxLayout->addWidget(yOffsetLabel, 3, 0);
	editObjectivesBoxLayout->addWidget(yOffsetEdit, 3, 1);
	editObjectivesBoxLayout->addWidget(zOffsetLabel, 4, 0);
	editObjectivesBoxLayout->addWidget(zOffsetEdit, 4, 1);

	QPushButton* saveButton = new QPushButton("Save");
	QPushButton* cancelButton = new QPushButton("Cancel");

	connect(saveButton, SIGNAL(clicked()), this, SIGNAL(objectivesUpdated(_objectives)));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

	setLayout(editObjectivesBoxLayout);
}

int EditObjectivesWidget::_objectiveAndLightingToIndex(int objectiveIndex, bool lighting)
{
	if (lighting) //if reflected, second set
	{
		return objectiveIndex + _objectives.size();
	}
	else
	{
		return objectiveIndex;
	}

}

void EditObjectivesWidget::showNewObjectivesWindow()
{
	_newObjectiveWidget = new AddNewObjectiveWidget(this);
	//connect(_newObjectiveWidget, )
	_newObjectiveWidget->show();
}



void EditObjectivesWidget::addNewObjective(double power)
{
	Objective newObjectiveRef(power, true);
	Objective newObjectiveTrans(power, false);
	
	_objectives.push_back(newObjectiveRef);
	_objectives.push_back(newObjectiveTrans);
}