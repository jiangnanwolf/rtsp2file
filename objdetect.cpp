#include "ojbdetect.h"

ObjDetect::ObjDetect()
{
    // Load the network
    m_net = cv::dnn::readNetFromDarknet("cfg/yolov3-tiny.cfg", "yolov3-tiny.weights");
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);

}

void run()
{
    // Define the desired smaller size
    cv::Size smallerSize(320, 240); // Adjust the size as needed

    // Resize the image
    cv::Mat resizedImg;
    cv::resize(img, resizedImg, smallerSize);

    // Detect humans in the resized image
    std::vector<cv::Rect> detections;
    std::vector<double> weights;
    cv::Mat blob;
    cv::dnn::blobFromImage(resizedImg, blob, 1/255.0, cv::Size(416, 416), cv::Scalar(0,0,0), true, false);
    net.setInput(blob);

    std::vector<cv::Mat> outs;
    net.forward(outs, getOutputsNames(net));

    for (size_t i = 0; i < outs.size(); ++i)
    {
        // for each detection from each output layer
        // get the confidence, class id, bounding box params
        // and ignore weak detections (confidence < 0.5)
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
        {
            cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            cv::Point classIdPoint;
            double confidence;
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > 0.6)
            {
                int centerX = (int)(data[0] * img.cols);
                int centerY = (int)(data[1] * img.rows);
                int width = (int)(data[2] * img.cols);
                int height = (int)(data[3] * img.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;
                
                detections.push_back(cv::Rect(left, top, width, height));
                weights.push_back(confidence);
            }
        }
    }

    // Draw bounding boxes around detected humans
    for (const cv::Rect& detection : detections) {
        cv::rectangle(img, detection, cv::Scalar(0, 255, 0), 2);
    }

    cv::imshow("img", img);
    cv::waitKey(1);
}