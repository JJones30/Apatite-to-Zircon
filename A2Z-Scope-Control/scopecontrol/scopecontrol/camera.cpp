// scopecontrol.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "camera.h"
//#include <stdexcept>


namespace sc
{
	//-------------------------------------------------
	//PUBLIC
	//-------------------------------------------------

	LumeneraCamera::LumeneraCamera(int cameraNum)
		:_cameraNum(cameraNum), _isConnected(false)
	{
		//Do nothing! Woo
	}


	bool LumeneraCamera::connect()
	{
		_hCam = LucamCameraOpen(_cameraNum);

		if (_hCam != NULL)
		{
			//Set local object settings
			_isConnected = true;


			if (!LucamGetFormat(_hCam, &_lffFormat, &_videoFrameRate))
			{
				std::cerr << "Unable to get camera video format. Snapshot mode disabled." << std::endl;
				return false;
			}

			std::cout << "Frame rate: " << _videoFrameRate << std::endl;

			//Lucam settings
			if (!_getCamInfo())
			{
				std::cerr << "Unable to get connection info from camera! Connection failed." << std::endl;
				return false;
			}

			//TODO Setup all the settings or load a settings file. Look more at colormonocapturedlg lines ~300

			//All our class settings


		}

		return _isConnected;
	}

	bool LumeneraCamera::disconnect()
	{
		if (!LucamCameraClose(_hCam))
		{
			std::cerr << "Unable to disconnect to the camera." << std::endl;
			return false;
		}
		else
		{
			_isConnected = false;
			_hCam = NULL;
			_lffFormat.height = 0;
			_lffFormat.width = 0;

			_previewCallbackRegNumber = -1;

			return true;
		}
	}


	bool LumeneraCamera::_getCamInfo()
	{
		FLOAT fValue;
		LONG lFlags;

		if (!LucamGetProperty(_hCam, LUCAM_PROP_EXPOSURE, &fValue, &lFlags))
		{
			std::cerr << "Unable to get properties 1 from camera." << std::endl;
			return false;
		}
		_lsSettings.exposure = fValue;
		if (!LucamGetProperty(_hCam, LUCAM_PROP_GAIN, &fValue, &lFlags))
		{
			std::cerr << "Unable to get properties 2 from camera." << std::endl;
			return false;
		}
		_lsSettings.gain = fValue;
		if (!LucamGetProperty(_hCam, LUCAM_PROP_GAIN_RED, &fValue, &lFlags))
		{
			std::cerr << "Unable to get properties 3 from camera." << std::endl;
			return false;
		}
		_lsSettings.gainRed = fValue;
		if (!LucamGetProperty(_hCam, LUCAM_PROP_GAIN_BLUE, &fValue, &lFlags))
		{
			std::cerr << "Unable to get properties 4 from camera." << std::endl;
			return false;
		}
		_lsSettings.gainBlue = fValue;

		/*if ((m_ulCameraId != 0x097) && (m_ulCameraId != 0x092))
		{
		if (!LucamGetProperty(_hCam, LUCAM_PROP_GAIN_GREEN1, &fValue, &lFlags))
		{
		cout << "Unable to get properties 5 from camera." << endl;
		return false;
		}
		m_lsSettings.gainGrn1 = fValue;
		if (!LucamGetProperty(_hCam, LUCAM_PROP_GAIN_GREEN2, &fValue, &lFlags))
		{
		cout << "Unable to get properties 6 from camera." << endl;
		return false;
		}
		m_lsSettings.gainGrn2 = fValue;
		}
		else
		{*/

		_lsSettings.gainGrn1 = 1.0;
		_lsSettings.gainGrn2 = 1.0;
		//}
		_lsSettings.useStrobe = FALSE;
		_lsSettings.strobeDelay = 0;
		if (!LucamGetFormat(_hCam, &_lffFormat, &fValue))
		{
			std::cerr << "Unable to get camera video format. Snapshot mode disabled." << std::endl;
			return false;
		}
		_lsSettings.format = _lffFormat;

		_lsSettings.format.pixelFormat = LUCAM_PF_8; //alternatively LUCAM_PF_16, but we want 8 bits per channel
		_lsSettings.useHwTrigger = FALSE;
		_lsSettings.timeout = 10000;

		_lsSettings.shutterType = LUCAM_SHUTTER_TYPE_ROLLING; //IDC_RADIO_GLOBAL_SHUTTER
		_lsSettings.exposureDelay = 0;
		_lsSettings.flReserved1 = 0.0;
		_lsSettings.flReserved2 = 0.0;
		_lsSettings.ulReserved1 = 0;
		_lsSettings.ulReserved2 = 0;

		return true;
	}

