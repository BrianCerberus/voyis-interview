/*
 * SIFT Processor Header
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <vector>
#include "message_protocol.h"

namespace imaging {

class SIFTProcessor {
public:
    SIFTProcessor();
    ~SIFTProcessor() = default;
    
    // Process image and extract SIFT features
    bool processImage(const std::vector<uint8_t>& image_data,
                     std::vector<KeyPoint>& keypoints,
                     std::vector<float>& descriptors);
    
    // Convert OpenCV keypoints to our format
    static void convertKeyPoints(const std::vector<cv::KeyPoint>& cv_keypoints,
                                 std::vector<KeyPoint>& keypoints);
    
    // Convert OpenCV descriptors to flat vector
    static void convertDescriptors(const cv::Mat& cv_descriptors,
                                   std::vector<float>& descriptors);

private:
    cv::Ptr<cv::SIFT> sift_;
};

} // namespace imaging
