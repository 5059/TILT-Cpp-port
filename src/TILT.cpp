#include "opencv2/opencv.hpp"
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
using namespace std;
using namespace cv;
// ----------------------------------------------------------------
// �������� ������ �� ��� ��� ���������� ����, �������� ������� :)
// ----------------------------------------------------------------
#define COUT_VAR(x) cout << #x"=" << x << endl;
#define SHOW_IMG(x) namedWindow(#x);imshow(#x,x);waitKey(20);
// ----------------------------------------------------------------
// �������� �� ��������� 2
// ----------------------------------------------------------------
double log2( double n )  
{   
	return log( n ) / log( 2.0 );  
}
// ----------------------------------------------------------------
// ����������������� �������������
// ----------------------------------------------------------------
Mat getTau(double theta,double t)
{
	Mat Tau = (Mat_<double>(3, 3) << 
		cos(theta), -sin(theta), 0,
		sin(theta),  cos(theta), 0,
		0		  ,			  0, 1);

	Mat s= (Mat_<double>(2, 2) << 
		1,t,
		0,1);

	Mat Tau22=Tau(Range(0,2),Range(0,2));
	Tau22=Tau22*s;

	return Tau;
}
// ----------------------------------------------------------------
// ���������� ����������� img �������������� Tau
// src_pts - ����� ��������������, ��������������� ������� ��������
//	src_pts[0]=Point2f(p1.x,p1.y);
//	src_pts[1]=Point2f(p2.x,p1.y);
//	src_pts[2]=Point2f(p2.x,p2.y);
//	src_pts[3]=Point2f(p1.x,p2.y);
// ��� p1 - �������-����� �����, p2 - ������-������
// ----------------------------------------------------------------
void transformImg(Mat& img,vector<Point2f>& src_pts,Mat& Tau,Mat& dst)
{	
	vector<Point2f> pts(4);
	double w=fabs(src_pts[0].x-src_pts[2].x);
	double h=fabs(src_pts[0].y-src_pts[2].y);
	pts.assign(src_pts.begin(),src_pts.end());

	// ����� �����

	Point2f center=(pts[0]+pts[2])*0.5;

	Mat tmp(4,1,CV_64FC2);

	for(int i=0;i<4;i++)
	{
		tmp.at<Vec2d>(i)[0]=pts[i].x-center.x;
		tmp.at<Vec2d>(i)[1]=pts[i].y-center.y;
	}

	perspectiveTransform(tmp,tmp,Tau);

	for(int i=0;i<4;i++)
	{
		pts[i].x=tmp.at<Vec2d>(i)[0]+w/2;
		pts[i].y=tmp.at<Vec2d>(i)[1]+h/2;
	}

	dst=Mat::zeros(h,w,CV_64FC1);
	Mat trans=getPerspectiveTransform(src_pts,pts);
	warpPerspective(img,dst,trans,Size(w,h),INTER_LANCZOS4,BORDER_TRANSPARENT);
}
// ----------------------------------------------------------------
// ���������� ���� ��� ����, �� �� ���������� ��������������, � �� �� �������
// ----------------------------------------------------------------
void transformImg(Mat& img,vector<Point2f>& src_pts,double theta,double t,Mat& dst)
{
	Mat Tau=getTau(theta,t);

	vector<Point2f> pts(4);
	double w=fabs(src_pts[0].x-src_pts[2].x);
	double h=fabs(src_pts[0].y-src_pts[2].y);
	pts.assign(src_pts.begin(),src_pts.end());
	// ����� �����
	Point2f center=(pts[0]+pts[2])*0.5;

	Mat tmp(4,1,CV_64FC2);

	for(int i=0;i<4;i++)
	{
		tmp.at<Vec2d>(i)[0]=pts[i].x-center.x;
		tmp.at<Vec2d>(i)[1]=pts[i].y-center.y;
	}

	perspectiveTransform(tmp,tmp,Tau);

	for(int i=0;i<4;i++)
	{
		pts[i].x=tmp.at<Vec2d>(i)[0]+w/2;
		pts[i].y=tmp.at<Vec2d>(i)[1]+h/2;
	}

	dst=Mat::zeros(h,w,CV_64FC1);
	Mat trans=getPerspectiveTransform(src_pts,pts);
	warpPerspective(img,dst,trans,Size(w,h),INTER_LANCZOS4,BORDER_TRANSPARENT);
}
// ----------------------------------------------------------------
// ��������� �������������� � ������ �����
// ----------------------------------------------------------------
void transformPts(vector<Point2f>& src_pts,Mat& Tau,vector<Point2f>& dst_pts)
{
	vector<Point2f> pts(4);
	double w=fabs(src_pts[0].x-src_pts[2].x);
	double h=fabs(src_pts[0].y-src_pts[2].y);
	pts.assign(src_pts.begin(),src_pts.end());
	// ����� �����
	Point2f center=(pts[0]+pts[2])*0.5;
	
	Mat tmp(4,1,CV_64FC2);

	for(int i=0;i<4;i++)
	{
		tmp.at<Vec2d>(i)[0]=pts[i].x-center.x;
		tmp.at<Vec2d>(i)[1]=pts[i].y-center.y;
	}
	Mat Tau_inv=Tau.inv();
	perspectiveTransform(tmp,tmp,Tau_inv);

	for(int i=0;i<4;i++)
	{
		pts[i].x=tmp.at<Vec2d>(i)[0]+center.x;
		pts[i].y=tmp.at<Vec2d>(i)[1]+center.y;
	}
	dst_pts.assign(pts.begin(),pts.end());
}
//----------------------------------------------------------
// ������� ��������� (������� �� ������������)
//----------------------------------------------------------
double getF(Mat& img)
{
	Mat tmp=img/norm(img);
	Mat A, w, u, vt;
	SVD::compute(img, w, u, vt,SVD::NO_UV);
	return sum(w)[0];
}
//----------------------------------------------------------
// ������� ����������� �� ������� ��������������
// ����������� ����������� �� ��������� ������� ��������������
// ������������� �� ��������: 1 ������� - ���� �����������.
// src_pts - ����� ��������������, ��������������� ������� ��������
//	src_pts[0]=Point2f(p1.x,p1.y);
//	src_pts[1]=Point2f(p2.x,p1.y);
//	src_pts[2]=Point2f(p2.x,p2.y);
//	src_pts[3]=Point2f(p1.x,p2.y);
// Tau - ������� ��������������.
//----------------------------------------------------------
void getJacobian(Mat& img,vector<Point2f>& src_pts,Mat& Tau,Mat& J_vec)
{
	// ���������� �������� ������� ��������������
	int p=8;
	// ������� ����� ��������, ��� ���������� ����������� ��������� �������.
	double epsilon=1e-3;
	Mat dst;
	Mat Ip,Im,Tau_p,Tau_m,diff;
	// ������� ������� ��������
	int n=fabs(src_pts[0].x-src_pts[2].x);
	int m=fabs(src_pts[0].y-src_pts[2].y);
	// ������� ����������
	J_vec=Mat::zeros(n*m,p,CV_64FC1);
	// ---------------------------------------------------------------------
	// ��������� 8 ������� ����������� �� ��������� ������� ��������������.
	// ������� ������� ����� 1
	// ---------------------------------------------------------------------
	for(int i=0;i<p;i++)
	{
		Tau_p=Tau.clone();
		Tau_m=Tau.clone();
		Tau_p.at<double>(i)+=epsilon;
		Tau_m.at<double>(i)-=epsilon;
		transformImg(img,src_pts,Tau_p,Ip);
		transformImg(img,src_pts,Tau_m,Im);
		// ������� �����������
		diff=Ip-Im;
		// �������� ���.
		cv::GaussianBlur(diff,diff,Size(3,3),2);
		diff=diff.reshape(1,m*n);

		diff=diff/(2*epsilon);

		diff.copyTo(J_vec.col(i));
	}
}
//--------------------------------
// ���������� ������������� �� �������
//--------------------------------
double Garrot_shrink(double d,double T)
{
	double res;
	if(fabs(d)>T)
	{
		res=d-((T*T)/d);
	}
	else
	{
		res=0;
	}

	return res;
}
//----------------------------------------------------------
// ������� ���������� ������ ������������� ��� ������
//----------------------------------------------------------
Mat Smu(Mat& x,double mu)
{
	Mat res(x.size(),CV_64FC1);
	for(int i=0;i<x.rows;i++)
	{
		for(int j=0;j<x.cols;j++)
		{
			res.at<double>(i,j)=Garrot_shrink(x.at<double>(i,j),mu);
		}
	}
	return res;
}
//----------------------------------------------------------
// ������� �������
//----------------------------------------------------------
Mat TILT(Mat& I,Mat& Tau_0,vector<Point2f>& src_pts,double Lambda)
{
	int maxcicles=100;
	int cicle_counter1=0;
	int cicle_counter2=0;
	bool converged1=0;
	bool converged2=0;
	Mat I_tau;
	Mat Tau=Tau_0.clone();
	Mat deltaTau_prev;
	Mat deltaTau;

	double F_prev=DBL_MAX;

	int n=fabs(src_pts[0].x-src_pts[2].x);
	int m=fabs(src_pts[0].y-src_pts[2].y);
	int p=8;

	double mu;
	double rho=1.25; // ������ ���� ����� �������������
	Mat J_vec;

	while(!converged1)
	{
		// ��� 1 �������� ������������� ��������������� ����������� � �������
		transformImg(I,src_pts,Tau,I_tau);
		getJacobian(I,src_pts,Tau,J_vec);

		// ��� 2 ������� ��������������� ������ �����������
		Mat E=Mat::zeros(m, n,CV_64FC1);
		Mat A=Mat::zeros(m, n,CV_64FC1);

		// ��������� ����������� ������ ����������
		mu=1.25/norm(I_tau);


		deltaTau=Mat::zeros(p, 1,CV_64FC1);
		deltaTau_prev=Mat::zeros(p, 1,CV_64FC1);

		Mat Y=I_tau.clone();
		Mat I0;

		cicle_counter2=0;
		converged2=0;

		Mat J_vec_inv=J_vec.inv(DECOMP_SVD);

		while(!converged2)
		{
			Mat t1=J_vec*deltaTau;
			Mat tmp;
			Mat U, Sigma,V;
			SVD::compute(I_tau+t1.reshape(1,m)-E+Y/mu,Sigma,U,V);
			Sigma=Smu(Sigma,1.0/mu);// ������� ����������� ��������

			// �������� ������� Sigma (��-��������� ����������� �����, ��������� ����)
			// ��� ��� ������� ������� ��� �� � ���� �������, � ��� ����� ������������ �������
			Mat W=Mat::zeros(Sigma.rows,Sigma.rows,CV_64FC1);
			Sigma.copyTo(W.diag());

			I0=U*W*V;// ������� �������

			tmp=I_tau+t1.reshape(1,m)-I0+Y/mu;
			E=Smu(tmp,Lambda/mu);

			tmp=(-I_tau+I0+E-Y/mu);
			deltaTau=J_vec_inv*tmp.reshape(1,m*n);

			tmp=J_vec*deltaTau;			
			Y+=mu*(I_tau+tmp.reshape(1,m)-I0-E);

			mu*=rho;

			// ����������� �� ���������� ���������� ������
			cicle_counter2++;
			if(cicle_counter2>maxcicles){break;}

			// ��������, ������� ��� ���.
			double m,M;
			cv::minMaxLoc(abs(deltaTau_prev-deltaTau),&m,&M);	
			if(cicle_counter2>1 && M<1e-3)
			{
				converged2=true;
			}

			// ��� ����� ����������� �������� ��������
			deltaTau_prev=deltaTau.clone();
		}
		// ��� 3 �������� ��������������
		for(int i=0;i<p;i++)
		{
			Tau.at<double>(i)+=deltaTau.at<double>(i);
		}

		Mat dst;
		transformImg(I,src_pts,Tau,dst);

		double F=getF(dst);
		double perf=F_prev-F;
		F_prev=F;
		// ������� �������
		COUT_VAR(F);

		// ----------------------------------
		// �����������
		dst.convertTo(dst,CV_8UC1,255);
		imshow("TILT",dst);
		waitKey(10);
		// ----------------------------------

		// ��������, ������� ��� ���.
		if(perf<=0){converged1=true;}

		// ����������� �� ���������� ������� ������
		cicle_counter1++;
		if(cicle_counter1>maxcicles){break;}

	}
	return Tau;
}
//----------------------------------------------------------
// ���� ��������� �����������
//----------------------------------------------------------
void getInitialGuess(Mat& img,vector<Point2f>& region,double& theta_opt,double& t_opt)
{
	double angle_min=-CV_PI/6;
	double angle_max= CV_PI/6;
	double n_angle_steps=10;

	double t_min=-0.5;
	double t_max=0.5;
	double n_t_steps=10;

	Mat dst;
	double F_min=DBL_MAX;

	double angle_step=(angle_max-angle_min)/n_angle_steps;
	double t_step=(t_max-t_min)/n_t_steps;

	double theta=angle_min;
	double t=t_min;

	for(double i=0;i<n_angle_steps;i++)
	{	
		t=t_min;
		for(double j=0;j<n_t_steps;j++)
		{
			transformImg(img,region,theta,t,dst);
			double F=getF(dst); // ������� �������
			cout << F << endl;
			if(F<F_min)
			{
				F_min=F; 
				theta_opt=theta;
				t_opt=t;
			}
			t+=t_step;
		}
		theta=theta+angle_step;
	}
}
//----------------------------------------------------------
// ������� ��������� ��������� ������� (2 ������� �����)
// ������� 2 ����� � ������ ������
//----------------------------------------------------------
vector<Point2f> pt;
void pp_MouseCallback(int event, int x, int y, int /*flags*/, void* /*param*/)
{
	if(event == cv::EVENT_LBUTTONDOWN)
	{
		pt.push_back(Point2f(x,y));
	}
}
//----------------------------------------------------------
// ������ ������� �� 4 ������
//----------------------------------------------------------
void DrawRegion(Mat& img,vector<Point2f>& region,Scalar color)
{
	for(int i = 0; i < 4; i++ )
	{
		line(img, region[i], region[(i+1)%4], color, 1, cv::LINE_AA);
	}
}

