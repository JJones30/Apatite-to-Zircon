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

	LumeneraCamera::LumeneraCamera()
	{
		//Do nothing! Woo
	}


	bool LumeneraCamera::connect(int cameraNum)
	{
		_hCam = LucamCameraOpen(cameraNum);

		if (_hCam != NULL)
		{
			//Set local object settings
			_isConnected = true;


			if (!LucamGetFormat(_hCam, &_lffFormat, &_videoFrameRate))
			{
				std::cerr << "Unable to get camera video format. Snapshot mode disabled." << std::endl;
				return false;
			}


			//Lucam settings

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
			_isConnected = FALSE;
			_hCam = NULL;
			_lffFormat.height = 0;
			_lffFormat.width = 0;

			return true;
		}
	}

	bool LumeneraCamera::getFrame(cv::Mat& outImage)
	{
		FLOAT fClockSpeed;
		INT iSize;
		int iPixelFormat;
		FLOAT fValue;
		LONG lFlags;
		BYTE bTemp;
		USHORT usTemp;

		BYTE* m_pbRawFrame = NULL;
		BYTE* m_pbMonoFrame = NULL;
		BYTE* m_pbColorFrame = NULL;

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

		_lsSettings.format.pixelFormat = LUCAM_PF_8; //LUCAM_PF_16
		_lsSettings.useHwTrigger = FALSE;
		_lsSettings.timeout = 10000;

		_lsSettings.shutterType = LUCAM_SHUTTER_TYPE_ROLLING; //IDC_RADIO_GLOBAL_SHUTTER
		_lsSettings.exposureDelay = 0;
		_lsSettings.flReserved1 = 0.0;
		_lsSettings.flReserved2 = 0.0;
		_lsSettings.ulReserved1 = 0;
		_lsSettings.ulReserved2 = 0;

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
		if (_lsSettings.format.pixelFormat == LUCAM_PF_16)
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
		}

		return true;
	}
	
}