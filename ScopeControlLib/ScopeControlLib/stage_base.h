//stage_base.h
//Defines a basic stage class, that all stages should derive from

#pragma once

namespace sc
{
	class StageBase
	{
	public: 
		virtual bool move(double X, double Y) { return false; };
		virtual bool move(double X, double Y, double Z) { return false; };
		virtual bool move(double Z) { return false; };

		//Moves to a location and blocks until it has reached that location (i.e. the motors have stopped moving)
		virtual bool move_sync(double X, double Y) { return false; };
		virtual bool move_sync(double X, double Y, double Z) { return false; };
		virtual bool move_sync(double Z)  { return false; };

		//Gets the position of the stage
		virtual bool where(double& X, double& Y, double& Z)  { return false; };
		virtual bool where(double& X, double& Y)  { return false; };

		//Closes the connection to the stage
		virtual bool close() { return false; };
	};
}
