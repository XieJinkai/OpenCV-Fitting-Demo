#include <windows.h>
#include <commdlg.h>
#include <opencv2/opencv.hpp>
#include "GeometryDetector.h"
#include <string>
#include <vector>
#include <iostream>

// Link with comdlg32.lib for GetOpenFileName
#pragma comment(lib, "comdlg32.lib")

using namespace cv;
using namespace std;

// Global Variables
Mat g_src, g_display;
vector<Point> g_selectedPoints;
string g_currentWindowName = "几何图形检测";

struct AppSettings {
    // Hough Circle
    double dp = 1.2;
    double minDist = 30.0;
    double param1 = 100.0;
    double param2 = 30.0;
    int minRadius = 10;
    int maxRadius = 200;

    // Hough Line
    double rho = 1.0;
    double theta = CV_PI / 180;
    int threshold = 50;
    double minLineLength = 50.0;
    double maxLineGap = 10.0;
} g_settings;

// Control IDs
#define ID_BTN_LOAD 101
#define ID_BTN_HOUGH_CIRCLE 102
#define ID_BTN_FIT_CIRCLE 103
#define ID_BTN_HOUGH_LINE 104
#define ID_BTN_FIT_LINE 105
#define ID_BTN_SETTINGS 106
#define ID_BTN_CLEAR 107
#define ID_BTN_HELP 108

// Settings Dialog Controls
#define ID_EDIT_DP 201
#define ID_EDIT_MINDIST 202
#define ID_EDIT_PARAM1 203
#define ID_EDIT_PARAM2 204
#define ID_EDIT_MINRAD 205
#define ID_EDIT_MAXRAD 206
#define ID_EDIT_RHO 207
#define ID_EDIT_THETA 208
#define ID_EDIT_THRESH 209
#define ID_EDIT_MINLEN 210
#define ID_EDIT_MAXGAP 211
#define ID_BTN_SAVE_SETTINGS 212

// Mouse Callback for OpenCV Window
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        g_selectedPoints.push_back(Point(x, y));
        circle(g_display, Point(x, y), 3, Scalar(0, 0, 255), -1);
        imshow(g_currentWindowName, g_display);
    }
    else if (event == EVENT_RBUTTONDOWN) {
        // Right click to clear points but keep image
        if (!g_src.empty()) {
            g_display = g_src.clone();
            g_selectedPoints.clear();
            imshow(g_currentWindowName, g_display);
        }
    }
}

