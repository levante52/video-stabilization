#include "stabilizer.hpp"
#include <cmath>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include <iostream>


bool Stabilizer::init( const cv::Mat& frame)
{
    prevFrame = frame.clone();
    cv::Mat gray4cor;
    cv::cvtColor(frame, gray4cor, cv::COLOR_BGR2GRAY);
   
    cv::goodFeaturesToTrack(gray4cor, previousFeatures, 100, 0.1, 5);

	return true;
}

bool Stabilizer::track( const cv::Mat& frame)
{
    cv::Mat previous_frame_gray;
    const int m = 100;
    const double qLevel = 0.1;
    const double dist = 5.0;

    cvtColor(prevFrame, previous_frame_gray, cv::COLOR_BGR2GRAY);
    cv::goodFeaturesToTrack(previous_frame_gray, previousFeatures, m, qLevel, dist);
    size_t n = previousFeatures.size();
    CV_Assert(n);
    
    // Compute optical flow in selected points.
    std::vector<cv::Point2f> currentFeatures;
    std::vector<uchar> state;
    std::vector<float> error;
    cv::calcOpticalFlowPyrLK(prevFrame, frame, previousFeatures, currentFeatures, state, error);

    std::sort(error.begin(), error.end());
    double median_err = error[error.size()/2];
    std::vector<cv::Point2f> good_points;
    std::vector<cv::Point2f> curr_points;

    for (size_t i = 0; i < n; ++i)
        if ((state[i])&&(error[i]<=median_err))
        {
            good_points.push_back(previousFeatures[i]);
            curr_points.push_back(currentFeatures[i]);
        }


    size_t s = good_points.size();
    CV_Assert(s == curr_points.size());

    // Find points shift.
    std::vector<float> shifts_x(n);
    std::vector<float> shifts_y(n);

    for (size_t i = 0; i < s; ++i)
    {
        shifts_x[i] = curr_points[i].x - good_points[i].x;
        shifts_y[i] = curr_points[i].y - good_points[i].y;
    }

    std::sort(shifts_x.begin(), shifts_x.end());
    std::sort(shifts_y.begin(), shifts_y.end());
    
    // Find median shift.
    cv::Point2f median_shift(shifts_x[s / 2], shifts_y[s / 2]);

    printf("%.4f %.4f\n", median_shift.x, median_shift.y);

    xshift.push_back(median_shift.x);
    yshift.push_back(median_shift.y);
    prevFrame = frame.clone();
    return true;
}

void Stabilizer :: generateFinalShift()
{
    double xsum = 0;
    double ysum = 0;
    double avg_x = 0;
    double avg_y = 0;
    int count = 0;
    int radius = 3;
    for(int j=-radius; j <= radius; j++)
    {
        xsum =0; ysum =0;
        for(int i=0; i < xshift.size(); i++)
        {
            if(i+j >= 0 && i+j < xshift.size())
            {
               xsum += xshift[i];
               ysum += yshift[i];
               count++;
            }
            
            avg_x = xsum / count;
            avg_y = ysum / count;

        }
        xsmoothed.push_back(avg_x);    
        ysmoothed.push_back(avg_y);
    }
}

void Stabilizer :: drawPlots()
{
    int plotWidth = 200; 
    int plotHeight = 200; 

    //std::vector<cv::Point> plot; 

    //for(int i=0; i < xshift.size(); i++)
    //{
    //    plot.push_back(cv::Point(i,xsmoothed[i]));
    //}
    cv::Mat img;
    //CV_Assert(xshift.size() == xsmoothed.size());
    std::cout<< xshift.size()<< std::endl;
    std::cout<< xsmoothed.size()<< std::endl;
    for(unsigned int i=1; i < xsmoothed.size(); ++i)
    {

        cv::Point2f p1; p1.x = i-1; p1.y = xsmoothed[i-1];
        cv::Point2f p2; p2.x = i;   p2.y = xsmoothed[i];
        cv::line(img, p1, p2, 'r', 5, 8, 0);
    }

    //the image to be plotted 
   // cv::Mat img = cv::Mat::zeros(plotHeight,plotWidth, CV_8UC3); 

    cv::namedWindow("Plot", CV_WINDOW_AUTOSIZE); 
    if (!img.empty()) 
    {
        cv::imshow("Plot", img);
    }
    cv::waitKey(0);

}

void Stabilizer::resizeVideo(const cv::Mat& frame, int number, cv::Mat& outputFrame){
    cv::Rect rect(xsmoothed[number],ysmoothed[number],frame.size().width - maxX,frame.size().height - maxY);
    outputFrame = frame(rect);
}


void Stabilizer::caclMaxShifts(){
    int x = 0,y = 0;
    for (int i = 0 ; i < xsmoothed.size(); i++){
        if (abs(xsmoothed[i] - xshift[i]) > x){
            x = abs(xsmoothed[i] - xshift[i]);
        }
        if (abs(ysmoothed[i] - yshift[i]) > y){
            y = abs(ysmoothed[i] - yshift[i]);
        }
    }
    maxX = x; maxY = y;
}
