#include "uied_phase2.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

using namespace cv;
using namespace std;

namespace {

int bbox_relation_nms(const Rect& a, const Rect& b,
                      double iou_thresh, double io_thresh) {
    int x1 = max(a.x, b.x);
    int y1 = max(a.y, b.y);
    int x2 = min(a.x + a.width,  b.x + b.width);
    int y2 = min(a.y + a.height, b.y + b.height);

    int w = max(0, x2 - x1);
    int h = max(0, y2 - y1);
    double inter = static_cast<double>(w) * h;

    double area_a = static_cast<double>(a.width)  * a.height;
    double area_b = static_cast<double>(b.width)  * b.height;

    if (area_a <= 0 || area_b <= 0) return 0;

    double iou = inter / (area_a + area_b - inter);
    double ioa = inter / area_a;
    double iob = inter / area_b;

    if (iou == 0.0 && ioa == 0.0 && iob == 0.0) return 0;
    if (ioa >= 1.0) return -1;
    if (iob >= 1.0) return 1;
    if (iou >= iou_thresh || iob > io_thresh || ioa > io_thresh) return 2;

    return 0;
}

bool is_block(const Mat& clip, double threshold) {
    if (clip.rows < 10 || clip.cols < 10) return false;

    const int side = 4;
    auto check_border = [&](int start, int end, int step, bool is_row) -> bool {
        int blank_count = 0;
        for (int i = start; i != end; i += step) {
            double pixel_sum;
            if (is_row) {
                pixel_sum = static_cast<double>(sum(clip.row(i))[0]) / 255.0;
                if (pixel_sum > threshold * clip.cols) blank_count++;
            } else {
                pixel_sum = static_cast<double>(sum(clip.col(i))[0]) / 255.0;
                if (pixel_sum > threshold * clip.rows) blank_count++;
            }
        }
        return blank_count <= 2;
    };

    if (!check_border(side + 1, side + 5, 1, true))  return false;
    if (!check_border(side + 1, side + 5, 1, false)) return false;
    int bs = clip.rows - 4;
    if (!check_border(bs - 4, bs, 1, true))  return false;
    int rs = clip.cols - 4;
    if (!check_border(rs - 4, rs, 1, false)) return false;

    return true;
}

} // namespace

vector<UIComponent> extract_contours(const Mat& binary,
                                     const Phase2Config& cfg) {
    Mat work = binary.clone();

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(work, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    vector<UIComponent> compos;
    compos.reserve(contours.size());

    for (size_t i = 0; i < contours.size(); ++i) {
        double area = contourArea(contours[i]);
        if (area < cfg.min_obj_area) continue;

        Rect br = boundingRect(contours[i]);
        if (br.width <= cfg.min_comp_width || br.height <= cfg.min_comp_height)
            continue;

        compos.push_back(UIComponent(br));
    }
    return compos;
}

vector<UIComponent> filter_components(const vector<UIComponent>& compos,
                                      const Size& img_size,
                                      const Phase2Config& cfg) {
    double max_height = img_size.height * cfg.max_height_ratio;
    vector<UIComponent> result;
    result.reserve(compos.size());

    for (const auto& c : compos) {
        if (c.area < cfg.min_obj_area) continue;
        if (c.bbox.height > max_height) continue;

        double ratio_h = static_cast<double>(c.bbox.width)  / c.bbox.height;
        double ratio_w = static_cast<double>(c.bbox.height) / c.bbox.width;

        if (ratio_h > cfg.max_wh_ratio || ratio_w > cfg.max_hw_ratio) continue;

        int min_side = min(c.bbox.width, c.bbox.height);
        if (min_side < cfg.thin_side_min && max(ratio_h, ratio_w) > cfg.thin_ratio_max)
            continue;

        result.push_back(c);
    }
    return result;
}

vector<UIComponent> merge_intersected(const vector<UIComponent>& compos,
                                      const Phase2Config& cfg) {
    vector<UIComponent> current = compos;
    bool changed = true;

    while (changed) {
        changed = false;
        vector<UIComponent> temp;
        temp.reserve(current.size());

        for (const auto& ca : current) {
            bool merged = false;
            for (auto& cb : temp) {
                int rel = bbox_relation_nms(ca.bbox, cb.bbox,
                                            cfg.nms_iou_threshold,
                                            cfg.nms_ioa_threshold);
                if (rel == 2) {
                    int x1 = min(ca.bbox.x, cb.bbox.x);
                    int y1 = min(ca.bbox.y, cb.bbox.y);
                    int x2 = max(ca.bbox.x + ca.bbox.width,  cb.bbox.x + cb.bbox.width);
                    int y2 = max(ca.bbox.y + ca.bbox.height, cb.bbox.y + cb.bbox.height);
                    cb.bbox = Rect(x1, y1, x2 - x1, y2 - y1);
                    cb.area = cb.bbox.width * cb.bbox.height;
                    merged  = true;
                    changed = true;
                    break;
                }
            }
            if (!merged) temp.push_back(ca);
        }
        current = move(temp);
    }
    return current;
}

void recognize_blocks(const Mat& binary,
                      vector<UIComponent>& compos,
                      const Phase2Config& cfg) {
    int img_h = binary.rows;
    int img_w = binary.cols;

    for (auto& comp : compos) {
        double h_ratio = static_cast<double>(comp.bbox.height) / img_h;
        double w_ratio = static_cast<double>(comp.bbox.width)  / img_w;

        if (h_ratio > cfg.block_side_ratio && w_ratio > cfg.block_side_ratio) {
            Rect safe_roi = comp.bbox & Rect(0, 0, img_w, img_h);
            Mat clip = binary(safe_roi);
            if (is_block(clip, cfg.block_blank_threshold))
                comp.category = "Block";
        }
    }
}

vector<UIComponent> remove_contained_non_block(const vector<UIComponent>& compos) {
    size_t n = compos.size();
    vector<bool> marked(n, false);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            int rel = bbox_relation_nms(compos[i].bbox, compos[j].bbox, 0.02, 0.2);
            if (rel == -1 && compos[j].category != "Block") marked[i] = true;
            if (rel ==  1 && compos[i].category != "Block") marked[j] = true;
        }
    }

    vector<UIComponent> result;
    result.reserve(n);
    for (size_t i = 0; i < n; ++i)
        if (!marked[i]) result.push_back(compos[i]);
    return result;
}

void detect_containment(vector<UIComponent>& compos) {
    size_t n = compos.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            int rel = bbox_relation_nms(compos[i].bbox, compos[j].bbox, 0.02, 0.2);
            if (rel == -1) compos[j].contain.push_back(compos[i].id);
            else if (rel == 1) compos[i].contain.push_back(compos[j].id);
        }
    }
}

void assign_ids(vector<UIComponent>& compos) {
    for (size_t i = 0; i < compos.size(); ++i)
        compos[i].id = static_cast<int>(i) + 1;
}

vector<UIComponent> detect_components_full(const Mat& binary,
                                           const Phase2Config& cfg) {
    auto compos = extract_contours(binary, cfg);
    compos = filter_components(compos, binary.size(), cfg);
    compos = merge_intersected(compos, cfg);
    recognize_blocks(binary, compos, cfg);
    compos = remove_contained_non_block(compos);
    assign_ids(compos);
    detect_containment(compos);
    return compos;
}

vector<Rect> detect_components(const Mat& binary, const Phase2Config& cfg) {
    auto compos = detect_components_full(binary, cfg);
    vector<Rect> rects;
    rects.reserve(compos.size());
    for (const auto& c : compos)
        rects.push_back(c.bbox);
    return rects;
}
