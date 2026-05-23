#include <benchmark/benchmark.h>
#include <opencv2/opencv.hpp>

#include "uied_phase1.hpp"
#include "uied_phase2.hpp"

using namespace cv;

static void BM_Phase1_Binarization(benchmark::State& state) {
    Mat img = imread("../assets/test_ui.png", IMREAD_COLOR);
    if (img.empty()) { state.SkipWithError("Image not found"); return; }

    for (auto _ : state) {
        Mat output = process_binarization(img, 10);
        benchmark::DoNotOptimize(output);
    }
}

static void BM_Phase2_ComponentDetection(benchmark::State& state) {
    Mat img = imread("../assets/test_ui.png", IMREAD_COLOR);
    if (img.empty()) { state.SkipWithError("Image not found"); return; }

    Mat binary = process_binarization(img, 10);
    Phase2Config cfg;

    for (auto _ : state) {
        auto compos = detect_components(binary, cfg);
        benchmark::DoNotOptimize(compos);
    }
}

static void BM_Phase2_FullDetection(benchmark::State& state) {
    Mat img = imread("../assets/test_ui.png", IMREAD_COLOR);
    if (img.empty()) { state.SkipWithError("Image not found"); return; }

    Mat binary = process_binarization(img, 10);
    Phase2Config cfg;

    for (auto _ : state) {
        auto compos = detect_components_full(binary, cfg);
        benchmark::DoNotOptimize(compos);
    }
}

static void BM_FullPipeline(benchmark::State& state) {
    Mat img = imread("../assets/test_ui.png", IMREAD_COLOR);
    if (img.empty()) { state.SkipWithError("Image not found"); return; }

    Phase2Config cfg;

    for (auto _ : state) {
        Mat binary = process_binarization(img, 10);
        auto compos = detect_components_full(binary, cfg);
        benchmark::DoNotOptimize(compos);
    }
}

BENCHMARK(BM_Phase1_Binarization)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Phase2_ComponentDetection)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Phase2_FullDetection)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_FullPipeline)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
