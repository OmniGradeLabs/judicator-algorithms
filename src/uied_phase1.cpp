#include "uied_phase1.hpp"

using namespace cv;

Mat process_binarization(const Mat& input_img, int grad_min) {
    Mat gray, dst1, dst2, gradient, binary, morph;
    
    cvtColor(input_img, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32F);

    Mat kernel_h = (Mat_<float>(3,3) << 0, 0, 0, 0, -1, 1, 0, 0, 0);
    Mat kernel_v = (Mat_<float>(3,3) << 0, 0, 0, 0, -1, 0, 0, 1, 0);

    filter2D(gray, dst1, -1, kernel_h);
    filter2D(gray, dst2, -1, kernel_v);

    dst1 = cv::abs(dst1);
    dst2 = cv::abs(dst2);
    dst1.add(dst2).convertTo(gradient, CV_8U);

    threshold(gradient, binary, grad_min, 255, THRESH_BINARY);

    Mat morph_kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(binary, morph, MORPH_CLOSE, morph_kernel);

    return morph;
}
