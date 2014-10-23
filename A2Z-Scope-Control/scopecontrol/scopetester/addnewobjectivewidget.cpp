
#include "stdafx.h"

#include "addnewobjectivewidget.h"

AddNewObjectiveWidget::AddNewObjectiveWidget(QWidget* parent)
{
	setWindowTitle("Add new objective");

	QGridLayout* addNewObjectivesBoxLayout = new QGridLayout;

	QLabel* objectiveLabel = new QLabel("Magnification");
	QLineEdit* objectivePower = new QLineEdit;
	objectivePower->setValidator(new QDoubleValidator(0, 10000, 2, this));

	addNewObjectivesBoxLayout->addWidget(objectiveLabel, 0, 0);
	addNewObjectivesBoxLayout->addWidget(objectivePower, 0, 1);

	QPushButton* saveButton = new QPushButton("Save");
	QPushButton* cancelButton = new QPushButton("Cancel");

	addNewObjectivesBoxLayout->addWidget(saveButton, 1, 0);
	addNewObjectivesBoxLayout->addWidget(cancelButton, 1, 1);

	connect(saveButton, SIGNAL(clicked()), parent, SIGNAL(newObjective(QString::toDouble(objectivePower->text()))));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	//connect(saveButton, SGINAL(clicked()), this, SLOT(close()));

	setLayout(addNewObjectivesBoxLayout);
}