// scopecontrol.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <msclr/marshal.h>

#include "stagecontroller.h"
//#include <stdexcept>


namespace sc
{
	//-------------------------------------------------
	//PUBLIC
	//-------------------------------------------------

	//Channel should be something like "COM3" on windows (if that doesn't work check the device manager for which port its on)
	//SCOPETYPE is the type of the scope, as an enum of type SCOPETYPE (sc::SCOPETYPE)
	//configFilename should be the full name (and path if not in the same folder) of the config file
	StageController::StageController(const std::string& channel, STAGE_TYPE type, const std::string& configFilename) :
		_io(), _stage(_io), _stagetype(type), stageLock()
	{
		//std::cout << "Entered constructor" << std::endl;

		//Create variables for all settings
		int baud_rate;
		int flow;
		int parity;
		int stop_bits;
		int char_size;

		//Read in the settings from a text file
		std::string typeString;
		std::string valString;
		std::ifstream configFile;
		configFile.open(configFilename, std::ifstream::in);

		//Make sure we've successfully opened it
		if (configFile)
		{
			for (int c = 0; c < NUM_CONFIGS; c++)
			{
				std::getline(configFile, typeString, ','); //get everything up to the first comma

				//std::cout << "Got line: " << typeString << std::endl;

				//Figure out which var it is, read in the value, store it
				if (typeString == "baud rate")
				{
					std::getline(configFile, valString); //get the value for the string
					baud_rate = atoi(valString.c_str());
				}
				else if (typeString == "flow")
				{
					std::getline(configFile, valString); //get the value for the string
					flow = flow_from_str(valString);
				}
				else if (typeString == "parity")
				{
					std::getline(configFile, valString);
					parity = parity_from_str(valString);
				}
				else if (typeString == "stop bits")
				{
					std::getline(configFile, valString);
					stop_bits = stop_bits_from_str(valString);
				}
				else if (typeString == "char size")
				{
					std::getline(configFile, valString); //get the value for the string
					char_size = atoi(valString.c_str());
#					ifdef SCOPECONTROL_DEBUG
						std::cout << "Char size: " << valString << std::endl;
#					endif
				}
			}
		}
		else
		{
#			ifdef SCOPECONTROL_DEBUG
				std::cout << "Failed to open configuration file!" << std::endl;
#			endif

			throw SCException("Failed to open configuration file!");
		}

		_channel = channel;
		_stagetype = type;
		_baud_rate = baud_rate;
		_flow = flow;
		_parity = parity;
		_stop_bits = stop_bits;
		_char_size = char_size; //TODO: Fix this crap

		/*if (!initialize_stage(channel, type, baud_rate, flow, parity, stop_bits, char_size))
		{
#			ifdef SCOPECONTROL_DEBUG
				std::cout << "Failed to initialize stage!" << std::endl;
#			endif
			throw SCException("Error connecting to stage!");
		}
		
		_is_connected = true;*/
	}

	StageController::StageController(const std::string& channel, STAGE_TYPE type, int baud_rate, const std::string& flow, const std::string& parity, double stop_bits, int char_size)
		:_io(), _stage(_io), _stagetype(type), stageLock(),
		_channel(channel), _baud_rate(baud_rate), _flow(flow), _parity(parity), _stop_bits(stop_bits), _char_size(char_size)
	{
		int int_flow = flow_from_str(flow);
		int int_parity = parity_from_str(parity);
		int int_stop_bits = stop_bits_from_double(stop_bits);

		/*if (!initialize_stage(channel, type, baud_rate, int_flow, int_parity, int_stop_bits, char_size))
		{
			throw "Error connecting to stage!";
		}

		_is_connected = true;*/
	}

	bool StageController::connect()
	{
		if (initialize_stage(_channel,
			_stagetype,
			_baud_rate,
			flow_from_str(_flow),
			parity_from_str(_parity),
			stop_bits_from_double(_stop_bits),
			_char_size))
		{
			return true;
		}

		return false;
	}

	//Send any command to the stage. Waits for a response from the stage and returns it.
	//Send a command without any parameters
	std::string StageController::send_command(const std::string& command)
	{
		return send_command(command, 0, 0, 0, 0, 0, 0);
	}
	//Send a command with one input, X
	std::string StageController::send_command(const std::string& command, double X)
	{
		return send_command(command, 2, X, 0, 0, 0, 0);
	}
	//Send a command with two inputs, X and Y
	std::string StageController::send_command(const std::string& command, double X, double Y)
	{
		return send_command(command, 2, X, 2, Y, 0, 0);
	}
	//Send a command with three inputs
	std::string StageController::send_command(const std::string& command, double X, double Y, double Z)
	{
		return send_command(command, 2, X, 2, Y, 2, Z);
	}

