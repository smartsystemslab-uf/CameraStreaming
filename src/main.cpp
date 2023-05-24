/*
 * Read video frame with FFmpeg and convert to OpenCV image
 *
 * Copyright (c) 2018 Erman
 */
// C++ std library
#include <iostream>
#include <vector>
// C std library
#include <pthread.h>
#include <signal.h>

// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
// Local include
#include "queue.h"
#include "config.h"
#include "showmayimages.h"
#include "stitchingimg.h"


// Stitching with OpenPano
//#define _USE_MATH_DEFINES
//#include <cmath>
//#include "common/common.hh"
//#ifdef DISABLE_JPEG
//#define IMGFILE(x) #x ".png"
//#else
//#define IMGFILE(x) #x ".jpg"
//#endif

using namespace std;

////#define RUN_STITCHING_
//#ifdef RUN_STITCHING_
//#define NUM_THREADS 1
//#else
#define NUM_THREADS 9
//#endif

typedef struct VideoState {
    int id;
    AVFormatContext *inctx;
    AVPixelFormat dst_pix_fmt;
    int dst_width;
    int dst_height;
    int vstrm_idx;
    SwsContext *swsctx;
    AVStream* vstrm;
    AVCodec* vcodec;
    AVDictionary *opts;

    int result;
} VideoState;

int record_images(int id, AVFormatContext *inctx, const AVPixelFormat dst_pix_fmt,
                  const int dst_width, const int dst_height, const int vstrm_idx,
                  SwsContext *swsctx);

void* m_thread_void_recorder(void* arg){
    VideoState* vs = (VideoState*)arg;
    int ret = record_images(vs->id, vs->inctx, vs->dst_pix_fmt, vs->dst_width,
                            vs->dst_height, vs->vstrm_idx, vs->swsctx);
    if(ret < 0)
        ret = record_images(vs->id, vs->inctx, vs->dst_pix_fmt, vs->dst_width,
                            vs->dst_height, vs->vstrm_idx, vs->swsctx);
    pthread_exit(&ret);
}

// Create and initialize a videoState object from file.
VideoState* InitVideoStateObject(const char* infile, int id);

std::vector<MQueue<cv::Mat>> mQueueList(NUM_THREADS);


#define SAVE_INFILE \
{\
    GuardedTimer tm("Writing image");\
    write_rgb(IMGFILE(out_hum), resPano);\
    resPano = crop(resPano);\
    write_rgb(IMGFILE(out_hum_crop), resPano);\
}

#include <experimental/filesystem>
namespace filesys = std::experimental::filesystem;
/** Get the list of all files in given directory and its sub directories.
 *
 * Arguments
 * 	dirPath : Path of directory to be traversed
 * 	dirSkipList : List of folder names to be skipped
 *
 * Returns:
 * 	vector containing paths of all the files in given directory and its sub directories
 *
 */
std::vector<string>
getAllFilesInDir(const std::string &dirPath, const std::vector<std::string> dirSkipList = {});


/** Get the list of sub-directory names in given directory.
 *
 * Arguments
 * 	dirPath : Path of directory to be traversed
 * 	dirSkipList : List of folder names to be skipped
 *
 * Returns:
 * 	vector containing paths of all sub directories in a given directory
 *
 */
std::vector<string>
getDirPathInDir(const std::string &dirPath, const std::vector<std::string> dirSkipList = {});


///     /media/sf_Data/data_stitching/Airplanes/Input
std::vector<std::vector<std::string>> readAirplaneImages(std::string folder_dir){

    // List of folder per cameras
    std::vector<std::string> camfolders = getDirPathInDir(folder_dir/*, std::vector<std::string>{"00002"}*/);

    // Matrix of all images Paths. Each row per cameras.
    std::vector<std::vector<std::string>> imagePaths;

    for (const auto & entry : camfolders){
        std::cout << entry << std::endl;
        std::vector<std::string> vec = getAllFilesInDir(entry);
        imagePaths.push_back(vec);
    }

    for(unsigned i=0; i<imagePaths.size(); ++i){
        std::cout << i << " - Total #file: " << imagePaths[i].size() << std::endl;
    }

    return imagePaths;
}

