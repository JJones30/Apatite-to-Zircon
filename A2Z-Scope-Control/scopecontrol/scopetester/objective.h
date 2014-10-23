//objective class
//fairly simple, just keeps track of coordinate system conversions relative to the base objective

/*
Different objectives essentially have different offsets
so if you're looking at the point
	x=-14.74990
	y=-8.34804
	z=-.70930
And flip objectives, you'll be looking at a different point on the mount than before
So when the user flips objectives we want to automatically move so that they're looking at the same spot as before
More importantly, if the user notes a bunch of locations for later recording and then records in a different obj/lighting, we need to account for this or we record the wrong spots
*/

#pragma once

#include <string>

class Objective
{
public:
	Objective(double power_, bool reflected_);

	//objective properities
	double power; //magnification
	bool reflected; //or refracted

	//Distance off from base objective (probably 100x reflected)
	double xConv;
	double yConv;
	double zConv;

	//conversion function
	void convertTo(double& x, double& y, double& z);
	
};