// Settings Window Procedure
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdits[11];
    
    switch (msg) {
    case WM_CREATE:
        CreateWindow("STATIC", "霍夫圆检测参数:", WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, hwnd, NULL, NULL, NULL);
        
        CreateWindow("STATIC", "dp:", WS_VISIBLE | WS_CHILD, 10, 40, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[0] = CreateWindow("EDIT", to_string(g_settings.dp).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 40, 100, 20, hwnd, (HMENU)ID_EDIT_DP, NULL, NULL);
        
        CreateWindow("STATIC", "minDist:", WS_VISIBLE | WS_CHILD, 10, 65, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[1] = CreateWindow("EDIT", to_string(g_settings.minDist).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 65, 100, 20, hwnd, (HMENU)ID_EDIT_MINDIST, NULL, NULL);

        CreateWindow("STATIC", "param1:", WS_VISIBLE | WS_CHILD, 10, 90, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[2] = CreateWindow("EDIT", to_string(g_settings.param1).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 90, 100, 20, hwnd, (HMENU)ID_EDIT_PARAM1, NULL, NULL);

        CreateWindow("STATIC", "param2:", WS_VISIBLE | WS_CHILD, 10, 115, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[3] = CreateWindow("EDIT", to_string(g_settings.param2).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 115, 100, 20, hwnd, (HMENU)ID_EDIT_PARAM2, NULL, NULL);

        CreateWindow("STATIC", "minRadius:", WS_VISIBLE | WS_CHILD, 10, 140, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[4] = CreateWindow("EDIT", to_string(g_settings.minRadius).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 140, 100, 20, hwnd, (HMENU)ID_EDIT_MINRAD, NULL, NULL);

        CreateWindow("STATIC", "maxRadius:", WS_VISIBLE | WS_CHILD, 10, 165, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[5] = CreateWindow("EDIT", to_string(g_settings.maxRadius).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 165, 100, 20, hwnd, (HMENU)ID_EDIT_MAXRAD, NULL, NULL);

        CreateWindow("STATIC", "霍夫直线检测参数:", WS_VISIBLE | WS_CHILD, 220, 10, 200, 20, hwnd, NULL, NULL, NULL);

        CreateWindow("STATIC", "rho:", WS_VISIBLE | WS_CHILD, 220, 40, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[6] = CreateWindow("EDIT", to_string(g_settings.rho).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 310, 40, 100, 20, hwnd, (HMENU)ID_EDIT_RHO, NULL, NULL);

        CreateWindow("STATIC", "theta (rad):", WS_VISIBLE | WS_CHILD, 220, 65, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[7] = CreateWindow("EDIT", to_string(g_settings.theta).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 310, 65, 100, 20, hwnd, (HMENU)ID_EDIT_THETA, NULL, NULL);

        CreateWindow("STATIC", "threshold:", WS_VISIBLE | WS_CHILD, 220, 90, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[8] = CreateWindow("EDIT", to_string(g_settings.threshold).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 310, 90, 100, 20, hwnd, (HMENU)ID_EDIT_THRESH, NULL, NULL);

        CreateWindow("STATIC", "minLineLen:", WS_VISIBLE | WS_CHILD, 220, 115, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[9] = CreateWindow("EDIT", to_string(g_settings.minLineLength).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 310, 115, 100, 20, hwnd, (HMENU)ID_EDIT_MINLEN, NULL, NULL);

        CreateWindow("STATIC", "maxLineGap:", WS_VISIBLE | WS_CHILD, 220, 140, 80, 20, hwnd, NULL, NULL, NULL);
        hEdits[10] = CreateWindow("EDIT", to_string(g_settings.maxLineGap).c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 310, 140, 100, 20, hwnd, (HMENU)ID_EDIT_MAXGAP, NULL, NULL);

        CreateWindow("BUTTON", "保存并关闭", WS_VISIBLE | WS_CHILD, 150, 220, 120, 30, hwnd, (HMENU)ID_BTN_SAVE_SETTINGS, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_SAVE_SETTINGS) {
            char buffer[100];
            GetWindowText(hEdits[0], buffer, 100); g_settings.dp = atof(buffer);
            GetWindowText(hEdits[1], buffer, 100); g_settings.minDist = atof(buffer);
            GetWindowText(hEdits[2], buffer, 100); g_settings.param1 = atof(buffer);
            GetWindowText(hEdits[3], buffer, 100); g_settings.param2 = atof(buffer);
            GetWindowText(hEdits[4], buffer, 100); g_settings.minRadius = atoi(buffer);
            GetWindowText(hEdits[5], buffer, 100); g_settings.maxRadius = atoi(buffer);

            GetWindowText(hEdits[6], buffer, 100); g_settings.rho = atof(buffer);
            GetWindowText(hEdits[7], buffer, 100); g_settings.theta = atof(buffer);
            GetWindowText(hEdits[8], buffer, 100); g_settings.threshold = atoi(buffer);
            GetWindowText(hEdits[9], buffer, 100); g_settings.minLineLength = atof(buffer);
            GetWindowText(hEdits[10], buffer, 100); g_settings.maxLineGap = atof(buffer);

            DestroyWindow(hwnd);
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindow("BUTTON", "加载图片", WS_VISIBLE | WS_CHILD, 20, 20, 180, 30, hwnd, (HMENU)ID_BTN_LOAD, NULL, NULL);
        CreateWindow("BUTTON", "设置", WS_VISIBLE | WS_CHILD, 220, 20, 180, 30, hwnd, (HMENU)ID_BTN_SETTINGS, NULL, NULL);
        
        CreateWindow("BUTTON", "霍夫圆检测", WS_VISIBLE | WS_CHILD, 20, 70, 180, 30, hwnd, (HMENU)ID_BTN_HOUGH_CIRCLE, NULL, NULL);
        CreateWindow("BUTTON", "最小二乘法拟合圆", WS_VISIBLE | WS_CHILD, 220, 70, 180, 30, hwnd, (HMENU)ID_BTN_FIT_CIRCLE, NULL, NULL);
        
        CreateWindow("BUTTON", "霍夫直线检测", WS_VISIBLE | WS_CHILD, 20, 110, 180, 30, hwnd, (HMENU)ID_BTN_HOUGH_LINE, NULL, NULL);
        CreateWindow("BUTTON", "最小二乘法拟合直线", WS_VISIBLE | WS_CHILD, 220, 110, 180, 30, hwnd, (HMENU)ID_BTN_FIT_LINE, NULL, NULL);
        
        CreateWindow("BUTTON", "清除显示", WS_VISIBLE | WS_CHILD, 20, 160, 180, 30, hwnd, (HMENU)ID_BTN_CLEAR, NULL, NULL);
        CreateWindow("BUTTON", "帮助", WS_VISIBLE | WS_CHILD, 220, 160, 180, 30, hwnd, (HMENU)ID_BTN_HELP, NULL, NULL);
        
        CreateWindow("STATIC", "说明:\n1. 加载图片\n2. 霍夫检测: 直接点击按钮\n3. 拟合: 自动提取图像边缘点进行拟合", WS_VISIBLE | WS_CHILD, 20, 210, 380, 100, hwnd, NULL, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_LOAD: {
            char filename[MAX_PATH] = { 0 };
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = "图像文件\0*.jpg;*.png;*.bmp\0所有文件\0*.*\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            
            if (GetOpenFileName(&ofn)) {
                g_src = imread(filename);
                if (!g_src.empty()) {
                    g_display = g_src.clone();
                    g_selectedPoints.clear();
                    namedWindow(g_currentWindowName, WINDOW_AUTOSIZE);
                    setMouseCallback(g_currentWindowName, onMouse);
                    imshow(g_currentWindowName, g_display);
                }
            }
        } break;

        case ID_BTN_SETTINGS: {
            WNDCLASS wc = { 0 };
            wc.lpfnWndProc = SettingsWndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = "SettingsWindowClass";
            RegisterClass(&wc);
            
            CreateWindow("SettingsWindowClass", "算法参数设置", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 450, 300, hwnd, NULL, GetModuleHandle(NULL), NULL);
        } break;

        case ID_BTN_HOUGH_CIRCLE: {
            if (g_src.empty()) break;
            g_display = g_src.clone();
            vector<Vec3f> circles;
            GeometryDetector::detectHoughCircles(g_src, circles, g_settings.dp, g_settings.minDist, g_settings.param1, g_settings.param2, g_settings.minRadius, g_settings.maxRadius);
            for (const auto& c : circles) {
                circle(g_display, Point(cvRound(c[0]), cvRound(c[1])), cvRound(c[2]), Scalar(0, 255, 0), 2);
                circle(g_display, Point(cvRound(c[0]), cvRound(c[1])), 2, Scalar(0, 0, 255), 3);
            }
            imshow(g_currentWindowName, g_display);
        } break;

        case ID_BTN_HOUGH_LINE: {
            if (g_src.empty()) break;
            g_display = g_src.clone();
            vector<Vec4i> lines;
            GeometryDetector::detectHoughLines(g_src, lines, g_settings.rho, g_settings.theta, g_settings.threshold, g_settings.minLineLength, g_settings.maxLineGap);
            for (const auto& l : lines) {
                line(g_display, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 0, 0), 2, LINE_AA);
            }
            imshow(g_currentWindowName, g_display);
        } break;

        case ID_BTN_FIT_CIRCLE: {
            if (g_src.empty()) break;
            g_display = g_src.clone();
            Mat gray, edges;
            if (g_src.channels() == 3) cvtColor(g_src, gray, COLOR_BGR2GRAY); else gray = g_src.clone();
            GaussianBlur(gray, gray, Size(9, 9), 2, 2);
            Canny(gray, edges, 50, 150);
            vector<vector<Point>> contours;
            findContours(edges, contours, RETR_LIST, CHAIN_APPROX_NONE);
            for (const auto& cnt : contours) {
                if (cnt.size() < 20) continue;
                Point2f center; float radius;
                GeometryDetector::fitCircleLeastSquares(cnt, center, radius);
                if (radius <= 0) continue;
                double err = 0.0;
                for (const auto& p : cnt) {
                    err += fabs(norm(Point2f((float)p.x, (float)p.y) - center) - radius);
                }
                err /= cnt.size();
                if (err < 2.5 && radius >= g_settings.minRadius && radius <= g_settings.maxRadius) {
                    circle(g_display, center, cvRound(radius), Scalar(255, 255, 0), 2);
                    circle(g_display, center, 3, Scalar(0, 0, 255), -1);
                }
            }
            imshow(g_currentWindowName, g_display);
        } break;

        case ID_BTN_FIT_LINE: {
            if (g_src.empty()) break;
            g_display = g_src.clone();
            Mat gray, edges;
            if (g_src.channels() == 3) cvtColor(g_src, gray, COLOR_BGR2GRAY); else gray = g_src.clone();
            GaussianBlur(gray, gray, Size(9, 9), 2, 2);
            Canny(gray, edges, 50, 150);
            vector<Vec4i> segments;
            HoughLinesP(edges, segments, g_settings.rho, g_settings.theta, g_settings.threshold, g_settings.minLineLength, g_settings.maxLineGap);
            vector<Point> allPts;
            findNonZero(edges, allPts);
            int w = g_src.cols, h = g_src.rows;
            for (const auto& l : segments) {
                int x1 = l[0], y1 = l[1], x2 = l[2], y2 = l[3];
                int minx = max(0, min(x1, x2) - 5);
                int miny = max(0, min(y1, y2) - 5);
                int maxx = min(w - 1, max(x1, x2) + 5);
                int maxy = min(h - 1, max(y1, y2) + 5);
                vector<Point> pts;
                for (const auto& p : allPts) {
                    if (p.x < minx || p.x > maxx || p.y < miny || p.y > maxy) continue;
                    double d = GeometryDetector::pointToLineDistance(Point2f((float)p.x, (float)p.y), Vec4f((float)x1, (float)y1, (float)x2, (float)y2));
                    if (d < 2.0) pts.push_back(p);
                }
                if (pts.size() < 2) continue;
                Vec4f lineParam;
                GeometryDetector::fitLineLeastSquares(pts, lineParam);
                double vx = lineParam[0], vy = lineParam[1];
                double x0 = lineParam[2], y0 = lineParam[3];
                Point pt1(cvRound(x0 - 1000 * vx), cvRound(y0 - 1000 * vy));
                Point pt2(cvRound(x0 + 1000 * vx), cvRound(y0 + 1000 * vy));
                line(g_display, pt1, pt2, Scalar(0, 255, 255), 2, LINE_AA);
            }
            imshow(g_currentWindowName, g_display);
        } break;

        case ID_BTN_CLEAR: {
            if (!g_src.empty()) {
                g_display = g_src.clone();
                g_selectedPoints.clear();
                imshow(g_currentWindowName, g_display);
            }
        } break;

        case ID_BTN_HELP:
            MessageBox(hwnd, "左键点击选择点。\n右键点击清除点。\n使用设置按钮调整霍夫参数。", "帮助", MB_OK);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main() {
    // Create Win32 Window
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ControlPanelClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("ControlPanelClass", "控制面板", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        100, 100, 450, 400, NULL, NULL, GetModuleHandle(NULL), NULL);

    // Win32 Message Loop with OpenCV support
    MSG msg = { 0 };
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // OpenCV event processing
            if (!g_display.empty()) {
                // Only call waitKey if an image is displayed, to handle mouse events on that window
                int key = waitKey(10);
                if (key == 27) break; // ESC to exit
            } else {
                Sleep(10); // Avoid high CPU usage when no image
            }
        }
    }
    
    return 0;
}
