//virtual_stage.h
//Defines a virtual stage class! Emulates a stage/scope without needed to have one attached, using some neat data files

#include "stdafx.h"

#include "virtual_stage.h"

#ifdef SCOPECONTROL_EXPORTS
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport) 
#endif

namespace sc
{
	//PUBLIC
	VirtualScope::VirtualScope(const std::string configFile)
		: _x(), _y(), _z()
	{
		using namespace std;
		//Read in the config file, do stuff with it
		//cout << "Making pt" << endl;
		//Read the config file into a tree
		boost::property_tree::ptree pt;

		//cout << "Reading config file" << endl;

		boost::property_tree::read_xml(configFile, pt);

		//cout << "getting micronsppx" << endl;

		_micronsppx = pt.get<double>("virtual_data.micronsppx");
		_micronsppy = pt.get<double>("virtual_data.micronsppy");
		_x = pt.get<double>("virtual_data.midx");
		_y = pt.get<double>("virtual_data.midy");

		cout << "X: " << _x << endl;

		

		_tilewidth = pt.get<int>("virtual_data.tilewidth");
		//cout << "Getting tile width: " << _tilewidth << endl;
		
		_tileheight = pt.get<int>("virtual_data.tileheight");
		//cout << "And height: " << _tileheight << endl;

		std::cout << "micronsppx: " << _micronsppx << std::endl;
		std::cout << "micronsppy: " << _micronsppy << std::endl;

		std::cout << "x: " << _x << std::endl;
		std::cout << "y: " << _y << std::endl;

		//Read in the images in each row into tiles		
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("virtual_data"))
		{
			//If we're on a row, loop through the row reading in images
			if (v.first == "row")
			{
				//New row, push it back onto tiles
				_tiles.push_back(vector<Tile>());

				//cout << "Read a row!" << endl;
				//cout << "type: " << v.first << endl;

				//Read each image in the row
				BOOST_FOREACH(boost::property_tree::ptree::value_type &w, v.second.get_child(""))
				{
					//cout << "Read an image: " << w.first << endl;
					Tile newtile;

					//Parse its properties into the tile
					newtile._loc = w.second.get<string>("loc");

					//Store the tile!
					_tiles.back().push_back(newtile);
				}
			}
			
		}
	}

	bool VirtualScope::getFrame(cv::Mat& out)
	{
		//std::cout << "Gettin' a frame" << std::endl;
		//Stich together the out images from our tiles

		//std::cout << "X" << _x << std::endl;
		
		//Compute the range of tiles we need
		size_t start_x = (size_t)(_x / _tilewidth);
		size_t end_x = (size_t)(_x / _tilewidth + (double)(_scopew) / _tilewidth - 1.0/_tilewidth);

		//std::cout << "Xposition, width: " << _x << ", " << _scopew << " spans tiles " << start_x << " to " << end_x << std::endl;

		size_t start_y = (size_t)(_y / _tileheight);
		size_t end_y = (size_t)(_y / _tileheight + (double)(_scopeh) / _tileheight - 1.0 / _tilewidth); //TODO: rethink this and make doubly sure that it's right

		//Initialize the out matrix
		out = cv::Mat(_scopew, _scopeh, CV_8UC3); //WORRY: does this mess up our memory?

		//Loop through relevant tiles and stitch!
		int xleft = _scopew;
		int currentX = _x;
		
		for (int x = start_x; x <= end_x; x++)
		{
			int width = 0;
			int currentY = _y;
			for (int y = start_y; y <= end_y; y++)
			{
				//std::cout << "iteration x, y: " << x << ", " << y << std::endl;

				//Read in the first mat
				cv::Mat tile = cv::imread(_tiles[x][y]._loc, CV_LOAD_IMAGE_COLOR); //load in color

				//Tile test
				/*cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", tile);
				cv::waitKey(0);*/

				using namespace std;

				//Compute subsection of tile
				int tilestartx = currentX % tile.cols;
				int tilewidth = std::min(tile.cols - tilestartx, (int)(out.cols - (currentX - _x)));

				int tilestarty = currentY % tile.rows;
				int tileheight = std::min(tile.rows - tilestarty, (int) (out.rows - (currentY - _y)));

				//Compute subsection of out image
				int outstartx = currentX - _x;
				int outwidth = tilewidth;
				int outstarty = currentY - _y;
				int outheight = tileheight;

				if (currentX == 18000)
				{
					cout << "tile.cols: " << tile.cols << endl;
					cout << "tilestartx: " << tilestartx << endl;
					cout << "currentX: " << currentX << endl;
					cout << "out.cols: " << out.cols << endl;
					cout << "_x: " << _x << endl;
					cout << "outwidth: " << outwidth << endl;

					cout << "Start: " << start_x << endl;
					cout << "End: " << end_x << endl;
				}

				/*xleft = _scopew - currentPosX;

				yleft = _scopeh - currentPosY;

				std::cout << "currentx, currenty: " << currentPosX << ", " << currentPosY << std::endl;
				std::cout << "xleft, yleft: " << xleft << ", " << yleft << std::endl;

				int addedHeight = std::min(yleft, tile.rows) - currentY;*/

				//std::cout << "tilestartx, width, y, height: " << tilestartx << ", " << tilewidth << ", " << tilestarty << ", " << tileheight << std::endl;

				//Compute the subsection we are copying from and to
				cv::Mat tileTmp = tile(cv::Rect(tilestartx, tilestarty, tilewidth, tileheight)); //TODO: Do these always start at 0?
				//std::cout << "tiletmp size: " << tileTmp.cols << ", " << tileTmp.rows << std::endl;

				/*cout << "Tiletmp display" << endl;
				cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", tileTmp);
				cv::waitKey(0);*/

				//std::cout << "outstartx, width, y, height: " << outstartx << ", " << outwidth << ", " << outstarty << ", " << outheight << std::endl;
				cv::Mat outTmp = out(cv::Rect(outstartx, outstarty, outwidth, outheight));
				
				/*cout << "OUTTEMP display" << endl;
				cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", outTmp);
				cv::waitKey(0);*/
				
				//std::cout << "outtmp size: " << outTmp.cols << ", " << outTmp.rows << std::endl;

				//std::cout << " copying" << std::endl;

				//Copy it!
				tileTmp.copyTo(out(cv::Rect(outstartx, outstarty, outwidth, outheight)));

				/*cout << "Tiletmp display" << endl;
				cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", tileTmp);
				cv::waitKey(0);

				cout << "COPY display" << endl;
				cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", outTmp);
				cv::waitKey(0);*/

				/*cout << "ORIGINAL display" << endl;
				cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
				cv::imshow("Frame Display", out);
				cv::waitKey(0);*/

				//Update currentY
				currentY += tileheight;
				width = tilewidth;
			}
			currentX += width;
		}

		return true;
	}

	bool VirtualScope::move(double X, double Y)
	{
		_x = X;
		_y = Y;
		return true;
	}
	bool VirtualScope::move(double X, double Y, double Z)
	{
		_x = X;
		_y = Y;
		_z = Z;
		return true;
	}
	bool VirtualScope::move(double Z)
	{
		
		_z = Z;
		return true;
	}

	//Moves to a location and blocks until it has reached that location (i.e. the motors have stopped moving)
	bool VirtualScope::move_sync(double X, double Y)
	{
		return move(X, Y);
	}
	bool VirtualScope::move_sync(double X, double Y, double Z)
	{
		return move(X, Y, Z);
	}
	bool VirtualScope::move_sync(double Z)
	{
		return move(Z);
	}

	//Gets the position of the stage
	bool VirtualScope::where(double& X, double& Y, double& Z)
	{
		X = _x;
		Y = _y;
		Z = _z;
		return true;
	}
	bool VirtualScope::where(double& X, double& Y)
	{
		X = _x;
		Y = _y;
		return true;
	}

	//Closes the connection to the stage
	bool VirtualScope::close()
	{
		return true;
	}

	void VirtualScope::max_position(double& X, double& Y)
	{
		X = _tiles.size() * _tilewidth - _scopew - 1;
		Y = _tiles[0].size() * _tileheight - _scopeh - 1;
	}

	void VirtualScope::min_position(double& X, double& Y)
	{
		X = 0;
		Y = 0;
	}

	//PRIVATE

	bool VirtualScope::matOverlay(cv::Mat& base, const cv::Mat& overlay, int x, int y)
	{
		std::cout << "overlay size: " << overlay.cols << ", " << overlay.rows << std::endl;
		for (int i = 0; i < overlay.cols; i++)
		{
			for (int j = 0; j < overlay.rows; j++)
			{
				/*std::cout << "overlay size: " << overlay.cols << ", " << overlay.rows << std::endl;
				std::cout << "base size: " << base.cols << ", " << base.rows << std::endl;
				std::cout << "I, J: " << i << ", " << j << std::endl;
				std::cout << "overlay: " << (int) overlay.at<unsigned char>(i, j) << std::endl;
				std::cout << "base: " << (int) base.at<unsigned char>(i + x, j + y) << std::endl;*/
				base.at<unsigned char>(i + x, j + y) = overlay.at<unsigned char>(i, j);
			}
		}
		return true;
	}
}
