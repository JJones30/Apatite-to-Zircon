//Class for status indicators! They have text, turn green when good and red when bad

#include "stdafx.h"

class SimpleStatusIndicator : public QLabel
{
public:
	void enable();
	void disable();
};