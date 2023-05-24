#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <iostream>

/*Function///////////////////////////////////////////////////////////////

Name:       StitchingAlgo

Purpose:
...
///////////////////////////////////////////////////////////////////////*/

class StitchingAlgo {
private:
    bool try_use_gpu = false;
    bool divide_images = false;
    cv::Stitcher::Mode mode = cv::Stitcher::PANORAMA;
    cv::Ptr<cv::Stitcher> stitcher;
    cv::Mat panoImg;

public:
    // Default resolution 0 is for SD and other is for HD
    StitchingAlgo(){
        stitcher = cv::Stitcher::create(mode, try_use_gpu);
    }


    ~StitchingAlgo(){}

    cv::Mat StitchingPano(std::vector<cv::Mat> imgs){

        cv::Stitcher::Status status = stitcher->stitch(imgs, panoImg);
        if (status != cv::Stitcher::OK)
        {
            std::cout << "Can't stitch images, error code = " << int(status) << std::endl;
            return cv::Mat();
        }

        return panoImg;
    }

};

