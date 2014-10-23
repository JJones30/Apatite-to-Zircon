//window for editing our objectives, saving the results, etc

#pragma once

#include "objective.h"
#include "addnewobjectivewidget.h"

#include <vector>

class EditObjectivesWidget : public QGroupBox
{
	Q_OBJECT

public:
	EditObjectivesWidget(std::vector<Objective> objectives);

signals:
	void objectivesUpdated(std::vector<Objective> newObjectives);

public slots:
	void showNewObjectivesWindow();
	void addNewObjective(double power);

private:
	AddNewObjectiveWidget* _newObjectiveWidget;

	std::vector<Objective> _objectives;

	int _objectiveAndLightingToIndex(int objectiveIndex, bool lighting);
};


