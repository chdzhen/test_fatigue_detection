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

	//��ȡ����ͼƬ�ĸ���Ȥ���򣬲�׼��������ͷ�ɼ���ͼ����е���
	//Mat imageROI;
	//imageROI = background(Rect(760, 220, 328, 500));

	cout << "Collecting calibration info\n";
	Mat temp;
	double ratio = 0;


	for (int i = 0; i < CHECK_PERIOD * 3; i++)
	{
		//��������¼�
		setMouseCallback("back", mouseEvent);

		if (exit_)
		{
			break;
		}

		//��ȡ����ͷͼ����м��
		cap >> temp;
		cv_image<bgr_pixel> cimg(temp);
		
		// Detect faces 
		std::vector<dlib::rectangle> faces = detector(cimg);

		// Find the pose of each face.
		std::vector<full_object_detection> shapes;
		for (unsigned long i = 0; i < faces.size(); ++i)
			shapes.push_back(pose_model(cimg, faces[i]));

		//û�м�⵽����������
		if (shapes.empty())
		{
			i--;
			cout << "1-�������ˣ�����" << endl;
			resize(temp, temp, Size(240, 320));
			putText(temp, "1-Change the sitting position!!!", Point(20, 20), 5,1, Scalar(255, 255, 100), 1, 8);

			//1.0
			addWeighted(imageROI, 0, temp, 1, 0, imageROI);
			imshow("back", background);
			if (cvWaitKey(1) == 27) break;
			continue;
		}
		//�궨ǰ10֡��Ҫ����׼��ʱ�䣡����
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
		//������ͷ
		cv::VideoCapture cap(0);
		if (!cap.isOpened())
		{
			cerr << "Unable to connect to camera" << endl;
			return 1;
		}

		//���ñ�����ͼƬ
		Mat background;
		namedWindow("back");

		//��ȡ��ʾ�Ĵ��ھ��
		HWND hWnd1 = (HWND)cvGetWindowHandle("back");
		HWND hParentWnd1 = ::GetParent(hWnd1);

		//���ݴ��ھ���޸Ĵ���
		::SetWindowPos(hParentWnd1, HWND_TOPMOST, 0, 0, 1920, 720, SWP_NOSIZE | SWP_NOMOVE); //�޸Ĵ���Ϊ���
																						
		long style1 = GetWindowLong(hParentWnd1, GWL_STYLE);
		style1 &= ~(WS_CAPTION);
		// style1 &= ~(WS_MAXIMIZEBOX); 
		SetWindowLong(hParentWnd1, GWL_STYLE, style1);

		//�ƶ����ڵ��̶�λ��
		::MoveWindow(hParentWnd1, 0, 0, 1920, 720, 0);

		background = imread("background.jpg");
		

		//���ñ����ĸ���Ȥ����
		Mat imageROI;
		imageROI = background(Rect(840, 400, 240,320 ));


		

		//���������������landmark��
		frontal_face_detector detector = get_frontal_face_detector();
		shape_predictor pose_model;
		deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;


		double ratio;
		Mat temp;
		
		// Grab and process frames until the main window is closed by the user.
		std::vector<double> window;


		//�������۱궨
		calibrate(window, cap, detector, pose_model, background, imageROI);
		std::vector<double> bd_temp = window;

		double runningAvg1 = computeavg(window);//��ƽ���ı���
		cout << "�궨�ľ�ֵ��" << runningAvg1 << endl;

		int num = 0;


		while (1)
		{
			//����������Ӧ
			setMouseCallback("back", mouseEvent);
			if (exit_)
			{
				break;
			}

			//������ͷ�м������������ȡ���Ƶ�
			// Grab a frame
			cap >> temp;
			cv_image<bgr_pixel> cimg(temp);

			// Detect faces 
			std::vector<dlib::rectangle> faces = detector(cimg);

			// Find the pose of each face.
			std::vector<full_object_detection> shapes;
			for (unsigned long i = 0; i < faces.size(); ++i)
				shapes.push_back(pose_model(cimg, faces[i]));

			//���û�л�ȡ�����Ƶ㣨Ҳ����û�м�⵽��������ô����ִ������ĳ������½���һ���µ�ѭ����
			if (shapes.empty())
			{
				resize(temp, temp, Size(240, 320));
				putText(temp, "2-Change the sitting position!!!", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
				
				//1.0
				addWeighted(imageROI, 0, temp, 1, 0, imageROI);
				imshow("back", background);


				if (cvWaitKey(1) == 27)break;
				cout << "�������ˣ�����" << endl;
				continue;
			}

			//���㳤��ı���
			ratio = (shapes[0].part(36) - shapes[0].part(39)).length_squared();
			ratio /= (shapes[0].part(37) - shapes[0].part(41)).length_squared();

			if (ratio - runningAvg1 > 4.5)
			{
				window.clear();
				window.push_back(ratio);
				num += 1;

				//�������ʮ֡����⵽��ƣ�ͣ���ͼƬ�����ʾ��ʻ��Ա
				if (num > 5)
				{
					cout << "˯���" << endl;

					//��ͼƬƫ��
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
				//�����⵽��ƣ�ͣ�������������ʮ֡����⵽����ʾע����Ϣ
				else
				{
					cout << "ע����Ϣ��" << endl;

					resize(temp, temp, Size(240, 320));
					putText(temp, "Pay attention to rest!!!", Point(20, 20), 5, 1, Scalar(255, 255, 100), 1, 8);
					
					//3.0
					addWeighted(imageROI, 0, temp, 1, 0, imageROI);
					imshow("back", background);
				}
			}
			else
			{
				//�����������û�м�⵽ƣ��
				cout << "�ӻ��ɣ�" << endl;

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
