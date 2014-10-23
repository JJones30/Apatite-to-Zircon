// scopecontrol.h

#pragma once


#include "stdafx.h"



#pragma unmanaged
#include <boost/asio/basic_serial_port.hpp>
#include <boost/algorithm/string/trim.hpp> //string trimming
#include <boost/algorithm/string/split.hpp> //String parsing for responses from the stage

//For timeouts
#include <boost/date_time/posix_time/date_duration_operators.hpp>
#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

//For concurrency/locking
#include<boost/thread/mutex.hpp>

#pragma managed
#include "stdafx.h"
#include <vector>
#include <cassert>
#include <fstream> //reading in config files
#include "stage_base.h"
#using <System.dll> //Gives us system.io.ports.serial port for initializing connection

#define NUM_CONFIGS 5
//#define SCOPECONTROL_DEBUG true
#define DEFAULT_MOVESYNC_TIMEOUT 5000
#define DEFAULT_MOVESYNC_TOLERANCE 1
#define CONSTRUCTOR_INPUTS int baud_rate, const std::string& flow, const std::string& parity, double stop_bits, int char_size

#ifdef SCOPECONTROL_DEBUG
#include <iostream>
#endif

namespace sc
{
	enum STAGE_TYPE {ASI_MS2000, LAURIN_TECHNIC, LUDL_KINETIC};

	class SCException
	{
	public:
		SCException(std::string reason) : _reason(reason) {};
		~SCException(){};
		const char *ShowReason() const { return _reason.c_str(); }

	private:
		std::string _reason;
	};

	// This class is exported from the scopecontrol.dll
	class StageController : public StageBase
	{
	public:

		//Constructors

		//Connects to the specified camera/scope on the specified channel, reading in the connection information from a file
		DLL_API StageController(const std::string& channel, STAGE_TYPE type, const std::string& configFilename);

		//Connects to the specified stage with the given connection info
		DLL_API StageController(const std::string& channel, STAGE_TYPE type, int baud_rate, const std::string& flow, const std::string& parity, double stop_bits, int char_size);


		//Checks if the stage is still busy after a move command
		DLL_API bool is_busy();

		//Sends the command to move to a location
		//Returns upon receiving acknowledgment of that command, not when the stage actually reaches that position
		DLL_API bool move(double X, double Y);
		DLL_API bool move(double X, double Y, double Z);
		DLL_API bool move(double Z);

		//Moves to a location and blocks until it has reached that location (i.e. the motors have stopped moving)
		DLL_API bool move_sync(double X, double Y);
		DLL_API bool move_sync(double X, double Y, double Z);
		DLL_API bool move_sync(double Z);

		//Set the speed in a direction
		DLL_API bool set_speed_x(double X);
		DLL_API bool set_speed_y(double Y);
		DLL_API bool set_speed_z(double Z);

		//Gets the position of the stage
		DLL_API bool where(double& X, double& Y, double& Z);
		DLL_API bool where(double& X, double& Y);
		DLL_API bool where(double& Z);

		//For any general commands that don't have their own function
		DLL_API std::string send_command(const std::string& command);
		DLL_API std::string send_command(const std::string& command, double X);
		DLL_API std::string send_command(const std::string& command, double X, double Y);
		DLL_API std::string send_command(const std::string& command, double X, double Y, double Z);

		//Any general send_command
		DLL_API std::string send_command(const std::string& command, int Xon, double X, int Yon, double Y, int Zon, double Z);

		//Get one line of buffered text from the stage
		//Note: get_response does not perform any locking! It is up to the calling function to ensure that correctness is maintained during concurrent operations.
		DLL_API std::string get_response();

		//Given a response string (of the form ":A 1 1.5 3" for example) will parse it into a vector of strings.
		DLL_API static std::vector<std::string> parse_response(std::string);

		//Connects to the scope
		DLL_API bool connect();

		//closes the connection to the scope
		DLL_API bool close();

		DLL_API bool is_connected();

	private:
		boost::asio::io_service _io;
		boost::asio::basic_serial_port<boost::asio::serial_port_service> _stage;
		STAGE_TYPE _stagetype;
		boost::mutex stageLock;

		std::string _channel;
		int _baud_rate;
		std::string _flow;
		std::string _parity;
		double _stop_bits;
		int _char_size;

		//Uses built-in windows port structures to open and close the port, just to get the initial settings right
		//Only needs to be done once per boot, but since we have no way of detecting his just do it on construction
		bool StageController::runWinPort(const std::string& channel, STAGE_TYPE type_, int baud_rate, int flow, int parity, int stop_bits, int char_size);

		//Initializes the connection to stage, and sets up the variables correctly
		bool initialize_stage(const std::string& channel, STAGE_TYPE type_, int baud_rate, int flow, int parity, int stop_bits, int char_size);

		//Checks if a character is a space, \n, or \r
		static bool is_space(char c);

		//Time since a start time, for timeout related stuff
		long time_since(boost::posix_time::ptime start_time);

		//Converters from strings to various flow/parity/stop bits types
		static int flow_from_str(std::string flow);
		static int parity_from_str(std::string parity);
		static int stop_bits_from_str(std::string stop_bits);
		static int stop_bits_from_double(double stop_bits);
	};

	
}


//C interface to labview
//labview doesn't really do namespaces as far as I can tell,
//hence stuff down here


LVEXP DLL_API int zero_test(int test);
LVEXP DLL_API int32_t move(int32_t x, int32_t y, CONSTRUCTOR_INPUTS);
