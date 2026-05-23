#include <benchmark/benchmark.h>
#include <opencv2/opencv.hpp>
#include "uied_phase1.hpp"

using namespace cv;

static void BM_Phase1_Processing(benchmark::State& state) {
    Mat img = imread("../assets/test_ui.png", IMREAD_COLOR);
    if (img.empty()) {
        state.SkipWithError("Không tìm thấy ảnh test!");
        return;
    }

    for (auto _ : state) {
        Mat output = process_binarization(img, 10);
        benchmark::DoNotOptimize(output);
    }
}

BENCHMARK(BM_Phase1_Processing)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
