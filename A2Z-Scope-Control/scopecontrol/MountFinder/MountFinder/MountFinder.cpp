// MountFinder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define EDGE_CONTOUR_AREA_LIMIT .95 //95 percent
#define BLUR_THRESH_SIZE 101 //101 pixels
#define CORNER_MIN_DISTANCE 1000
#define MIN_SIDE_LENGTH 50
#define PAUSETIME 1
#define MIN_ERROR_FOR_CORNER 3200

#pragma managed
#include "MountFinder.h"

//Trackbar junk
cv::Mat frame;
int trackbar_x;
int trackbar_y;
int corner_index;

sc::VirtualScope* scope;
std::vector<cv::Point> corners;





double get_point_line_distance(cv::Vec4f line, cv::Point point)
{
	using namespace cv;
	Point2f linePoint(line[2], line[3]);
	Mat lineDirection(Point2f(line[0], line[1]));

	Mat normedLineDirection;
	normalize(lineDirection, normedLineDirection);
;
	Mat pointDifference(linePoint - (Point2f) point);
	pointDifference.at<float>(0, 0);
	double differenceLength = pow(pow(pointDifference.at<float>(0, 0), 2) + pow(pointDifference.at<float>(1, 0), 2), 0.5);

	Mat normedDifference;
	normalize(pointDifference, normedDifference);

	//std::cout << "Line " << normedLineDirection << std::endl;
	//std::cout << "normed" << normedDifference << std::endl;

	double dotProduct = normedLineDirection.dot(normedDifference);

	assert(abs(dotProduct) <= 1.001);

	if (dotProduct > 1)
	{
		dotProduct = 1;
	}
	else if (dotProduct < -1)
	{
		dotProduct = -1;
	}

	//std::cout << "Dot product: " << dotProduct << std::endl;

	double angle = acos(dotProduct);

	//std::cout << "Angle: " << angle << std::endl;

	double distance = differenceLength * sin(angle);

	//std::cout << "Adding distance " << distance << std::endl;
	return distance;
}


double get_squared_error_line(cv::Vec4f line, std::vector<cv::Point> points)
{
	double totalDistance = 0; //sum of squared error
	for (auto pointIt = points.begin(); pointIt != points.end(); pointIt++)
	{
		double distance = get_point_line_distance(line, *pointIt);
		totalDistance += distance * distance;
	}

	return totalDistance;
}

double fit_two_lines(std::vector<cv::Point> points, int& splitPoint)
{
	using namespace std;
	using namespace cv;

	//Ensure we have enough points
	assert(points.size() > 2);

	double min_error = 100000;

	for (size_t c = 1; c < points.size() - 1; c++)
	{
		//Split into two new vectors
		vector<Point>::const_iterator first = points.begin();
		vector<Point>::const_iterator mid = points.begin() + c;
		vector<Point>::const_iterator last = points.end();
		vector<Point> newVec(first, last);

		vector<Point> set1(first, mid);
		vector<Point> set2(mid, last);

		assert(set1.size() + set2.size() == points.size());

		//Fit a line to each
		Vec4f line1;
		Vec4f line2;
		fitLine(set1, line1, CV_DIST_L2, 0, 0.01, 0.01);
		fitLine(set2, line2, CV_DIST_L2, 0, 0.01, 0.01);
		
		//Compute total error on the line
		double error1 = get_squared_error_line(line1, set1);
		double error2 = get_squared_error_line(line2, set2);

		double total_error = error1 + error2;

		//If it's better, store it!
		if (total_error < min_error)
		{
			min_error = total_error;
			splitPoint = c;
		}
	}

	return min_error;
}

