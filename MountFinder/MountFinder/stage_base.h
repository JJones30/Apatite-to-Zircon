//stage_base.h
//Defines a basic stage class, that all stages should derive from

#pragma once

#pragma warning(disable: 4100)

namespace sc
{
	class StageBase
	{
	public: 
		//Tells the stage to move to a location and immediately returns
		//Units are in tenths of microns
		virtual bool move(double X, double Y) { return false; };
		virtual bool move(double X, double Y, double Z) { return false; };
		virtual bool move(double Z) { return false; };

		//Moves to a location and blocks until it has reached that location (i.e. the motors have stopped moving)
		//Units are in tenths of microns
		virtual bool move_sync(double X, double Y) { return false; };
		virtual bool move_sync(double X, double Y, double Z) { return false; };
		virtual bool move_sync(double Z)  { return false; };

		//Set the speed in a direction
		virtual bool set_speed_x(double X) { return false; };
		virtual bool set_speed_y(double Y) { return false; };
		virtual bool set_speed_z(double Z) { return false; };
		//DLL_API std::string send_command(const std::string& command, int Xon, double X, int Yon, double Y, int Zon, double Z);

		//Gets the position of the stage
		virtual bool where(double& X, double& Y, double& Z)  { return false; };
		virtual bool where(double& X, double& Y)  { return false; };
		virtual bool where(double& Z) { return false; };

		//Opens the connection to the stage
		virtual bool connect() { return false; };

		//Closes the connection to the stage
		virtual bool close() { return false; };

		virtual bool is_connected() { return false; };
	};
}
