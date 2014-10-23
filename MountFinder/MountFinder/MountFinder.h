
#pragma once




double get_point_line_distance(cv::Vec4f line, cv::Point point);

double get_squared_error_line(cv::Vec4f line, std::vector<cv::Point> points);

double fit_two_lines(std::vector<cv::Point> points, int& splitPoint);

bool is_edge(const cv::Mat& frame, int& nextX, int& nextY, bool& is_corner, int& cornerX, int& cornerY);

void check_tests(const cv::Mat& frame);

void on_x(int, void*);

void on_y(int, void*);

void on_corner_index(int, void*);

void run_corner_detection();