	bool LumeneraCamera::getFrame(cv::Mat& outImage)
	{
		//If we're not connected bail
		if (!_isConnected)
		{
			return false;
		}

		//CString csTemp;
		//FLOAT fClockSpeed;
		INT iSize;
		//int iPixelFormat;

		//BYTE bTemp;
		//USHORT usTemp;

		BYTE* m_pbRawFrame = NULL;
		BYTE* m_pbMonoFrame = NULL;
		BYTE* m_pbColorFrame = NULL;

		

		iSize = _lffFormat.width * _lffFormat.height;
		if (_lsSettings.format.pixelFormat == LUCAM_PF_16)
			iSize *= 2;

		/*if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
		if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
		if (m_pbColorFrame != NULL) delete (m_pbColorFrame);*/
		m_pbRawFrame = new BYTE[iSize];
		m_pbMonoFrame = new BYTE[iSize * 3];
		m_pbColorFrame = new BYTE[iSize * 3];
		if ((m_pbRawFrame == NULL) || (m_pbMonoFrame == NULL) || (m_pbColorFrame == NULL))
		{
			std::cerr << "Failed to allocate memory for frame buffer." << std::endl;
			if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
			m_pbRawFrame = NULL;
			if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
			m_pbMonoFrame = NULL;
			if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
			m_pbColorFrame = NULL;
			return false;
		}

		//fClockSpeed = (FLOAT)(GetCheckedRadioButton(IDC_RADIO_SCLOCK_FAST, IDC_RADIO_SCLOCK_VSLOW) - IDC_RADIO_SCLOCK_FAST); 
		//LucamSetProperty(_hCam, LUCAM_PROP_SNAPSHOT_CLOCK_SPEED, fClockSpeed, 0)

		if (_enableFastFrames)
		{
			if (LucamEnableFastFrames(_hCam, &_lsSettings))
			{
				if (!LucamTakeFastFrame(_hCam, m_pbRawFrame))
				{
					std::cerr << "Could not take fast snapshot." << std::endl;
					if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
					m_pbRawFrame = NULL;
					if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
					m_pbMonoFrame = NULL;
					if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
					m_pbColorFrame = NULL;
					LucamDisableFastFrames(_hCam);
					return false;
				}
				LucamDisableFastFrames(_hCam);
			}
			else
			{
				std::cerr << "Could not enable fast snapshot." << std::endl;
				if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}

		}
		else
		{
			if (!LucamTakeSnapshot(_hCam, &_lsSettings, m_pbRawFrame))
			{
				std::cerr << "Could not get snapshot." << std::endl;
				if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}
		}

		// Get color image
		_lcConversion.CorrectionMatrix = LUCAM_CM_FLUORESCENT;
		_lcConversion.DemosaicMethod = LUCAM_DM_HIGHER_QUALITY;

		if (!LucamConvertFrameToRgb24(_hCam, m_pbColorFrame, m_pbRawFrame, _lsSettings.format.width, _lsSettings.format.height, _lsSettings.format.pixelFormat, &_lcConversion))
		{
			std::cerr << "Could not convert snapshot to rgb." << std::endl;
			if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
			m_pbRawFrame = NULL;
			if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
			m_pbMonoFrame = NULL;
			if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
			m_pbColorFrame = NULL;
			return false;
		}

		// Get monochrome image
		/*if (_lsSettings.format.pixelFormat == LUCAM_PF_16)
		{
			if (!LucamConvertFrameToGreyscale16(_hCam, (USHORT *)m_pbMonoFrame, (USHORT *)m_pbRawFrame, _lsSettings.format.width, _lsSettings.format.height, _lsSettings.format.pixelFormat, &_lcConversion))
			{
				std::cerr << "Could not convert snapshot to rgb." << std::endl;
				if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}

			// Flip for display
			for (ULONG j = 0; j < _lsSettings.format.height / 2; j++)
			{
				for (ULONG k = 0; k < _lsSettings.format.width * 2; k++)
				{
					usTemp = m_pbMonoFrame[j * _lsSettings.format.width + k];
					m_pbMonoFrame[j * _lsSettings.format.width + k] = m_pbMonoFrame[(_lsSettings.format.height - j - 1) * _lsSettings.format.width + k];
					m_pbMonoFrame[(_lsSettings.format.height - j - 1) * _lsSettings.format.width + k] = (BYTE)usTemp;
				}
			}

			// Convert it to 24 bit mono (for display purposes)
			for (int i = iSize - 1; i >= 0; i--)
			{
				m_pbMonoFrame[i * 3] = m_pbMonoFrame[i * 2];
				m_pbMonoFrame[i * 3 + 1] = m_pbMonoFrame[i * 2];
				m_pbMonoFrame[i * 3 + 2] = m_pbMonoFrame[i * 2];
			}
		}
		else
		{
			if (!LucamConvertFrameToGreyscale8(_hCam, m_pbMonoFrame, m_pbRawFrame, _lsSettings.format.width, _lsSettings.format.height, _lsSettings.format.pixelFormat, &_lcConversion))
			{
				std::cerr << "Could not convert snapshot to rgb." << std::endl;
				if (m_pbRawFrame != NULL) delete[] (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete[] (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete[] (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}

			// Flip for display
			for (ULONG j = 0; j < _lsSettings.format.height / 2; j++)
			{
				for (ULONG k = 0; k < _lsSettings.format.width; k++)
				{
					bTemp = m_pbMonoFrame[j * _lsSettings.format.width + k];
					m_pbMonoFrame[j * _lsSettings.format.width + k] = m_pbMonoFrame[(_lsSettings.format.height - j - 1) * _lsSettings.format.width + k];
					m_pbMonoFrame[(_lsSettings.format.height - j - 1) * _lsSettings.format.width + k] = bTemp;
				}
			}

			// Convert it to 24 bit mono (for display purposes)
			for (int i = iSize - 1; i >= 0; i--)
			{
				m_pbMonoFrame[i * 3] = m_pbMonoFrame[i];
				m_pbMonoFrame[i * 3 + 1] = m_pbMonoFrame[i];
				m_pbMonoFrame[i * 3 + 2] = m_pbMonoFrame[i];
			}
		}*/

		//Load the image into the out mat
		outImage = cv::Mat(_lffFormat.height, _lffFormat.width, CV_8UC3, m_pbColorFrame);

		return true;
	}