//Checks if something is an edge or a corner of a mount!
//
bool is_edge(const cv::Mat& frame, int& nextX, int& nextY, bool& is_corner, int& cornerX, int& cornerY)
{
	using namespace cv;

	namedWindow("Thresh", WINDOW_AUTOSIZE);// Create a window for display.

	is_corner = false;
	
	Scalar frameMean = mean(frame);
	double totalMean = (frameMean[0] + frameMean[1] + frameMean[2]) / 3;

	Mat greyFrame;
	cvtColor(frame, greyFrame, CV_BGR2GRAY);

	//Blur the original image hugely so that its defined in edge/not edge, basically
	Mat greyBlur;
	int blur_size = BLUR_THRESH_SIZE; //size has to be odd
	GaussianBlur(greyFrame, greyBlur, cv::Size(blur_size, blur_size), 0, 0);

	//Threshold by mean
	Mat threshed;
	
	threshold(greyBlur, threshed, totalMean / 255.0, 255, cv::THRESH_BINARY); //TODO: totalMean (and maxVal) might need to be divided by 255
	
	Mat displayImage;
	cvtColor(threshed, displayImage, CV_GRAY2RGB);
	
	//Find contours within the image
	vector<vector<Point>> unsorted_contours;
	Mat contourCopy = threshed.clone();
	findContours(contourCopy, unsorted_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	vector<Point> contour; //the biggest contour
	double maxContourArea = -1.0;
	for (auto contour_it = unsorted_contours.begin(); contour_it != unsorted_contours.end(); contour_it++)
	{
		if (contourArea(*contour_it) > maxContourArea)
		{
			maxContourArea = contourArea(*contour_it);
			contour = *contour_it;
		}
	}


	//Draw contours on threshed image
	drawContours(displayImage, unsorted_contours, -1, Scalar(128, 255, 255), 2);




	//Compute whether or not we're seeing an edge
	//If we found no contours, not an edge
	if (unsorted_contours.size() < 1)
	{
		return false;
	}

	//std::cout << "Contour area: " << contourArea(contours[0]) << std::endl;
	//std::cout << "Image area: " << frame.cols * frame.rows << std::endl;

	//If our first contour's area is the whole image, not an edge
	//If the contour's area is more than 95% of the image area then its probably not an edge
	if (contourArea(contour) >= (frame.cols * frame.rows) * EDGE_CONTOUR_AREA_LIMIT)
	{
		return false;
	}

	//So now it is most likely an edge, find where we should move next
	vector<Point> corners;
	vector<size_t> cornerIdx;

	//corners.push_back(Point(contours[0][0]));
	circle(displayImage, contour[0], 10, Scalar(0, 0, 255), -1);
	circle(displayImage, contour[1], 10, Scalar(0, 0, 255), -1);

	bool is_horizontal = true;
	bool is_vertical = true;
	
	int min_side_length = MIN_SIDE_LENGTH;
	Point last = contour[0];

	Point destination;

	std::cout << "Contours has: " << contour.size() << std::endl;


	//Find edges of the frame 
	size_t last_edge_end = 0;
	size_t longest_start = 0;
	size_t longest_end = 0;
	
	Point point = contour[0];
	Point end(-1, -1);
	for (size_t c = 0; point != end || c < contour.size(); c++)
	{
		point = contour[c % contour.size() ];
		Point next = contour[(c + 1) % contour.size()];

		//std::cout << "Point: " << point << std::endl;
		if ((point.y == next.y && (point.y == frame.cols - 2 || point.y == 1)) ||
			(point.x == next.x && (point.x == frame.rows - 2 || point.x == 1)))
		{
			if (end == Point(-1, -1))
				end = point;


			std::cout << "Found edge, long enough? ";
			if (pow(point.y - next.y, 2) + pow(point.x - next.x, 2) > MIN_SIDE_LENGTH * MIN_SIDE_LENGTH)
			{
				//Have an edge of the frame!
				std::cout << "Yes!~ Is it the longest? ";

				std::cout << "c: " << c << std::endl; 
				std::cout << "last_edge_end: " << last_edge_end << std::endl;
				std::cout << "longest_end: " << longest_end << std::endl;
				std::cout << "longest_start: " << longest_start << std::endl;

				std::cout << "c mod: " << (c % contour.size()) - last_edge_end << std::endl;
				std::cout << "whole mod: " << ((c % contour.size()) - last_edge_end) % contour.size() << std::endl;

				//Is it the longest edge?
				if ((c % contour.size()) - last_edge_end > longest_end - longest_start) //careful about c % - last being negative?
				{
					//Yes!
					std::cout << "Yeeeee!" << std::endl;
					longest_start = last_edge_end;
					longest_end = c % contour.size();

					destination = contour[longest_end];
				}
				
				last_edge_end = (c + 1) % contour.size();
			}
		}
	}

	std::cout << "Corners: " << corners << std::endl;

	std::cout << "CornerIdx: ";
	for (auto c = cornerIdx.begin(); c != cornerIdx.end(); c++)
	{
		std::cout << *c << ", ";
	}
	std::cout << std::endl;

	//Find destination. Final destination?
	size_t max_dist = 0;
	size_t mountedge_start = 0;
	size_t mountedge_end = 0;
	for (int c = 0; c < corners.size(); c++)
	{
		int index1 = cornerIdx[c];
		int index2 = cornerIdx[(c + 1) % cornerIdx.size()];
		int distance = abs<int>((index2 - index1) % contour.size());

		std::cout << "Points: " << index1 << " and " << index2 << std::endl;
		//std::cout << "Minus " << index2 - index1 << std::endl;
		//std::cout << "Mod " << (index2 - index1) % contour.size() << std::endl;
		//std::cout << "Abs " << abs<int>((index2 - index1) % contour.size()) << std::endl;

		if (distance > max_dist)
		{
			
			//New destination!
			max_dist = distance;
			std::cout << "New maxdistance: " << max_dist << std::endl;
			
			std::cout << "New maxdistance with corners: " << corners[c] << " and " << corners[(c + 1) % cornerIdx.size()] << std::endl;
			std::cout << "New maxdistance with points: " << contour[index1] << " and " << contour[index2] << std::endl;
			 
			destination = corners[c];

			mountedge_start = index1;
			mountedge_end = index2;

			

			//Handle regular case (doesn't cross 0)
			/*if (index1 < index2)
			{
				destination = corners[c];
			}
			else
			{
				destination = corners[index1];
			}*/
		}
	}

	mountedge_start = longest_start;
	mountedge_end = longest_end;

	//Now that we have mountedge_start and end, analyze how closely the edge follows a line
	std::cout << "start: " << mountedge_start << std::endl;
	std::cout << "end: " << mountedge_end << std::endl;

	//Extract the edge points into another vector
	vector<Point> mountEdge;
	for (int c = mountedge_start; c != mountedge_end; c = (c+1) % contour.size())
	{
		mountEdge.push_back(contour[c]);
	}

	std::cout << "mountEdgeSize: " << mountEdge.size() << std::endl;

	/*imshow("Thresh", displayImage);                   // Show our image inside it.
	waitKey(0);*/

	//Now fit a line to it
	Vec4f line;
	fitLine(mountEdge, line, CV_DIST_L2, 0, 0.01, 0.01);

	std::cout << "Line: " << line << std::endl;

	//Now get the sum of squared error
	Point2f linePoint(line[2], line[3]);
	Mat lineDirection(Point2f(line[0], line[1]));

	//Draw it
	cv::line(displayImage, linePoint, Point(linePoint.x + line[0] * 500, linePoint.y + line[1] * 500), Scalar(100, 100, 255), 5);
	cv::line(displayImage, linePoint, Point(linePoint.x + line[0] * -500, linePoint.y + line[1] * -500), Scalar(100, 100, 255), 5);

	Mat normedLineDirection;
	normalize(lineDirection, normedLineDirection);

	double totalDistance = get_squared_error_line(line, mountEdge);

	//std::cout << "Total Distance: " << totalDistance << std::endl;

	//double averageSquaredError = sqrt(totalDistance) / mountEdge.size();
	double averageSquaredError = totalDistance / mountEdge.size();
	std::cout << "Avg sq error: " << averageSquaredError << std::endl;
	

	std::cout << "Destination is " << destination << std::endl;

	std::cout << "Corners size: " << corners.size() << std::endl;


	//std::cout << "Corners has: " << corners.size() << std::endl;
	//That should accumulate the right points, draw them
	for (auto corners_it = corners.begin(); corners_it != corners.end(); corners_it++)
	{
		circle(displayImage, *corners_it, 5, Scalar(255, 128, 0), -1);
	}
	
	//Draw the destination for next move
	circle(displayImage, destination, 6, Scalar(0, 128, 255), -1);

	//Draw other test stuff
	for (auto corners_it = mountEdge.begin(); corners_it != mountEdge.end(); corners_it++)
	{
		circle(displayImage, *corners_it, 5, Scalar(0, 255, 0), -1);
	}

	//Set it to the output variables
	nextX = destination.x;
	nextY = destination.y;

	//If we've got three sides, this is a corner most likely. Better detection desired! TODO
	if (averageSquaredError > MIN_ERROR_FOR_CORNER)
	{
		is_corner = true;
		
	}

	//std::cout << "2 line distance: " << fit_two_lines(mountEdge) / mountEdge.size() << std::endl;


	Point estimatedCorner;
	if (is_corner)
	{
		int cornerIndex = 0;
		fit_two_lines(mountEdge, cornerIndex);

		estimatedCorner = contour[cornerIndex];

		cornerX = estimatedCorner.x;
		cornerY = estimatedCorner.y;
	}

	//If we found a corner we should try and figure out exactly where it is
	//if (is_corner)
	//{
	//	//Three points, find the two that don't have any points in common
	//	for (size_t c = 0; c < corners.size(); c++)
	//	{
	//		Point cPoint = corners[c];
	//		Point nextPoint = corners[(c + 1) % corners.size()];
	//		if (cPoint.x != nextPoint.x && cPoint.y != nextPoint.y)
	//		{
	//			//Find which point's x value we should be using, and use the other's y
	//			if (abs(cPoint.x - frame.size().width / 2) < abs(nextPoint.x - frame.size().width / 2))
	//			{
	//				estimatedCorner.x = cPoint.x;
	//				estimatedCorner.y = nextPoint.y;
	//			}
	//			else
	//			{
	//				estimatedCorner.x = nextPoint.x;
	//				estimatedCorner.y = cPoint.y;
	//			}
	//		}
	//	}

	//	//Then find the point on our contour that is closest
	//	for (auto contour_it = contour.begin(); contour_it != contour.end(); contour_it++)
	//	{
	//		if (pow((contour_it->x - estimatedCorner.x), 2) + pow((contour_it->y - estimatedCorner.y), 2) <
	//			pow((cornerX - estimatedCorner.x), 2) + pow((cornerY - estimatedCorner.y), 2))
	//		{
	//			//Found a new closer point
	//			cornerX = contour_it->x;
	//			cornerY = contour_it->y;
	//		}
	//	}

	//	//Draw the corner
	//	circle(displayImage, Point(cornerX, cornerY), 6, Scalar(0, 255, 0), -1);
	//}

	




	//waitKey(0);

	//std::cout << "Edge: " << totalMean << std::endl;

	//Show threshed window
	imshow("Thresh", displayImage);                   // Show our image inside it.

	return true;
}


void check_tests(const cv::Mat& frame)
{
	int x, y;
	int cornerX, cornerY;
	bool is_corner;
	std::cout << "Is an edge: " << is_edge(frame, x, y, is_corner, cornerX, cornerY) << std::endl;

}

void on_x(int, void*)
{
	scope->move_sync(trackbar_x, trackbar_y);

	scope->getFrame(frame);

	imshow("Frame Display", frame);

	check_tests(frame);
}

void on_y(int, void*)
{
	scope->move_sync(trackbar_x, trackbar_y);

	scope->getFrame(frame);

	imshow("Frame Display", frame);

	check_tests(frame);
}

void on_corner_index(int, void*)
{
	double corner_x, corner_y;
	corner_x = corners[corner_index].x;
	corner_y = corners[corner_index].y;

	trackbar_x = (int) corner_x - frame.cols/2;
	trackbar_y = (int) corner_y - frame.rows/2;

	scope->move_sync(trackbar_x, trackbar_y);

	scope->getFrame(frame);

	imshow("Frame Display", frame);

	//Does it move there?
}

void run_corner_detection(sc::StageBase* stage, sc::CameraBase* cam)
{
	using namespace std;

	//Get our position from the scope
	//scope->where(trackbar_x, trackbar_y);

	double xtest, ytest;
	stage->where(xtest, ytest);
	trackbar_x = (int)xtest;
	trackbar_y = (int)ytest;

	cout << "Getting a frame" << endl;
	cam->getFrame(frame);

	cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
	cv::imshow("Frame Display", frame);

	double maxX, maxY;
	maxX = 100;
	maxY = 100;
	//stage->max_position(maxX, maxY);
	cv::createTrackbar("X", "Frame Display", &trackbar_x, (int)maxX, on_x);
	cv::createTrackbar("Y", "Frame Display", &trackbar_y, (int)maxY, on_y);

	//Now start to search for corners
	
	bool still_possible = true;
	while (corners.size() < 4 && still_possible)
	{
		//Get a new frame
		cam->getFrame(frame);
		//Show new screen
		cv::imshow("Frame Display", frame);

		int nextX, nextY, cornerX, cornerY;
		bool is_corner;

		//Are we currently looking at an edge?
		if (is_edge(frame, nextX, nextY, is_corner, cornerX, cornerY))
		{
			//If so, is it a corner?
			if (is_corner)
			{
				cv::Point cornerLoc(cornerX + trackbar_x, cornerY + trackbar_y);

				//If it is, check if we already have a corner near there
				bool no_nearby_corners = true;
				for (auto corner_it = corners.begin(); corner_it != corners.end(); corner_it++)
				{
					cv::Point corner = *corner_it;
					if (sqrt(pow(corner.x - cornerLoc.x, 2) + pow(corner.y - cornerLoc.y, 2)) < CORNER_MIN_DISTANCE)
					{
						no_nearby_corners = false;
					}
				}

				if (no_nearby_corners)
				{
					std::cout << "Found a new corner!" << std::endl;
					corners.push_back(cornerLoc);
				}
			}
			
			cout << "trackbar, cols, next: " << trackbar_x << ", " << frame.cols << ", " << nextX << endl;

			//int newX = trackbar_x - (frame.cols - nextX) + nextX;
			//int newY = trackbar_y - (frame.rows - nextY) + nextY;
			int newX = trackbar_x + (nextX - frame.cols / 2);
			int newY = trackbar_y + (nextY - frame.rows / 2);

			cout << "Moving to: " << newX << ", " << newY << endl;

			//Move to new location
			trackbar_x = newX;
			trackbar_y = newY;
			stage->move_sync(newX, newY);



			//Pause for visibility
			cv::waitKey(PAUSETIME);
		}
		else
		{
			//If not, search for an edge

			//Move one frame to the right unless we hit an edge
			//Trackbar_x and y should be accurate
			size_t frameWidth = frame.cols;
			size_t frameHeight = frame.rows;

			double maxX, maxY;
			maxX = 100;
			maxY = 100;
			//stage->max_position(maxX, maxY);

			double minX, minY;
			minX = 0;
			minY = 0;
			//stage->min_position(minX, minY);

			if (trackbar_x + frameWidth < maxX)
			{
				//Move right
				trackbar_x += frameWidth;
			}
			else
			{
				if (trackbar_y + frameHeight < maxY)
				{
					//Move all the way left and down slightly
					trackbar_x = minX;
					trackbar_y = trackbar_y + frameHeight;
				}
				else
				{
					still_possible = false;
					std::cout << "WARNING: Search failed!" << std::endl;
				}
			}

			cout << "Moving to: " << trackbar_x << ", " << trackbar_y << endl;
			stage->move_sync(trackbar_x, trackbar_y);
			//Show new screen
			cv::imshow("Frame Display", frame);

			//Pause for visibility
			cv::waitKey(PAUSETIME);
		}
	}

	cout << "Corners: " << corners << endl;
	
	cv::createTrackbar("Corner", "Frame Display", &corner_index, corners.size() - 1, on_corner_index);

	//Also show the analysis window
	check_tests(frame); 

	cv::waitKey(0);
	 
	cout << "Closing stage." << endl;
	//scope->close();
	cout << "Stage closed." << endl;

}
/*
 The ORIGINAL
void run_corner_detection()
{
	using namespace std;

	scope = new sc::VirtualScope("virtual_config.xml");

	//Get our position from the scope
	//scope->where(trackbar_x, trackbar_y);

	double xtest, ytest;
	scope->where(xtest, ytest);
	trackbar_x = (int)xtest;
	trackbar_y = (int)ytest;

	cout << "Getting a frame" << endl;
	scope->getFrame(frame);

	cv::namedWindow("Frame Display", cv::WINDOW_AUTOSIZE);
	cv::imshow("Frame Display", frame);

	double maxX, maxY;
	scope->max_position(maxX, maxY);
	cv::createTrackbar("X", "Frame Display", &trackbar_x, (int)maxX, on_x);
	cv::createTrackbar("Y", "Frame Display", &trackbar_y, (int)maxY, on_y);

	//Now start to search for corners

	bool still_possible = true;
	while (corners.size() < 4 && still_possible)
	{
		//Get a new frame
		scope->getFrame(frame);
		//Show new screen
		cv::imshow("Frame Display", frame);

		int nextX, nextY, cornerX, cornerY;
		bool is_corner;

		//Are we currently looking at an edge?
		if (is_edge(frame, nextX, nextY, is_corner, cornerX, cornerY))
		{
			//If so, is it a corner?
			if (is_corner)
			{
				cv::Point cornerLoc(cornerX + trackbar_x, cornerY + trackbar_y);

				//If it is, check if we already have a corner near there
				bool no_nearby_corners = true;
				for (auto corner_it = corners.begin(); corner_it != corners.end(); corner_it++)
				{
					cv::Point corner = *corner_it;
					if (sqrt(pow(corner.x - cornerLoc.x, 2) + pow(corner.y - cornerLoc.y, 2)) < CORNER_MIN_DISTANCE)
					{
						no_nearby_corners = false;
					}
				}

				if (no_nearby_corners)
				{
					std::cout << "Found a new corner!" << std::endl;
					corners.push_back(cornerLoc);
				}
			}

			cout << "trackbar, cols, next: " << trackbar_x << ", " << frame.cols << ", " << nextX << endl;

			//int newX = trackbar_x - (frame.cols - nextX) + nextX;
			//int newY = trackbar_y - (frame.rows - nextY) + nextY;
			int newX = trackbar_x + (nextX - frame.cols / 2);
			int newY = trackbar_y + (nextY - frame.rows / 2);

			cout << "Moving to: " << newX << ", " << newY << endl;

			//Move to new location
			trackbar_x = newX;
			trackbar_y = newY;
			scope->move_sync(newX, newY);



			//Pause for visibility
			cv::waitKey(PAUSETIME);
		}
		else
		{
			//If not, search for an edge

			//Move one frame to the right unless we hit an edge
			//Trackbar_x and y should be accurate
			size_t frameWidth = frame.cols;
			size_t frameHeight = frame.rows;

			double maxX, maxY;
			scope->max_position(maxX, maxY);

			double minX, minY;
			scope->min_position(minX, minY);

			if (trackbar_x + frameWidth < maxX)
			{
				//Move right
				trackbar_x += frameWidth;
			}
			else
			{
				if (trackbar_y + frameHeight < maxY)
				{
					//Move all the way left and down slightly
					trackbar_x = minX;
					trackbar_y = trackbar_y + frameHeight;
				}
				else
				{
					still_possible = false;
					std::cout << "WARNING: Search failed!" << std::endl;
				}
			}

			cout << "Moving to: " << trackbar_x << ", " << trackbar_y << endl;
			scope->move_sync(trackbar_x, trackbar_y);
			//Show new screen
			cv::imshow("Frame Display", frame);

			//Pause for visibility
			cv::waitKey(PAUSETIME);
		}
	}

	cout << "Corners: " << corners << endl;

	cv::createTrackbar("Corner", "Frame Display", &corner_index, corners.size() - 1, on_corner_index);

	//Also show the analysis window
	check_tests(frame);

	cv::waitKey(0);

	cout << "Closing stage." << endl;
	scope->close();
	cout << "Stage closed." << endl;

}
*/

