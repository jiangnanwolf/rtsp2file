#include "objdetect.h"
#include "global.h"

using namespace std;

ObjDetect::ObjDetect()
{
    // Load the network
    m_net = cv::dnn::readNetFromDarknet("/home/l/MyCode/rtsp2file/build/cfg/yolov3-tiny.cfg", "/home/l/MyCode/rtsp2file/build/yolov3-tiny.weights");
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    // std::cout << cv::getBuildInformation() << std::endl;

}

void ObjDetect::run()
{
    while (g_quit == false) {
        cv::Mat img;
        // cout << "g_queue size: " << g_queue.Size() << endl;
        if (!g_queue.WaitAndPop(img)) {
            break;
        }
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
        m_net.setInput(blob);

        std::vector<cv::Mat> outs;
        m_net.forward(outs, getOutputsNames(m_net));

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
        cv::imshow("obj", img);
        cv::waitKey(1);
        
    }
    

}


std::vector<std::string> ObjDetect::getOutputsNames(const cv::dnn::Net& net)
{
    static std::vector<std::string> names;
    if (names.empty())
    {
        // Get the indices of the output layers, i.e. the layers with unconnected outputs
        std::vector<int> outLayers = net.getUnconnectedOutLayers();

        // Get the names of all the layers in the network
        std::vector<std::string> layersNames = net.getLayerNames();

        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i)
            names[i] = layersNames[outLayers[i] - 1];
    }
    return names;
}