	//Send a general command. X Y and Z are controlled by Xon, Yon, Zon as follows:
	//	If Xon is 0, X won't be included in the command.
	//	If Xon is 1, X will be included but without a value (either for assumed values or queries)
	//	If Xon is 2, X will be included with a value (for assignments)
	std::string StageController::send_command(const std::string& command, int Xon, double X, int Yon, double Y, int Zon, double Z)
	{
		//First check if we're really connected
		if (!is_connected())
			return "OPERATION FAILED";

		//Generate the command
		std::string send_string = command;
		
		//Generate X
		if (Xon == 1)
			send_string += " X";
		else if (Xon == 2)
			send_string += " X=" + std::to_string(X);

		//Generate Y
		if (Yon == 1)
			send_string += " Y";
		else if (Yon == 2)
			send_string += " Y=" + std::to_string(Y);

		//Generate Z
		if (Zon == 1)
			send_string += " Z";
		else if (Zon == 2)
			send_string += " Z=" + std::to_string(Z);

		//Lock the mutex so that we never send concurrent commands!
		stageLock.lock();

		//Send the command to the stage
		this->_stage.write_some(boost::asio::buffer(send_string + "\r")); //ASI commands end with \r. TODO: OTHERS MIGHT NOT

		//Wait for the response
		std::string result = get_response();

		//Unlock because we're done interacting with the stage
		stageLock.unlock();

		return result;
	}

	//Gets a response from the stage.
	std::string StageController::get_response()
	{
		//Gets a line of text ending with a \n, and strips \r and \n
		//Currently gets one char at a time, which could be way more efficient TODO
		
		//Get the line from the serial
		char c = ' ';
		std::string result;
		for (; c != '\n';)
		{
			this->_stage.read_some(boost::asio::buffer(&c, 1)); //we want to get :A\r\n
			//std::cout << "Read in: " << int(c) << std::endl;
			result += c;
		}

		//Strip off newline chars
		boost::algorithm::trim_right_if(result, is_space);

		//Return the resulting good string!
		return result;
	}
	
	//Gets the position when you only care about X and Y
	bool StageController::where(double& X, double& Y)
	{
		//Send the command WHERE X Y
		std::string response = send_command("WHERE", 1, 0, 1, 0, 0, 0);

		//Parse out the values we want
		auto parsed = parse_response(response);

		//Make sure we got a valid response
		if (parsed[0] != ":A")
			return false;

		assert(parsed.size() == 3);

		//Set them up into the return variables
		X = atof(parsed[1].c_str());
		Y = atof(parsed[2].c_str());
		
		return true;
	}

	//Get the position of the stage
	bool StageController::where(double& X, double& Y, double& Z)
	{
		//Send the command WHERE X Y Z
		std::string response = send_command("WHERE", 1, 0, 1, 0, 1, 0); 

		//Parse out the values we want
		auto parsed = parse_response(response);

		//Make sure we got a valid response
		if (parsed[0] != ":A")
			return false;

		assert(parsed.size() == 4);

		//Set them up into the return variables
		X = atof(parsed[1].c_str());
		Y = atof(parsed[2].c_str());
		Z = atof(parsed[3].c_str());

		return true;
	}

	//Gets the position when you only care about X and Y
	bool StageController::where(double& Z)
	{
		//Send the command WHERE Z
		std::string response = send_command("WHERE", 0, 0, 0, 0, 1, 0);

		//Parse out the values we want
		auto parsed = parse_response(response);

		//Make sure we got a valid response
		if (parsed[0] != ":A")
			return false;

		assert(parsed.size() == 2);

		//Set them up into the return variables
		Z = atof(parsed[1].c_str());

		return true;
	}

	bool StageController::is_busy()
	{
		std::string response = send_command("/");
#ifdef  SCOPECONTROL_DEBUG
			std::cout << "Got status response: " << response << std::endl;
#endif // SCOPECONTROL_DEBUG

		return response == "B";
	}

	//Move to a position in the X Y plane
	bool StageController::move(double X, double Y)
	{
		std::string result = send_command("MOVE", X, Y);

		#ifdef SCOPECONTROL_DEBUG
			std::cout << "Got result from move command: " << result << std::endl;
		#endif

		if (result == ":A")
			return true;
		return false;
	}