	bool LumeneraCamera::getRawFrame(BYTE* rawFrame)
	{
		//If we're not connected bail
		if (!_isConnected)
		{
			return false;
		}

		//CString csTemp;
		//FLOAT fClockSpeed;
		INT iSize;
		//int iPixelFormat;

		//BYTE bTemp;
		//USHORT usTemp;

		BYTE* m_pbRawFrame = NULL;
		BYTE* m_pbMonoFrame = NULL;
		BYTE* m_pbColorFrame = NULL;



		iSize = _lffFormat.width * _lffFormat.height;
		if (_lsSettings.format.pixelFormat == LUCAM_PF_16)
			iSize *= 2;

		/*if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
		if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
		if (m_pbColorFrame != NULL) delete (m_pbColorFrame);*/
		m_pbRawFrame = new BYTE[iSize];
		m_pbMonoFrame = new BYTE[iSize * 3];
		m_pbColorFrame = new BYTE[iSize * 3];
		if ((m_pbRawFrame == NULL) || (m_pbMonoFrame == NULL) || (m_pbColorFrame == NULL))
		{
			std::cerr << "Failed to allocate memory for frame buffer." << std::endl;
			if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
			m_pbRawFrame = NULL;
			if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
			m_pbMonoFrame = NULL;
			if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
			m_pbColorFrame = NULL;
			return false;
		}

		//fClockSpeed = (FLOAT)(GetCheckedRadioButton(IDC_RADIO_SCLOCK_FAST, IDC_RADIO_SCLOCK_VSLOW) - IDC_RADIO_SCLOCK_FAST); 
		//LucamSetProperty(_hCam, LUCAM_PROP_SNAPSHOT_CLOCK_SPEED, fClockSpeed, 0)

		if (_enableFastFrames)
		{
			if (LucamEnableFastFrames(_hCam, &_lsSettings))
			{
				if (!LucamTakeFastFrame(_hCam, m_pbRawFrame))
				{
					std::cerr << "Could not take fast snapshot." << std::endl;
					if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
					m_pbRawFrame = NULL;
					if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
					m_pbMonoFrame = NULL;
					if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
					m_pbColorFrame = NULL;
					LucamDisableFastFrames(_hCam);
					return false;
				}
				LucamDisableFastFrames(_hCam);
			}
			else
			{
				std::cerr << "Could not enable fast snapshot." << std::endl;
				if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}

		}
		else
		{
			if (!LucamTakeSnapshot(_hCam, &_lsSettings, m_pbRawFrame))
			{
				std::cerr << "Could not get snapshot." << std::endl;
				if (m_pbRawFrame != NULL) delete (m_pbRawFrame);
				m_pbRawFrame = NULL;
				if (m_pbMonoFrame != NULL) delete (m_pbMonoFrame);
				m_pbMonoFrame = NULL;
				if (m_pbColorFrame != NULL) delete (m_pbColorFrame);
				m_pbColorFrame = NULL;
				return false;
			}
		}

		rawFrame = m_pbRawFrame;
	
		return true;
	}
	
