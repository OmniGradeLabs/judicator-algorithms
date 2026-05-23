#include <iostream>
#include <opencv2/opencv.hpp>
#include "uied_phase1.hpp"

using namespace cv;
using namespace std;

int main() {
    string image_path = "../assets/test_ui.png";
    Mat img = imread(image_path, IMREAD_COLOR);
    
    if (img.empty()) {
        cout << "Không tìm thấy ảnh tại: " << image_path << endl;
        return -1;
    }

    Mat binary_map = process_binarization(img, 10);

    imwrite("output_binary.png", binary_map);
    cout << "Đã xử lý xong! Kiểm tra file output_binary.png" << endl;

    return 0;
}
