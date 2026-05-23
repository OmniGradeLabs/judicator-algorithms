#ifndef UIED_PHASE2_HPP
#define UIED_PHASE2_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct UIComponent {
    int id = -1;
    cv::Rect bbox;
    int area = 0;
    std::string category;
    bool is_rect = false;
    bool is_line = false;
    bool redundant = false;
    std::vector<int> contain;

    UIComponent() : category("Compo") {}
    explicit UIComponent(const cv::Rect& r)
        : bbox(r), area(r.width * r.height), category("Compo") {}
};

struct Phase2Config {
    int    min_obj_area          = 55;
    int    min_comp_width        = 3;
    int    min_comp_height       = 3;
    int    line_thickness        = 8;
    double max_height_ratio      = 0.8;
    double max_wh_ratio          = 50.0;
    double max_hw_ratio          = 40.0;
    int    thin_side_min         = 8;
    double thin_ratio_max        = 10.0;
    double block_side_ratio      = 0.15;
    double block_blank_threshold = 0.15;
    double nms_iou_threshold     = 0.02;
    double nms_ioa_threshold     = 0.20;
};

std::vector<cv::Rect> detect_components(const cv::Mat& binary,
                                        const Phase2Config& cfg = Phase2Config());

std::vector<UIComponent> detect_components_full(const cv::Mat& binary,
                                                const Phase2Config& cfg = Phase2Config());

std::vector<UIComponent> extract_contours(const cv::Mat& binary,
                                          const Phase2Config& cfg);

std::vector<UIComponent> filter_components(const std::vector<UIComponent>& compos,
                                           const cv::Size& img_size,
                                           const Phase2Config& cfg);

std::vector<UIComponent> merge_intersected(const std::vector<UIComponent>& compos,
                                           const Phase2Config& cfg);

void recognize_blocks(const cv::Mat& binary,
                      std::vector<UIComponent>& compos,
                      const Phase2Config& cfg);

std::vector<UIComponent> remove_contained_non_block(const std::vector<UIComponent>& compos);

void detect_containment(std::vector<UIComponent>& compos);

void assign_ids(std::vector<UIComponent>& compos);

#endif // UIED_PHASE2_HPP