	bool LumeneraCamera::is_connected()
	{
		return _isConnected;
	}

	void LumeneraCamera::get_image_size(int& width, int& height)
	{
		width = _lsSettings.format.width;
		height = _lsSettings.format.height;
	}

	bool LumeneraCamera::addNewFrameCallback(void* callbackFunc, void* callbackObj)
	{
		if (_previewCallbackRegNumber == -1)
		{
			//_previewCallbackRegNumber = LucamAddRgbPreviewCallback(_hCam, (VOID(__stdcall *)(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused)) callbackFunc, callbackObj, _lsSettings.format.pixelFormat);
			_previewCallbackRegNumber = LucamAddStreamingCallback(_hCam, (VOID(__stdcall *)(VOID *pContext, BYTE *pData, ULONG dataLength)) callbackFunc, callbackObj);
			return true;
		}
		return false;
	}

	bool LumeneraCamera::startVideoStream()
	{
		return LucamStreamVideoControl(_hCam, START_STREAMING, nullptr) != 0; //Returns a BOOL, aka an int so we compare it to 0 to stop a warning
	}

	void LumeneraCamera::byteDataToMat(BYTE* data, cv::Mat& outImage)
	{
		//First we convert it to color
		int iSize = _lffFormat.width * _lffFormat.height;
		BYTE* m_pbColorFrame = new BYTE[iSize * 3];

		LucamConvertFrameToRgb24(_hCam, m_pbColorFrame, data, _lsSettings.format.width, _lsSettings.format.height, _lsSettings.format.pixelFormat, &_lcConversion);

		//Now we convert it to a mat and return it
		//cv::Mat outImage;
		outImage = cv::Mat(_lffFormat.height, _lffFormat.width, CV_8UC3, data); //TODO: make this based on the format, not always CV_8UC3 maybe
	}
}

//(VOID(__stdcall *)(VOID *pContext, BYTE *pData, ULONG dataLength, ULONG unused)) //good lord