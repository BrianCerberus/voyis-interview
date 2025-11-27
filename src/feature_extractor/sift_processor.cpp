/*
 * SIFT Processor Implementation
 * Author: Haobo (Brian) Liu
 * Email: h349liu@gmail.com
 * Project: Distributed Imaging Services for Voyis Interview
 */

#include "sift_processor.h"
#include "logger.h"

namespace imaging {

SIFTProcessor::SIFTProcessor() {
    // Create SIFT detector with default parameters
    sift_ = cv::SIFT::create();
    Logger::info("SIFT processor initialized");
}

bool SIFTProcessor::processImage(const std::vector<uint8_t>& image_data,
                                 std::vector<KeyPoint>& keypoints,
                                 std::vector<float>& descriptors) {
    try {
        // Decode image from buffer
        cv::Mat img = cv::imdecode(image_data, cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            Logger::error("Failed to decode image");
            return false;
        }
        
        // Detect keypoints and compute descriptors
        std::vector<cv::KeyPoint> cv_keypoints;
        cv::Mat cv_descriptors;
        
        sift_->detectAndCompute(img, cv::noArray(), cv_keypoints, cv_descriptors);
        
        // Convert to our format
        convertKeyPoints(cv_keypoints, keypoints);
        convertDescriptors(cv_descriptors, descriptors);
        
        return true;
    } catch (const cv::Exception& e) {
        Logger::error("OpenCV exception: " + std::string(e.what()));
        return false;
    }
}

void SIFTProcessor::convertKeyPoints(const std::vector<cv::KeyPoint>& cv_keypoints,
                                     std::vector<KeyPoint>& keypoints) {
    keypoints.clear();
    keypoints.reserve(cv_keypoints.size());
    
    for (const auto& cv_kp : cv_keypoints) {
        KeyPoint kp;
        kp.x = cv_kp.pt.x;
        kp.y = cv_kp.pt.y;
        kp.size = cv_kp.size;
        kp.angle = cv_kp.angle;
        kp.response = cv_kp.response;
        kp.octave = cv_kp.octave;
        keypoints.push_back(kp);
    }
}

void SIFTProcessor::convertDescriptors(const cv::Mat& cv_descriptors,
                                       std::vector<float>& descriptors) {
    descriptors.clear();
    
    if (cv_descriptors.empty()) {
        return;
    }
    
    // SIFT descriptors are typically 128-dimensional float vectors
    int total_elements = cv_descriptors.rows * cv_descriptors.cols;
    descriptors.reserve(total_elements);
    
    // Convert to flat vector
    if (cv_descriptors.type() == CV_32F) {
        const float* ptr = cv_descriptors.ptr<float>();
        descriptors.assign(ptr, ptr + total_elements);
    } else {
        // Convert from other types to float
        cv::Mat float_descriptors;
        cv_descriptors.convertTo(float_descriptors, CV_32F);
        const float* ptr = float_descriptors.ptr<float>();
        descriptors.assign(ptr, ptr + total_elements);
    }
}

} // namespace imaging
