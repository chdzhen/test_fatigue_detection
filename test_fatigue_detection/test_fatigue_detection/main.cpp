#include<vector>
#include<Windows.h>
#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

#define CHECK_PERIOD 10
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) 

using namespace dlib;
using namespace std;
using namespace cv;



bool exit_ = false;

//mouse event callback  
void mouseEvent(int event, int x, int y, int flags, void *param)
{
	if (event == CV_EVENT_LBUTTONDBLCLK) {
		exit_ = true;
	}
}

double computeavg(std::vector<double> &window)
{
	double avg = 0.0;
	for (std::vector<double>::iterator i = window.begin(); i != window.end(); i++)
	{
		avg += *i;
	}

	avg /= window.size();
	return avg;
}

void disp(std::vector<double> &window)
{
	cout << computeavg(window) << "-------";
	for (std::vector<double>::iterator i = window.begin(); i != window.end(); i++)
		cout << *i << "\t";
	cout << endl;
}

void calibrate(std::vector<double> &pastRatios, VideoCapture &cap, frontal_face_detector &detector, shape_predictor &pose_model,Mat background, Mat imageROI)
{

	//获取背景图片的感兴趣区域，并准备将摄像头采集的图像进行叠加
	//Mat imageROI;
	//imageROI = background(Rect(760, 220, 328, 500));

	cout << "Collecting calibration info\n";
	Mat temp;
	double ratio = 0;


	for (int i = 0; i < CHECK_PERIOD * 3; i++)
	{
		//监听鼠标事件
		setMouseCallback("back", mouseEvent);

		if (exit_)
		{
			break;
		}

		//获取摄像头图像进行检测
		cap >> temp;
		cv_image<bgr_pixel> cimg(temp);
		
		// Detect faces 
		std::vector<dlib::rectangle> faces = detector(cimg);

		// Find the pose of each face.
		std::vector<full_object_detection> shapes;
		for (unsigned long i = 0; i < faces.size(); ++i)
			shapes.push_back(pose_model(cimg, faces[i]));

		//没有检测到人脸！！！
		if (shapes.empty())
		{
			i--;
			cout << "1-调整坐姿！！！" << endl;
			resize(temp, temp, Size(240, 320));
			putText(temp, "1-Change the sitting position!!!", Point(20, 20), 5,1, Scalar(255, 255, 100), 1, 8);

			//1.0
			addWeighted(imageROI, 0, temp, 1, 0, imageROI);
			imshow("back", background);
			if (cvWaitKey(1) == 27) break;
			continue;
		}
		//标定前10帧不要，是准备时间！！！
		if (i < CHECK_PERIOD)
		{
			
			resize(temp, temp, Size(240, 320));
			putText(temp, "1-READY", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
			//1.0
			addWeighted(imageROI, 0, temp, 1, 0, imageROI);
			imshow("back", background);
			if (cvWaitKey(1) == 27) break;
			continue;
		}

		ratio = (shapes[0].part(36) - shapes[0].part(39)).length_squared();
		ratio /= (shapes[0].part(37) - shapes[0].part(41)).length_squared();

		pastRatios.push_back(ratio);
		disp(pastRatios);


		resize(temp, temp, Size(240, 320));
		putText(temp, "1-normal", Point(20, 20), 5,1, Scalar(255, 255, 100), 1, 8);
		
		//2.0
		addWeighted(imageROI, 0, temp, 1, 0, imageROI);
		imshow("back", background);


		if (cvWaitKey(1) == 27)break;
	}
	
	cout << "------------------DONE CALIBRATING------------------\n";
}




int main()
{
	try
	{
		//打开摄像头
		cv::VideoCapture cap(0);
		if (!cap.isOpened())
		{
			cerr << "Unable to connect to camera" << endl;
			return 1;
		}

		//设置背景的图片
		Mat background;
		namedWindow("back");

		//获取显示的窗口句柄
		HWND hWnd1 = (HWND)cvGetWindowHandle("back");
		HWND hParentWnd1 = ::GetParent(hWnd1);

		//根据窗口句柄修改窗口
		::SetWindowPos(hParentWnd1, HWND_TOPMOST, 0, 0, 1920, 720, SWP_NOSIZE | SWP_NOMOVE); //修改窗口为最顶部
																						
		long style1 = GetWindowLong(hParentWnd1, GWL_STYLE);
		style1 &= ~(WS_CAPTION);
		// style1 &= ~(WS_MAXIMIZEBOX); 
		SetWindowLong(hParentWnd1, GWL_STYLE, style1);

		//移动窗口到固定位置
		::MoveWindow(hParentWnd1, 0, 0, 1920, 720, 0);

		background = imread("background.jpg");
		

		//设置背景的感兴趣区域
		Mat imageROI;
		imageROI = background(Rect(840, 400, 240,320 ));


		

		//导入人脸检测器及landmark点
		frontal_face_detector detector = get_frontal_face_detector();
		shape_predictor pose_model;
		deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;


		double ratio;
		Mat temp;
		
		// Grab and process frames until the main window is closed by the user.
		std::vector<double> window;


		//进行人眼标定
		calibrate(window, cap, detector, pose_model, background, imageROI);
		std::vector<double> bd_temp = window;

		double runningAvg1 = computeavg(window);//求平均的比率
		cout << "标定的均值：" << runningAvg1 << endl;

		int num = 0;


		while (1)
		{
			//监听鼠标的响应
			setMouseCallback("back", mouseEvent);
			if (exit_)
			{
				break;
			}

			//从摄像头中检测人脸，并获取姿势点
			// Grab a frame
			cap >> temp;
			cv_image<bgr_pixel> cimg(temp);

			// Detect faces 
			std::vector<dlib::rectangle> faces = detector(cimg);

			// Find the pose of each face.
			std::vector<full_object_detection> shapes;
			for (unsigned long i = 0; i < faces.size(); ++i)
				shapes.push_back(pose_model(cimg, faces[i]));

			//如果没有获取到姿势点（也就是没有检测到人脸，那么将不执行下面的程序，重新进行一次新的循环）
			if (shapes.empty())
			{
				resize(temp, temp, Size(240, 320));
				putText(temp, "2-Change the sitting position!!!", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
				
				//1.0
				addWeighted(imageROI, 0, temp, 1, 0, imageROI);
				imshow("back", background);


				if (cvWaitKey(1) == 27)break;
				cout << "调整坐姿！！！" << endl;
				continue;
			}

			//计算长宽的比率
			ratio = (shapes[0].part(36) - shapes[0].part(39)).length_squared();
			ratio /= (shapes[0].part(37) - shapes[0].part(41)).length_squared();

			if (ratio - runningAvg1 > 4.5)
			{
				window.clear();
				window.push_back(ratio);
				num += 1;

				//如果连续十帧都检测到了疲劳，将图片变红提示驾驶人员
				if (num > 5)
				{
					cout << "睡会吧" << endl;

					//让图片偏红
					for (int i = 0;i < temp.rows;i++)
					{
						for (int j = 0;j < temp.cols;j++)
						{
							temp.at<Vec3b>(i, j)[2] = 255;
						}
					}
					Beep(5000, 100);
					
					resize(temp, temp, Size(240, 320));
					putText(temp, "2- Sleep for a while!!!", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
					
					//2.0
					addWeighted(imageROI, 0, temp, 1, 0, imageROI);
					imshow("back", background);
				}
				//如果检测到了疲劳，但不是连续的十帧都检测到，提示注意休息
				else
				{
					cout << "注意休息！" << endl;

					resize(temp, temp, Size(240, 320));
					putText(temp, "Pay attention to rest!!!", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
					
					//3.0
					addWeighted(imageROI, 0, temp, 1, 0, imageROI);
					imshow("back", background);
				}
			}
			else
			{
				//这种情况就是没有检测到疲劳
				cout << "加会班吧？" << endl;

				resize(temp, temp, Size(240, 320));
				putText(temp, "2- normal", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
				
				//4.0
				addWeighted(imageROI, 0, temp, 1, 0, imageROI);
				imshow("back", background);
				num = 0;
			}

			
			if (cvWaitKey(1) == 27)break;

		}
		
	}
	catch (serialization_error& e)
	{
		cout << "Need landmarks.dat file first" << endl;
		cout << endl << e.what() << endl;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}
}