	//Move to a position in the X Y Z plane
	bool StageController::move(double X, double Y, double Z)
	{
		std::string result = send_command("MOVE", X, Y, Z);

		if (result == ":A")
			return true;
		return false;
	}

	//Move to a position in the X Y Z plane
	bool StageController::move(double Z)
	{
		std::string result = send_command("MOVE", 0, 0, 0, 0, 2, Z);

		if (result == ":A")
			return true;
		return false;
	}

	//Move to a position in the X Y plane and wait for the stage to get there before returning.
	bool StageController::move_sync(double X, double Y)
	{
		//First move to specified position
		move(X, Y);

		//Wait for it to not be busy
		while (is_busy()) { 1; }

		return true;
	}

	//Move to a position in the X Y plane and wait for the stage to get there before returning.
	bool StageController::move_sync(double X, double Y, double Z)
	{
		//First move to specified position
		move(X, Y, Z);

		//Wait for it to not be busy
		while (is_busy()) { 1; }

		return true;
	}

	//Move to a position in the X Y plane and wait for the stage to get there before returning.
	bool StageController::move_sync(double Z)
	{
		//First move to specified position
		move(Z);

		//Wait for it to not be busy
		while (is_busy()) { 1; }

		return true;
	}

	bool StageController::set_speed_x(double X)
	{
		//First move to specified position
		std::string result = send_command("VE", 2, X, 0, 0, 0, 0); //short for VECTOR

		if (result == ":A")
			return true;
		return false;
	}

	bool StageController::set_speed_y(double Y)
	{
		//First move to specified position
		std::string result = send_command("VE", 0, 0, 2, Y, 0, 0); //short for VECTOR

		if (result == ":A")
			return true;
		return false;
	}

	bool StageController::set_speed_z(double Z)
	{
		//First move to specified position
		std::string result = send_command("VE", 0, 0, 0, 0, 2, Z); //short for VECTOR

		if (result == ":A")
			return true;
		return false;
	}

	//Closes the connection to the stage
	bool StageController::close()
	{
		_stage.close();

		//zero out relevant variables
		//There aren't any!

		return true;
	}

	bool StageController::is_connected()
	{
		return _stage.is_open();
	}


	std::vector<std::string> StageController::parse_response(std::string response)
	{
		std::vector<std::string> splitVec;
		boost::split(splitVec, response, boost::algorithm::is_space(), boost::token_compress_on); //split on spaces
		return splitVec;
	}


	//-------------------------------------------------
	//PRIVATE
	//-------------------------------------------------


	bool StageController::runWinPort(const std::string& channel, STAGE_TYPE type_, int baud_rate, int flow, int parity, int stop_bits, int char_size)
	{

		//Initialize the windows settings from inputs
		System::IO::Ports::Parity winParity;
		if (parity == 0) //None
			winParity = System::IO::Ports::Parity::None;
		else if (parity == 1)
			winParity = System::IO::Ports::Parity::Odd;
		else if (parity == 2)
			winParity = System::IO::Ports::Parity::Even;

		System::IO::Ports::StopBits winStopBits;
		if (stop_bits == 0) //1.5
			winStopBits = System::IO::Ports::StopBits::OnePointFive;
		else if (stop_bits == 1) //1.5
			winStopBits = System::IO::Ports::StopBits::One;
		else if (stop_bits == 2) //1.5
			winStopBits = System::IO::Ports::StopBits::Two;

		//Connect via System.IO.Ports.SerialPort
		System::IO::Ports::SerialPort winPort("COM3", baud_rate, winParity, char_size, winStopBits);
		winPort.ReadTimeout = 500;
		winPort.NewLine = "\r";
		winPort.Open();
		if (!winPort.IsOpen)
		{
			return false;
		}

		//Deeper check because you can open serial ports to powered down stage controllers
		winPort.WriteLine("/"); //ask it where
		try
		{
			winPort.NewLine = "\r\n";
			winPort.ReadLine();
		}
		catch (System::TimeoutException^)
		{
			winPort.Close();
			return false; //we timed out
		}

		winPort.Close();
		return true;
	}

