#include "GeometryDetector.h"
#include <numeric>

using namespace cv;
using namespace std;

double GeometryDetector::calculateDistance(Point2f p1, Point2f p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

double GeometryDetector::calculateAngle(Point2f p1, Point2f p2, Point2f p3) {
    double angle = atan2(p3.y - p2.y, p3.x - p2.x) -
        atan2(p1.y - p2.y, p1.x - p2.x);
    angle = angle * 180 / CV_PI;
    if (angle < 0) angle += 360;
    if (angle > 180) angle = 360 - angle;  // 取锐角
    return angle;
}

double GeometryDetector::calculateLineAngle(const Vec4f& line) {
    return atan2(line[3] - line[1], line[2] - line[0]) * 180 / CV_PI;
}

bool GeometryDetector::isParallel(const Vec4f& line1, const Vec4f& line2, double angleThreshold) {
    double angle1 = calculateLineAngle(line1);
    double angle2 = calculateLineAngle(line2);

    double angleDiff = fabs(angle1 - angle2);
    while (angleDiff > 180) angleDiff = fabs(angleDiff - 360);

    return angleDiff <= angleThreshold || fabs(180 - angleDiff) <= angleThreshold;
}

double GeometryDetector::pointToLineDistance(Point2f point, const Vec4f& line) {
    double A = line[3] - line[1];  // y2 - y1
    double B = line[0] - line[2];  // x1 - x2
    double C = line[2] * line[1] - line[0] * line[3];  // x2*y1 - x1*y2

    return fabs(A * point.x + B * point.y + C) / sqrt(A * A + B * B);
}

double GeometryDetector::calculateTwoLinesAngle(const Vec4f& line1, const Vec4f& line2) {
    double angle1 = calculateLineAngle(line1);
    double angle2 = calculateLineAngle(line2);

    double angle = fabs(angle1 - angle2);
    if (angle > 180) angle = 360 - angle;
    if (angle > 90) angle = 180 - angle;

    return angle;
}

void GeometryDetector::putTextZH(Mat& img, const string& text, Point pos,
    const Scalar& color, double fontScale, int thickness) {
    int baseline = 0;
    Size textSize = getTextSize(text, FONT_HERSHEY_SIMPLEX,
        fontScale, thickness, &baseline);

    rectangle(img,
        Point(pos.x, pos.y - textSize.height),
        Point(pos.x + textSize.width, pos.y + baseline),
        Scalar(255, 255, 255),
        FILLED);

    putText(img, text, pos, FONT_HERSHEY_SIMPLEX,
        fontScale, color, thickness, LINE_AA);
}

void GeometryDetector::drawParallelBisector(Mat& img, const Vec4f& line1,
    const Vec4f& line2, Vec4f& bisector) {
    if (!isParallel(line1, line2)) {
        putTextZH(img, "所选直线不平行", Point(50, 50), Scalar(0, 0, 255), 1.0);
        return;
    }

    Point2f mid1((line1[0] + line1[2]) / 2, (line1[1] + line1[3]) / 2);
    Point2f mid2((line2[0] + line2[2]) / 2, (line2[1] + line2[3]) / 2);

    double dx = line1[2] - line1[0];
    double dy = line1[3] - line1[1];
    double length = sqrt(dx * dx + dy * dy);
    dx /= length;
    dy /= length;

    Point2f centerPoint((mid1.x + mid2.x) / 2, (mid1.y + mid2.y) / 2);

    double lineLength = length;
    Point2f start(centerPoint.x - dx * lineLength / 2, centerPoint.y - dy * lineLength / 2);
    Point2f end(centerPoint.x + dx * lineLength / 2, centerPoint.y + dy * lineLength / 2);

    bisector = Vec4f(start.x, start.y, end.x, end.y);

    line(img, start, end, Scalar(255, 0, 255), 2, LINE_AA);

    double dist = pointToLineDistance(centerPoint, line1);
    putTextZH(img, format("距离: %.1f", dist),
        Point(centerPoint.x - 60, centerPoint.y - 20),
        Scalar(0, 255, 255));
}

void GeometryDetector::drawAngleLabel(cv::Mat& img, const cv::Vec4f& line1, const cv::Vec4f& line2) {
    // Implementation can be added here or reusing existing logic
}

// New Functions

void GeometryDetector::preprocessImage(const cv::Mat& src, cv::Mat& dst) {
    if (src.channels() == 3) {
        cvtColor(src, dst, COLOR_BGR2GRAY);
    } else {
        dst = src.clone();
    }
    GaussianBlur(dst, dst, Size(9, 9), 2, 2);
}

void GeometryDetector::detectHoughCircles(const cv::Mat& src, std::vector<cv::Vec3f>& circles, 
    double dp, double minDist, double param1, double param2, int minRadius, int maxRadius) {
    Mat gray;
    preprocessImage(src, gray);
    HoughCircles(gray, circles, HOUGH_GRADIENT, dp, minDist, param1, param2, minRadius, maxRadius);
}

void GeometryDetector::detectHoughLines(const cv::Mat& src, std::vector<cv::Vec4i>& lines, 
    double rho, double theta, int threshold, double minLineLength, double maxLineGap) {
    Mat gray, edges;
    preprocessImage(src, gray);
    Canny(gray, edges, 50, 150, 3);
    HoughLinesP(edges, lines, rho, theta, threshold, minLineLength, maxLineGap);
}

void GeometryDetector::fitCircleLeastSquares(const std::vector<cv::Point>& points, cv::Point2f& center, float& radius) {
    if (points.size() < 3) return;
    
    // Use OpenCV's minEnclosingCircle as a fast approximation for least squares if precise algebraic fit isn't strictly required,
    // or implement Kasa method. 
    // For "Least Squares", minEnclosingCircle is NOT least squares (it's minmax).
    // But for simple arc fitting, we can use a simple algebraic fit.
    
    // Simple algebraic fit (Kasa method):
    // Minimize sum((x-xc)^2 + (y-yc)^2 - R^2)^2
    
    double sum_x = 0, sum_y = 0;
    double sum_x2 = 0, sum_y2 = 0;
    double sum_xy = 0, sum_x3 = 0, sum_y3 = 0, sum_xy2 = 0, sum_x2y = 0;
    
    int n = points.size();
    for (const auto& p : points) {
        double x = p.x;
        double y = p.y;
        sum_x += x;
        sum_y += y;
        sum_x2 += x * x;
        sum_y2 += y * y;
        sum_xy += x * y;
        sum_x3 += x * x * x;
        sum_y3 += y * y * y;
        sum_xy2 += x * y * y;
        sum_x2y += x * x * y;
    }
    
    double C = n * sum_x2 - sum_x * sum_x;
    double D = n * sum_xy - sum_x * sum_y;
    double E = n * sum_x3 + n * sum_xy2 - (sum_x2 + sum_y2) * sum_x;
    double G = n * sum_y2 - sum_y * sum_y;
    double H = n * sum_x2y + n * sum_y3 - (sum_x2 + sum_y2) * sum_y;
    
    double a = (H * D - E * G) / (C * G - D * D);
    double b = (H * C - E * D) / (D * D - G * C);
    double c = -(a * sum_x + b * sum_y + sum_x2 + sum_y2) / n;
    
    center.x = -a / 2.0;
    center.y = -b / 2.0;
    radius = sqrt(center.x * center.x + center.y * center.y - c);
}

void GeometryDetector::fitLineLeastSquares(const std::vector<cv::Point>& points, cv::Vec4f& line) {
    if (points.size() < 2) return;
    fitLine(points, line, DIST_L2, 0, 0.01, 0.01);
}
