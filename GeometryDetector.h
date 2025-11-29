#pragma once
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class GeometryDetector {
public:
	// 存储直线类型
	enum LineType {
		NORMAL_LINE,
		PARALLEL_BISECTOR,
		ANGLE_LINE
	};

	struct LineInfo {
		cv::Vec4f points;  // 线段端点坐标 (x1,y1,x2,y2)
		LineType type;
		LineInfo(cv::Vec4f p, LineType t) : points(p), type(t) {}
	};

	// 计算函数
	static double calculateDistance(cv::Point2f p1, cv::Point2f p2);
	static double calculateAngle(cv::Point2f p1, cv::Point2f p2, cv::Point2f p3);
	static double calculateLineAngle(const cv::Vec4f& line);
	static bool isParallel(const cv::Vec4f& line1, const cv::Vec4f& line2, double angleThreshold = 0.1);
	static double pointToLineDistance(cv::Point2f point, const cv::Vec4f& line);
	static double calculateTwoLinesAngle(const cv::Vec4f& line1, const cv::Vec4f& line2);

	// 几何图形检测与拟合
	static void detectHoughCircles(const cv::Mat& src, std::vector<cv::Vec3f>& circles, 
		double dp = 1.0, double minDist = 20.0, 
		double param1 = 100.0, double param2 = 30.0, 
		int minRadius = 10, int maxRadius = 100);

	static void detectHoughLines(const cv::Mat& src, std::vector<cv::Vec4i>& lines, 
		double rho = 1.0, double theta = CV_PI/180, int threshold = 50, 
		double minLineLength = 50.0, double maxLineGap = 10.0);

	static void fitCircleLeastSquares(const std::vector<cv::Point>& points, cv::Point2f& center, float& radius);
	static void fitLineLeastSquares(const std::vector<cv::Point>& points, cv::Vec4f& line);

	// 图像处理辅助
	static void preprocessImage(const cv::Mat& src, cv::Mat& dst);

	// 绘图辅助
	static void putTextZH(cv::Mat& img, const std::string& text, cv::Point pos,
		const cv::Scalar& color, double fontScale = 0.5, int thickness = 1);
	static void drawParallelBisector(cv::Mat& img, const cv::Vec4f& line1, const cv::Vec4f& line2, cv::Vec4f& bisector);
	static void drawAngleLabel(cv::Mat& img, const cv::Vec4f& line1, const cv::Vec4f& line2);
};