///     /media/sf_Data/data_stitching/SmartSysLab
std::vector<std::vector<std::string>> readSmartLabImages(std::string folder_dir){

    // Matrix of all images Paths. Each row per cameras.
    std::vector<std::vector<std::string>> imagePaths;

    std::vector<std::string> vec = getAllFilesInDir(folder_dir);
    for (const auto & entry : vec){
        std::cout << entry << std::endl;
    }

    imagePaths.push_back(vec);
    for(unsigned i=0; i<imagePaths.size(); ++i){
        std::cout << i << " - Total #file: " << imagePaths[i].size() << std::endl;
    }

    return imagePaths;
}

int main(int argc, char* argv[])
{

    srand(time(NULL));

    jsonconfig jconf;
    jconf.load_json();

    std::string _input_f = "rtsp://192.168.1.9:554/test"; //"rtsp://admin:SSL_pass2021@192.168.1.10/MediaInput/h264"; //
    std::vector<std::string> input_files;
    std::vector<std::string> output_videos;

    for (auto &elt: jconf.js["clientdata"]){
        std::string mac_addrr = elt["deviceInfo"]["con_mac_addr"].get<std::string>();
        input_files.push_back(_input_f+mac_addrr);
        output_videos.push_back(mac_addrr);
    }

    // initialize FFmpeg library
    av_register_all();
    avformat_network_init();

    av_log_set_level(AV_LOG_ERROR);

    int rc, i;
    size_t nbImg = 0; int mWidth = 0, mHeigh = 0;
    VideoState *vss[NUM_THREADS];
    for(i = 0; i<NUM_THREADS; i++, nbImg++){

        VideoState *vs = InitVideoStateObject(input_files[i].c_str(), i);
        if(vs->result < 0){
            std::cout << "Could not open file: " << input_files[i] << std::endl;
            return -1;
        }
        vss[i] = vs;

        // print input video stream informataion
        std::cout
            << "infile: " << input_files[i] << "\n"
            << "format: " << vs->inctx->iformat->name << "\n"
            << "vcodec: " << vs->vcodec->name << "\n"
            << "size:   " << vs->vstrm->codecpar->width << 'x' << vs->vstrm->codecpar->height << "\n"
            << "fps:    " << av_q2d(vs->vstrm->avg_frame_rate) << " [fps]\n"
            << "length: " << av_rescale_q(vs->vstrm->duration, vs->vstrm->time_base, {1,1000}) / 1000. << " [sec]\n"
            << "pixfmt: " << av_get_pix_fmt_name((AVPixelFormat)vs->vstrm->codecpar->format) << "\n"
            << "frame:  " << vs->vstrm->nb_frames << "\n"
            << std::flush;

        if(vs->vstrm->codecpar->width > mWidth) mWidth = vs->vstrm->codecpar->width;
        if(vs->vstrm->codecpar->height > mHeigh) mHeigh = vs->vstrm->codecpar->height;
        std::cout << "output: " << vs->dst_width << 'x' << vs->dst_height << ',' << av_get_pix_fmt_name(vs->dst_pix_fmt) << std::endl;
    }

    pthread_t thr[NUM_THREADS];
    /* create threads */
    for (i = 0; i < NUM_THREADS; ++i) {
        if ((rc = pthread_create(&thr[i], nullptr, m_thread_void_recorder, (void*)vss[i] ))) {
            fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
            return EXIT_FAILURE;
        }
    }

    std::cout << "NbImage: " << nbImg << ", Final size per image: " << mWidth << "x" << mHeigh << std::endl;
    CanvasMnger mCMnger(nbImg, mWidth, mHeigh, 1, output_videos, false);
    //Show images in the buffer
    std::string win_str = "Press ESC to exit";
    cv::namedWindow( win_str, cv::WINDOW_FULLSCREEN );// Create a window for display.
    cv::Mat mergedH;
    std::vector<cv::Mat> mImgs(NUM_THREADS);

    bool isImage = false;
    do{
        for(int i=0; i<NUM_THREADS; i++){

            mImgs[i] = mQueueList[i].peekSafe();
            if(mImgs[i].empty()){
                isImage = true;
                break;
            }
        }

        if(isImage){
            isImage = false;
            continue;
        }

        mergedH = mCMnger.ShowManyImages(mImgs);
        if(mergedH.empty()){
            fprintf(stderr, "Failed to capture image 1!\n");
            continue;
        }
        cv::imshow(win_str, mergedH);
        char key = (char) cv::waitKey(1);
        if (key == 27/*esc*/){
            mCMnger.stopsavevideos();
            break;
        }
        else if (key == 'p'){
            mCMnger.pauseplayrecord();
        }
        else if (key == 's'){
            mCMnger.stopsavevideos();
        }

    }while(1);

    cv::destroyAllWindows();


    /* Kill all threads */
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_kill( thr[i], SIGUSR1);
    }


    /* block until all threads complete */
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(thr[i], nullptr);

        // allocate frame buffer for output
        avcodec_close(vss[i]->vstrm->codec);
        avformat_close_input(&vss[i]->inctx);
    }

    avformat_network_deinit();
    return 0;
}