//----------------------------------------------------------
// 
//----------------------------------------------------------
void PutBanner(Mat& img,Mat& banner,Mat& Tau,vector<Point2f>& region_tau)
{		
	Point2f center=(region_tau[0]+region_tau[2])*0.5;

	Rect roi=cv::boundingRect(region_tau);

	double w=roi.width;
	double h=roi.height;
	vector<Point2f> pts(w*h);
	vector<Point2f> dst_pts(w*h);

	Mat tmp(w*h,1,CV_64FC2);
	int ind=0;

	for(int i=roi.y;i<roi.y+roi.height;i++)
	{
		for(int j=roi.x;j<roi.x+roi.width;j++)
		{
			tmp.at<Vec2d>(ind)[0]=j-roi.x-roi.width/2.0;
			tmp.at<Vec2d>(ind)[1]=i-roi.y-roi.height/2.0;
			ind++;
		}
	}
	
	Mat Tau_inv=Tau.inv();
	perspectiveTransform(tmp,tmp,Tau);

	ind=0;
	for(int i=roi.y;i<roi.y+roi.height;i++)
	{
		for(int j=roi.x;j<roi.x+roi.width;j++)
		{
			Vec2d val=tmp.at<Vec2d>(ind);
			pts[ind].x=val[0]+banner.cols/2.0;
			pts[ind].y=val[1]+banner.rows/2.0;
			ind++;
		}
	}

	dst_pts.assign(pts.begin(),pts.end());

	ind=0;

	for(int i=roi.y;i<roi.y+roi.height;i++)
	{
		for(int j=roi.x;j<roi.x+roi.width;j++)
		{
			if(dst_pts[ind].y>0 && dst_pts[ind].y<banner.rows && dst_pts[ind].x >0 && dst_pts[ind].x <banner.cols)
			{
				img.at<Vec3b>(i,j)=banner.at<Vec3b>(dst_pts[ind].y,dst_pts[ind].x);
			}
			ind++;
		}
	}

}
//----------------------------------------------------------
// ����� �����
//----------------------------------------------------------
int main(int argc, char* argv[])
{
	// ������� �����������
	Mat img_c,img;
	//---------------------------------------------
	// �������������
	//---------------------------------------------	
	namedWindow("�������� �����������");
	namedWindow("��������� �����������");
	namedWindow("Banner");
	namedWindow("TILT");

	// ������ �����������
	img_c=imread("building3.jpg",1);

	// ���� ����� �������
	imshow("�������� �����������", img_c);
	waitKey(10);
	setMouseCallback("�������� �����������",pp_MouseCallback,0);
	waitKey(0);

	// ��������� � ������ ������ ����������� � ��������� ������
	cv::cvtColor(img_c,img,cv::COLOR_BGR2GRAY);
	// � �������� 0-1
	img.convertTo(img,CV_64FC1,1.0/255.0);

	// ��������� ������ � ���������� �������
	vector<Point2f> region(4);
	region[0]=Point2f(pt[0].x,pt[0].y);
	region[1]=Point2f(pt[1].x,pt[0].y);
	region[2]=Point2f(pt[1].x,pt[1].y);
	region[3]=Point2f(pt[0].x,pt[1].y);
	// ������ �����, ��������� �������������
	//DrawRegion(img_c, region,Scalar(0, 255, 0));

	imshow("�������� �����������", img_c);

	// ���� ��������� ����������� (����� �� ������, � ������ ������ � ������� ��������)
	// �� ���, �������� �����.

	double theta_opt=0;
	double t_opt=0;
	Mat dst;

	getInitialGuess(img,region,theta_opt,t_opt);

	// ������� ��������� �����������
	transformImg(img,region,theta_opt,t_opt,dst);
	dst.convertTo(dst,CV_8UC1,255);
	imshow("��������� �����������",dst);
	waitKey(10);

	Mat Tau_0=getTau(theta_opt,t_opt);
	Mat Tau=TILT(img,Tau_0,region,0.1);

	// ��� ����, ����� ����� ����� ����� ��������������, ����� �������� ��������������
	//Mat Tau_inv=Tau.inv();

	// ����� ����������������� ����������� ��������������� �����
	vector<Point2f> region_tau(4);
	transformPts(region,Tau,region_tau);

	Mat banner=imread("E:\\ImagesForTest\\myface.jpg");

	int w=fabs(region[0].x-region[2].x);
	int h=fabs(region[0].y-region[2].y);

	resize(banner,banner,Size(w,h));

	PutBanner(img_c,banner,Tau,region_tau);

	// ������ �����, ����������� ����������
	 DrawRegion(img_c, region_tau,Scalar(0, 255, 255));

	imshow("Banner", banner);

	imshow("�������� �����������", img_c);
	waitKey(0);
	destroyAllWindows();
	return 0;
}
