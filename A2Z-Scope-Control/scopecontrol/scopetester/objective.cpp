//objective.cpp
//defines the objective functions, of which we currently have one

#include "stdafx.h"

#include "objective.h"

Objective::Objective(double power_, bool reflected_)
	:power(power_), reflected(reflected_), xConv(0), yConv(0), zConv(0)
	{
		
	}

void Objective::convertTo(double& x, double& y, double& z)
{
	//Add the offset to each one
	x = x + xConv;
	y = y + yConv;
	z = z + zConv;

	//and we're done!
}