#include <libavutil/motion_vector.h>
int record_images(int id, AVFormatContext* inctx, const AVPixelFormat dst_pix_fmt,
                  const int dst_width, const int dst_height, const int vstrm_idx, SwsContext* swsctx) {
    bool end_of_stream = false;
    unsigned nb_frames = 0;
    int got_pic = 0;
    int ret;
    AVPacket pkt;

    // allocate frame buffer for output
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> framebuf(avpicture_get_size(dst_pix_fmt, dst_width, dst_height));
    avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), dst_pix_fmt, dst_width, dst_height);

    // decoding loop
    AVFrame* decframe = av_frame_alloc();
    AVStream* vstrm = inctx->streams[vstrm_idx];


    do {
        if (!end_of_stream) {
            // read packet from input file
            ret = av_read_frame(inctx, &pkt);
            if (ret < 0 && ret != AVERROR_EOF) {
                std::cerr << "fail to av_read_frame ~ ret: " << ret << std::endl << std::endl;
                return 2;
            }
            if (ret == 0 && pkt.stream_index != vstrm_idx)
                goto next_packet;
            end_of_stream = (ret == AVERROR_EOF);
        }
        if (end_of_stream) {
            // null packet for bumping process
            av_init_packet(&pkt);
            pkt.data = nullptr;
            pkt.size = 0;
        }
        // decode video frame
        avcodec_decode_video2(vstrm->codec, decframe, &got_pic, &pkt);
        if (!got_pic)
            goto next_packet;
        // convert frame to OpenCV matrix
        sws_scale(swsctx, decframe->data, decframe->linesize, 0, decframe->height, frame->data, frame->linesize);
        {
            // Nice link for motion vetor
            // https://ffmpeg.org/pipermail/ffmpeg-devel/2014-August/161593.html
            // https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/extract_mvs.c
            AVFrameSideData* sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
            if (sd) {
                const AVMotionVector *mvs = (const AVMotionVector *)sd->data;
                for (unsigned i = 0; i < sd->size / sizeof(*mvs); i++) {
                    const AVMotionVector *mv = &mvs[i];
                    printf("%2d,%2d,%2d,%4d,%4d,%4d,%4d,0x%0lu\n",
                           mv->source,
                           mv->w, mv->h, mv->src_x, mv->src_y,
                           mv->dst_x, mv->dst_y, mv->flags);
                }
            }
            // Query to get the motion vector
            cv::Mat image(dst_height, dst_width, CV_8UC3, framebuf.data(), frame->linesize[0]);
            mQueueList[id].enqueueSafe(image);
        }
//        std::cout << nb_frames << '\r' << dst_height << "x" << dst_width << std::flush;  // dump progress
        ++nb_frames;

next_packet:
        av_packet_unref(&pkt);
    } while (!end_of_stream || got_pic);
    std::cout << nb_frames << " frames decoded" << std::endl;
    av_frame_free(&decframe);
    av_frame_free(&frame);
    return 0;
}

