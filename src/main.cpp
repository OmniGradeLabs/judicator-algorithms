#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "uied_phase1.hpp"
#include "uied_phase2.hpp"

using namespace cv;
using namespace std;

static void export_json(const string& path,
                        const vector<UIComponent>& compos,
                        const Size& img_size) {
    ofstream ofs(path);
    if (!ofs.is_open()) {
        cerr << "[ERROR] Cannot write to: " << path << endl;
        return;
    }

    ofs << "{\n";
    ofs << "  \"img_shape\": [" << img_size.height << ", " << img_size.width << "],\n";
    ofs << "  \"compos\": [\n";

    for (size_t i = 0; i < compos.size(); ++i) {
        const auto& c = compos[i];
        ofs << "    {\n";
        ofs << "      \"id\": "          << c.id                          << ",\n";
        ofs << "      \"class\": \""     << c.category                    << "\",\n";
        ofs << "      \"column_min\": "  << c.bbox.x                      << ",\n";
        ofs << "      \"row_min\": "     << c.bbox.y                      << ",\n";
        ofs << "      \"column_max\": "  << (c.bbox.x + c.bbox.width)     << ",\n";
        ofs << "      \"row_max\": "     << (c.bbox.y + c.bbox.height)    << ",\n";
        ofs << "      \"width\": "       << c.bbox.width                  << ",\n";
        ofs << "      \"height\": "      << c.bbox.height                 << "\n";
        ofs << "    }";
        if (i + 1 < compos.size()) ofs << ",";
        ofs << "\n";
    }

    ofs << "  ]\n}\n";
    ofs.close();
    cout << "[OK] JSON exported → " << path << endl;
}

static void draw_and_save(const Mat& original,
                          const vector<UIComponent>& compos,
                          const string& path) {
    Mat canvas = original.clone();

    for (const auto& c : compos) {
        Scalar color = (c.category == "Block")
            ? Scalar(255, 165, 0)
            : Scalar(0, 255, 0);

        rectangle(canvas, c.bbox, color, 2);

        string label = "#" + to_string(c.id);
        int baseline  = 0;
        Size text_size = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseline);
        Point label_pos(c.bbox.x, max(c.bbox.y - 4, text_size.height + 2));
        putText(canvas, label, label_pos, FONT_HERSHEY_SIMPLEX, 0.4, color, 1, LINE_AA);
    }

    imwrite(path, canvas);
    cout << "[OK] Annotated image saved → " << path << endl;
}

int main(int argc, char** argv) {
    string image_path = "../assets/test_ui.png";
    if (argc > 1) image_path = argv[1];

    cout << "=== UIED C++ Pipeline ===" << endl;
    cout << "[INPUT] " << image_path << endl;

    Mat img = imread(image_path, IMREAD_COLOR);
    if (img.empty()) {
        cerr << "[ERROR] Image not found: " << image_path << endl;
        return -1;
    }
    cout << "[INFO] Image: " << img.cols << "x" << img.rows << endl;

    auto t0 = chrono::high_resolution_clock::now();
    Mat binary_map = process_binarization(img, 10);
    auto t1 = chrono::high_resolution_clock::now();

    double ms_phase1 = chrono::duration<double, milli>(t1 - t0).count();
    cout << "[PHASE 1] " << fixed << setprecision(2) << ms_phase1 << " ms" << endl;
    imwrite("output_binary.png", binary_map);

    auto t2 = chrono::high_resolution_clock::now();
    Phase2Config cfg;
    auto compos = detect_components_full(binary_map, cfg);
    auto t3 = chrono::high_resolution_clock::now();

    double ms_phase2 = chrono::duration<double, milli>(t3 - t2).count();
    cout << "[PHASE 2] " << fixed << setprecision(2) << ms_phase2 << " ms"
         << "  (" << compos.size() << " components)" << endl;

    int blocks = 0;
    for (const auto& c : compos) if (c.category == "Block") blocks++;
    cout << "[STATS] Compos: " << (compos.size() - blocks) << "  Blocks: " << blocks << endl;
    cout << "[TOTAL] " << fixed << setprecision(2) << (ms_phase1 + ms_phase2) << " ms" << endl;

    draw_and_save(img, compos, "output_final.png");
    export_json("output_compos.json", compos, img.size());

    return 0;
}
