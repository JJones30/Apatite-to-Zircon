#include "stdafx.h"

class AddNewObjectiveWidget : public QGroupBox
{
	Q_OBJECT

public:
	AddNewObjectiveWidget(QWidget* parent);

signals:
	void newObjective(double power);
};