VideoState* InitVideoStateObject(const char* infile, int id){

    VideoState *vs = (VideoState*)malloc(sizeof(VideoState));
    vs->id = id;
    // open input file context
    AVFormatContext* inctx = nullptr;
    int ret;
    ret = avformat_open_input(&inctx, infile, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avforamt_open_input(\"" << infile << "\"): ret=" << ret << std::endl;
        vs->result = -1;
        return vs;
    }
    vs->inctx = inctx;

    // retrive input stream information
    ret = avformat_find_stream_info(inctx, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avformat_find_stream_info: ret=" << ret << std::endl;
        vs->result = -1;
        return vs;
    }

    // find primary video stream
    AVCodec* vcodec = nullptr;
    ret = av_find_best_stream(inctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
    if (ret < 0) {
        std::cerr << "fail to av_find_best_stream: ret=" << ret << std::endl;
        vs->result = -1;
        return vs;
    }
    vs->vcodec = vcodec;

    const int vstrm_idx = ret;
    AVStream* vstrm = inctx->streams[vstrm_idx];
    vs->vstrm = vstrm;
    vs->vstrm_idx = vstrm_idx;

    // open video decoder context
    /* Init the video decoder */
//    av_dict_set(&vs->opts, "flags2", "+export_mvs", 0);
    ret = avcodec_open2(vstrm->codec, vcodec, /*&vs->opts*/ nullptr);
    if (ret < 0) {
        std::cerr << "fail to avcodec_open2: ret=" << ret << std::endl;
        vs->result = -1;
        return vs;
    }

    // initialize sample scaler
    vs->dst_width = vstrm->codecpar->width;
    vs->dst_height = vstrm->codecpar->height;
    vs->dst_pix_fmt = AV_PIX_FMT_BGR24;
    SwsContext* swsctx = sws_getCachedContext(
        nullptr, vstrm->codecpar->width, vstrm->codecpar->height, (AVPixelFormat) vstrm->codecpar->format,
        vs->dst_width, vs->dst_height, vs->dst_pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        vs->result = -1;
        return vs;
    }
    vs->swsctx = swsctx;

    return vs;
}

std::vector<std::string>
getAllFilesInDir(const std::string &dirPath, const std::vector<std::string> dirSkipList)
{

    // Create a vector of string
    std::vector<std::string> listOfFiles;
    try {
        // Check if given path exists and points to a directory
        if (filesys::exists(dirPath) && filesys::is_directory(dirPath))
        {
            // Create a Recursive Directory Iterator object and points to the starting of directory
            filesys::recursive_directory_iterator iter(dirPath);

            // Create a Recursive Directory Iterator object pointing to end.
            filesys::recursive_directory_iterator end;

            // Iterate till end
            while (iter != end)
            {
                // Check if current entry is a directory and if exists in skip list
                if (filesys::is_directory(iter->path()) &&
                        (std::find(dirSkipList.begin(), dirSkipList.end(), iter->path().filename()) != dirSkipList.end()))
                {
                    // Skip the iteration of current directory pointed by iterator
#ifdef USING_BOOST
                    // Boost Fileystsem  API to skip current directory iteration
                    iter.no_push();
#else
                    // c++17 Filesystem API to skip current directory iteration
                    iter.disable_recursion_pending();
#endif
                }
                else
                {
                    // Add the name in vector
                    listOfFiles.push_back(iter->path().string());
                }

                error_code ec;
                // Increment the iterator to point to next entry in recursive iteration
                iter.increment(ec);
                if (ec) {
                    std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() << '\n';
                }
            }
        }
    }
    catch (std::system_error & e)
    {
        std::cerr << "Exception :: " << e.what();
    }
    return listOfFiles;
}

std::vector<std::string>
getDirPathInDir(const std::string &dirPath, const std::vector<std::string> dirSkipList)
{

    // Create a vector of string
    std::vector<std::string> listOfDirnames;
    try {
        // Check if given path exists and points to a directory
        if (filesys::exists(dirPath) && filesys::is_directory(dirPath))
        {
            for (const auto & entry : filesys::directory_iterator(dirPath)){
                // Check if current entry is a directory and if exists in skip list
                if (filesys::is_directory(entry.path()) &&
                        (std::find(dirSkipList.begin(), dirSkipList.end(), entry.path().filename())
                         == dirSkipList.end()))
                {
                    listOfDirnames.push_back(entry.path().string());
                }
            }
        }
    }
    catch (std::system_error & e)
    {
        std::cerr << "Exception :: " << e.what();
    }
    return listOfDirnames;
}


