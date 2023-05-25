#include <opencv2/opencv.hpp>

#include <stdio.h>


/*Function///////////////////////////////////////////////////////////////

Name:       ShowManyImages

Purpose:

This is a function illustrating how to display more than one
image in a single window using Intel OpenCV

Parameters:

string title: Title of the window to be displayed
int    nArgs: Number of images to be displayed
Mat    img1: First Mat, which contains the first image
...
Mat    imgN: First Mat, which contains the Nth image

Language:   C++

The method used is to set the ROIs of a Single Big image and then resizing
and copying the input images on to the Single Big Image.

This function does not stretch the image...
It resizes the image without modifying the width/height ratio..

This function can be called like this:

ShowManyImages("Images", 5, img2, img2, img3, img4, img5);


...
///////////////////////////////////////////////////////////////////////*/


#include <time.h>
#include <sys/time.h>
#include <time.h>
inline std::string GetCurrentFormattedDateTime(std::string fmt = "%Y:%m:%d %H:%M:%S")
{
    char buffer[26];
    int millisec;
    struct tm* tm_info;
    struct timeval tv;

    gettimeofday(&tv, nullptr);

    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
      millisec -=1000;
      tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, fmt.c_str(), tm_info);
    char currentTime[84] = "";
    sprintf(currentTime, "%s.%03d", buffer, millisec);
    std::string str(currentTime);

    return str;
}

#include <unistd.h>
std::string getpath() {
    char buf[PATH_MAX + 1];
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
        throw std::string("readlink() failed");
    std::string str(buf);
    return str.substr(0, str.rfind('/'));
}

template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


inline void PutLabelOnImg(cv::Mat& im,
                          const std::string label,
                          const cv::Point& pts,
                          cv::Scalar txtColor=CV_RGB(0,0,0),
                          cv::Scalar bgColor=CV_RGB(255,255,255),
                          int fontface = cv::FONT_HERSHEY_COMPLEX_SMALL,
                          double scale = 1.0,
                          int thickness = 1,
                          int baseline = 0)
{

    cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
    cv::rectangle(im, pts + cv::Point(0, baseline), pts + cv::Point(text.width, -text.height), txtColor, cv::FILLED);
    cv::putText(im, label, pts, fontface, scale, bgColor, thickness, cv::LINE_AA);
}


class CanvasMnger{
private:

    // nRows  - Maximum number of images in a row
    // nCols - Maximum number of images in a column
    size_t nRows, nCols;
    const size_t marginW = 5;//Margin on the width
    const size_t marginH = 5; // Margin on the heigh
    cv::Mat canvasImage;
    size_t nbImg;
    std::vector<std::string> filenames;
    std::vector<cv::VideoWriter> vws; // To write videos
    size_t W, H; // Width and the Heigh of the largest images

    // Final dimension of the canvas image (SD, HD)
    // 1280 x 720 or 1920 x 1080
    int finalWidth, finalHeight;

    /// Record videos control variable
    bool savevideos;
    std::string recordstatus;

    cv::Mat canvasImage2;
    double sw, sh; //scale on width and height
    int sMarginW, sMarginH; //scale on margin

public:
    // Default resolution 0 is for SD and other is for HD
    CanvasMnger(size_t nbImg, size_t _w, size_t _h, int resolution = 0,
                std::vector<std::string>_filenames = std::vector<std::string>(),
            bool savevideos = false)
        :filenames(_filenames), savevideos(savevideos)
    {
        if(nbImg == 0) {
            printf("Number of arguments too small....\n");
            nbImg = 1;
        }
        else if(nbImg > 14) {
            printf("Number of arguments too large, can only handle maximally 12 images at a time ...\n");
            nbImg = 12;
        }
        if(savevideos){
            recordstatus = "Recording";
        }else{
            recordstatus = "Pause Recording";
        }

        this->nbImg = nbImg;
        this->W = _w;
        this->H = _h;

        // Determine the size of the image,
        // and the number of rows/cols
        // from number of arguments
        if (nbImg == 1) {
            nRows = nCols = 1;
        }
        else if (nbImg == 2) {
            nRows = 1; nCols = 2;
        }
        else if (nbImg == 3 || nbImg == 4) {
            nRows = 2; nCols = 2;
        }
        else if (nbImg == 5 || nbImg == 6) {
            nRows = 2; nCols = 3;
        }
        else if (nbImg == 7 || nbImg == 8) {
            nRows = 2; nCols = 4;
        }
        else {
            nRows = 3; nCols = 4;
        }

        if(resolution == 0){
            // 1280 x 720 or 1920 x 1080 or 3440 x 1440
            finalWidth = 3440; finalHeight = 1440;
        }else{
            finalWidth = 1920; finalHeight = 1080;
        }

        // Compute the real value of width and hight
        size_t imgW = (W + 2*marginW)*nCols;
        size_t imgH = (H + 2*marginH)*nRows;
        printf("imgW: %lu, imgH: %lu, nCols: %lu, nRows: %lu (W:%luxH:%lu)\n", imgW, imgH, nCols, nRows, W, H);
        canvasImage2 = cv::Mat::zeros(cv::Size(finalWidth+10, finalHeight+10), CV_8UC3);
        sw = finalWidth/static_cast<double>(imgW);
        sh = finalHeight/static_cast<double>(imgH);
        sMarginW = static_cast<int>(marginW*sw);
        sMarginH = static_cast<int>(marginH*sh);
        std::cout << "Canvas size: " << canvasImage2.size() << "; " << sMarginW << "x" << sMarginH << "; (sw,sh): " << sw << "x" << sh << std::endl;

        std::string current_folder = getpath();
        std::cout << "Current Folder: " << current_folder << std::endl;
        for (unsigned i = 0;i<this->nbImg;++i) {
            std::string currf = current_folder+"/"+filenames[i]+
                    GetCurrentFormattedDateTime(/*std::string fmt =*/ "-%Y%m%d%H-%M-%S")+".avi";
            std::cout << "Current videos file (" << i << "): " << currf << std::endl;
            vws.push_back(cv::VideoWriter(currf,
                                          cv::VideoWriter::fourcc('M','J','P','G'),
                                          10, cv::Size(this->W,this->H)));
        }
    }
    ~CanvasMnger(){
        stopsavevideos();
    }

