#include "movdetect.h"
#include "global.h"

bool MovDetect::detectMovement(cv::Mat& curr_frame) {
    if (prev_frame.empty()) {
        curr_frame.copyTo(prev_frame);
        return false;
    }

    cv::Mat diff, gray_diff, thresh;
    cv::GaussianBlur(curr_frame, curr_frame, cv::Size(21, 21), 0);
    cv::absdiff(prev_frame, curr_frame, diff);
    cv::cvtColor(diff, gray_diff, cv::COLOR_BGR2GRAY);
    cv::threshold(gray_diff, thresh, 25, 255, cv::THRESH_BINARY);
    cv::erode(thresh, thresh, cv::Mat(), cv::Point(-1,-1), 2);
    cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1,-1), 2);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    bool movement_detected = false;
    for (const auto& contour : contours) {
        if (cv::contourArea(contour) > 500) { // adjust this value based on your requirement
            movement_detected = true;
            break;
        }
    }

    curr_frame.copyTo(prev_frame);
    return movement_detected;
}

void MovDetect::run() {
    while (g_quit == false) {

        cv::Mat img;
        // cout << "g_queue size: " << g_queue.Size() << endl;
        if (!g_queue.WaitAndPop(img)) {
            break;
        }

        // Resize the image
        cv::Mat resizedImg;
        cv::resize(img, resizedImg, cv::Size(320, 200));

        bool isMoving = detectMovement(resizedImg);
        if (isMoving) {
            std::cout << "Movement detected!" << std::endl;
            g_frame_count = 300;
        }

        // cv::imshow("mov", img);
        // if (cv::waitKey(1) == 27) {
        //     break;
        // }
    }
}