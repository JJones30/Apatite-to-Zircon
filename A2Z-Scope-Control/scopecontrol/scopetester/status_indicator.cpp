//Implementation of simple status indicators

#include "status_indicator.h"

void SimpleStatusIndicator::enable()
{
	setStyleSheet("QLabel { background-color: green; color: white; \
							border-radius: 3px; \
							padding: 1px;}");
}
//border-style: solid; border-width: 1px; border-color: beige;

void SimpleStatusIndicator::disable()
{
	setStyleSheet("QLabel { background-color: red; color: white; \
				  			border-radius: 3px; \
							padding: 1px;}");
}