    void stopsavevideos(){
        if(savevideos){
            savevideos = false;
        }
        recordstatus = "Stop Record";
        for (unsigned i = 0;i<this->nbImg;++i) {
            vws[i].release();
        }
    }

    void pauseplayrecord(){
        if(savevideos){
            recordstatus = "Pause Record";
        }else{
            recordstatus = "Recording";
        }
        savevideos = !savevideos;
    }

    cv::Mat ShowManyImages(std::vector<cv::Mat> imgs){
        int pos_y = sMarginH;

        if(savevideos){
            for(unsigned k = 0; k<nbImg; k++){
                if(imgs[k].empty()) continue;
                vws[k].write(imgs[k]);
            }
        }

        for(size_t i = 0; i < nRows; i++){
            int pos_x = sMarginW;
            for(size_t j = 0; j < nCols; j++){
                size_t k = i*nCols + j;
                if(k >= nbImg || k >= imgs.size())
                    // If we process all images we return the canvas image.
                    return canvasImage2;

                // Draw the k'th image at the position (posi, posj) in the canvas
                cv::Mat img = imgs[k].clone();

                // Check whether it is NULL or not
                // If it is NULL, release the image, and return
                if(img.empty()) {
                    printf("Invalid arguments\n");
                    return cv::Mat();
                }

                // String to write on camera
                std::string cam_str = "CSCE-E.S Lab-445 - capstone2018 - cam ";

                cam_str += std::to_string(k);
                cv::putText(img,
                            cam_str, // Message to write
                            cv::Point(30,30), // Coordinates
                            cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
                            1.0, // Scale. 2.0 = 2x bigger
                            cv::Scalar(0,0,255), // BGR Color
                            1, // Line Thickness (Optional)
                            cv::LINE_AA); // Anti-alias (Optional)

                PutLabelOnImg(img, GetCurrentFormattedDateTime(), cv::Point(50, img.rows - 50));
                if(!filenames.empty())
                    PutLabelOnImg(img, filenames[k], cv::Point(30, 60));

                // Set the image ROI to display the current image
                // Resize the input image and copy the it to the Single Big Image
                cv::Rect ROI(pos_x, pos_y, img.cols*sw, img.rows*sh);
                cv::Mat temp; resize(img,temp, cv::Size(ROI.width, ROI.height));
                temp.copyTo(canvasImage2(ROI));
                pos_x += (W*sw + 2*sMarginH);
            }
            pos_y += (H*sh + 2*sMarginW);
        }

        PutLabelOnImg(canvasImage2, recordstatus, cv::Point(30, 100));
        return canvasImage2;
    }
};

cv::Mat ShowManyImages(std::vector<cv::Mat> imgs) {
    int size;
    int i;
    int m, n;
    int x, y;

    // w - Maximum number of images in a row
    // h - Maximum number of images in a column
    int w, h;

    // scale - How much we have to resize the image
    float scale;
    int max;
    int nbImg = imgs.size();

    // If the number of arguments is lesser than 0 or greater than 12
    // return without displaying
    if(nbImg <= 0) {
        printf("Number of arguments too small....\n");
        return cv::Mat();
    }
    else if(nbImg > 14) {
        printf("Number of arguments too large, can only handle maximally 12 images at a time ...\n");
        return cv::Mat();
    }
    // Determine the size of the image,
    // and the number of rows/cols
    // from number of arguments
    else if (nbImg == 1) {
        w = h = 1;
        size = 300;
    }
    else if (nbImg == 2) {
        w = 2; h = 1;
        size = 300;
    }
    else if (nbImg == 3 || nbImg == 4) {
        w = 2; h = 2;
        size = 300;
    }
    else if (nbImg == 5 || nbImg == 6) {
        w = 3; h = 2;
        size = 200;
    }
    else if (nbImg == 7 || nbImg == 8) {
        w = 4; h = 2;
        size = 200;
    }
    else {
        w = 4; h = 3;
        size = 150;
    }

    // Create a new 3 channel image
    cv::Mat DispImage = cv::Mat::zeros(cv::Size(30 + size*w, 30 + size*h), CV_8UC3);

    // Loop for nArgs number of arguments
    for (i = 0, m = 10, n = 10; i < nbImg; i++, m += (10 + size)) {
        // Get the Pointer to the IplImage
        cv::Mat img = imgs[i];

        // Check whether it is NULL or not
        // If it is NULL, release the image, and return
        if(img.empty()) {
            printf("Invalid arguments\n");
            return cv::Mat();
        }

        // Find the width and height of the image
        x = img.cols;
        y = img.rows;

        // Find whether height or width is greater in order to resize the image
        max = (x > y)? x: y;

        // Find the scaling factor to resize the image
        scale = (float) ( (float) max / size );

        // Used to Align the images
        if( i % w == 0 && m!= 10) {
            m = 10;
            n+= 10 + size;
        }

        // Set the image ROI to display the current image
        // Resize the input image and copy the it to the Single Big Image
        cv::Rect ROI(m, n, (int)( x/scale ), (int)( y/scale ));
        cv::Mat temp; resize(img,temp, cv::Size(ROI.width, ROI.height));
        temp.copyTo(DispImage(ROI));
    }

    // Create a new window, and show the Single Big Image
    return DispImage;
}