	bool StageController::initialize_stage(const std::string& channel, STAGE_TYPE type_, int baud_rate, int flow, int parity, int stop_bits, int char_size)
	{
		//First connect using windows to properly set up the serial port
		//If we don't do this, then after a fresh boot boost asio won't be able to connect, couldn't figure out why
		if (!runWinPort(channel, type_, baud_rate, flow, parity, stop_bits, char_size))
		{
			std::cerr << "Error initializing connection with windows port!" << std::endl;
			return false;
		}

		//Now we do the real connection stuff
		using namespace boost;

		//generate proper variables from those inputs
		asio::serial_port_base::baud_rate asio_baud_rate(baud_rate);
		asio::serial_port_base::character_size asio_char_size(char_size);

		auto asio_parity = asio::serial_port_base::parity::none;
		if (parity == 0) //None
			asio_parity = asio::serial_port_base::parity::none;
		else if (parity == 1)
			asio_parity = asio::serial_port_base::parity::odd;
		else if (parity == 2)
			asio_parity = asio::serial_port_base::parity::even;

		auto asio_stop_bits = asio::serial_port_base::stop_bits::one;
		if (stop_bits == 0) //1.5
			asio_stop_bits = asio::serial_port_base::stop_bits::onepointfive;
		else if (stop_bits == 1) //1.5
			asio_stop_bits = asio::serial_port_base::stop_bits::one;
		else if (stop_bits == 2) //1.5
			asio_stop_bits = asio::serial_port_base::stop_bits::two;

		auto asio_flow = asio::serial_port_base::flow_control::none;
		if (flow == 0) //1.5
			asio_flow = asio::serial_port_base::flow_control::none;
		else if (flow == 1) //1.5
			asio_flow = asio::serial_port_base::flow_control::hardware;
		else if (flow == 2) //1.5
			asio_flow = asio::serial_port_base::flow_control::software;


		#ifdef SCOPECONTROL_DEBUG
			std::cout << "Is port already open? " << _stage.is_open() << std::endl;
			std::cout << "Opening connection to stage" << std::endl;
		#endif

		//Open the connection and configure it
		system::error_code error;
		_stage.open(channel, error);

		//Make sure that it's actually open
		if (error)
		{
			std::cout << "Error when connecting to stage: " << system::system_error(error).what() << std::endl;
			return false;
		}

		#ifdef SCOPECONTROL_DEBUG
			std::cout << "Port is now open: " << _stage.is_open() << std::endl;
			std::cout << "Opened, setting up settings" << std::endl;
		#endif

		_stage.set_option(asio_baud_rate);
		_stage.set_option(asio::serial_port_base::flow_control(asio_flow));
		_stage.set_option(asio::serial_port_base::parity(asio_parity));
		_stage.set_option(asio::serial_port_base::stop_bits(asio_stop_bits));
		_stage.set_option(asio_char_size);

		#ifdef SCOPECONTROL_DEBUG
			std::cout << "Done initializing" << std::endl;
		#endif

		//Done and ready to take commands!
			return true;
	}

	//Space characters to be stripped
	bool StageController::is_space(char c)
	{
		if (c == ' ' || c == '\r' || c == '\n')
			return true;
		return false;
	}

	//Gets the time since an event
	long StageController::time_since(boost::posix_time::ptime start_time)
	{
		boost::posix_time::ptime now = boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time();
		boost::posix_time::time_duration diff = now - start_time;
		return (long) diff.total_milliseconds();
	}

	int StageController::flow_from_str(std::string flow)
	{
		if (flow == "hardware")
			return 1;
		else if (flow == "software")
			return 2;

		return 0; //no flow control
	}

	int StageController::parity_from_str(std::string parity)
	{
		if (parity == "odd")
			return 1;
		else if (parity == "even")
			return 2;

		return 0; //no parity
	}

	int StageController::stop_bits_from_str(std::string stop_bits)
	{
		double d_stop_bits = atof(stop_bits.c_str());
		return stop_bits_from_double(d_stop_bits);
	}

	int StageController::stop_bits_from_double(double stop_bits)
	{
		if (stop_bits == 1.0)
			return 1;
		else if (stop_bits == 2.0)
			return 2;

		return 0; //1.5
	}


	//------------------------------------------------
	//C INTERFACE
	//------------------------------------------------

	

}



int32_t zero_test(int32_t test)
{
	using namespace sc;
	//Make a stage controller
	StageController stage("COM3", ASI_MS2000, "C:\\Users\\Ray Donelick\\Documents\\Visual Studio 2013\\Projects\\scopecontrol\\Debug\\asi_ms2000_config.txt");
	stage.move(0, 0);
	stage.close();
	return 1;
}

int32_t move(int32_t x, int32_t y, CONSTRUCTOR_INPUTS)
{
	using namespace sc;
	//Make a stage controller
	StageController stage("COM3", ASI_MS2000, baud_rate, flow, parity, stop_bits, char_size);
	stage.move(x, y);
	stage.close();
	return 1;
}