//virtual_stage.h
//Defines a virtual stage class! Emulates a stage/scope without needed to have one attached, using some neat data files

#pragma once

#pragma unmanaged

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#pragma managed

#include "stage_base.h"
#include "camera_base.h"

#include <string>
#include <vector>
#include <cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef SCOPECONTROL_EXPORTS
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport) 
#endif

#define SCOPE_WIDTH 720
#define SCOPE_HEIGHT 720

namespace sc
{
	class VirtualScope : public StageBase, public CameraBase
	{
	public:
		DLL_API VirtualScope(const std::string configFile);

		DLL_API bool getFrame(cv::Mat& out);

		DLL_API bool move(double X, double Y);
		DLL_API bool move(double X, double Y, double Z);
		DLL_API bool move(double Z);

		//Moves to a location and blocks until it has reached that location (i.e. the motors have stopped moving)
		DLL_API bool move_sync(double X, double Y);
		DLL_API bool move_sync(double X, double Y, double Z);
		DLL_API bool move_sync(double Z);

		//Gets the position of the stage
		DLL_API bool where(double& X, double& Y, double& Z);
		DLL_API bool where(double& X, double& Y);

		DLL_API void max_position(double& X, double& Y);
		DLL_API void min_position(double &x, double& Y);

		//Closes the connection to the stage
		DLL_API bool close();

	private:
		//Tile class
		class Tile
		{
		public:
			std::string _loc;

			/*Tile(std::string Loc, int Width, int Height)
				:_loc(Loc), _width(Width), _height(Height) {};*/
		};

		//Current position
		double _x;
		double _y;
		double _z;

		//Values about the scope/stage/objective
		double _micronsppx;
		double _micronsppy;

		//How big of an image we see at once (in pixels)
		const int _scopew = SCOPE_WIDTH;
		const int _scopeh = SCOPE_HEIGHT;

		//The grid of tiles!
		std::vector<std::vector<Tile>> _tiles;
		int _tilewidth;
		int _tileheight;
		
		static bool matOverlay(cv::Mat& base, const cv::Mat& overlay, int x, int y);

	};
}
