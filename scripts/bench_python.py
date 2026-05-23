"""
Python UIED benchmark — measures Phase 1 and Phase 2 latency using time.perf_counter.
Outputs one line per benchmark in the format:
  BENCHMARK_NAME<TAB>mean_ms<TAB>median_ms<TAB>stddev_ms<TAB>min_ms<TAB>max_ms<TAB>iterations
Called by bench_compare.sh — do not run directly unless PYTHONPATH is set.
"""
import sys
import os
import time
import statistics
import cv2

UIED_ROOT  = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'UIED'))
ASSET_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'assets', 'test_ui.png'))
ITERATIONS = 30

sys.path.insert(0, UIED_ROOT)

import detect_compo.lib_ip.ip_preprocessing as pre
import detect_compo.lib_ip.ip_detection as det
import detect_compo.lib_ip.Component as Compo


def run(fn, n=ITERATIONS):
    times = []
    for _ in range(n):
        t0 = time.perf_counter()
        fn()
        times.append((time.perf_counter() - t0) * 1000)
    return times


def emit(name, times):
    mean   = statistics.mean(times)
    median = statistics.median(times)
    std    = statistics.stdev(times) if len(times) > 1 else 0.0
    print(f"{name}\t{mean:.3f}\t{median:.3f}\t{std:.3f}\t{min(times):.3f}\t{max(times):.3f}\t{len(times)}")


if __name__ == '__main__':
    img = cv2.imread(ASSET_PATH)
    if img is None:
        print(f"ERROR: cannot read {ASSET_PATH}", file=sys.stderr)
        sys.exit(1)

    # warmup
    pre.binarization(img, grad_min=10)

    binary_ref = pre.binarization(img, grad_min=10)

    emit("BM_Phase1_Binarization",
         run(lambda: pre.binarization(img, grad_min=10)))

    def phase2():
        b = binary_ref.copy()
        c = det.component_detection(b, min_obj_area=50)
        c = det.compo_filter(c, min_area=50, img_shape=b.shape)
        c = det.merge_intersected_compos(c)
        det.compo_block_recognition(b, c)
        c = det.rm_contained_compos_not_in_block(c)
        Compo.compos_update(c, b.shape)
        Compo.compos_containment(c)

    emit("BM_Phase2_FullDetection", run(phase2))

    def full():
        b = pre.binarization(img, grad_min=10)
        c = det.component_detection(b, min_obj_area=50)
        c = det.compo_filter(c, min_area=50, img_shape=b.shape)
        c = det.merge_intersected_compos(c)
        det.compo_block_recognition(b, c)
        c = det.rm_contained_compos_not_in_block(c)
        Compo.compos_update(c, b.shape)
        Compo.compos_containment(c)

    emit("BM_FullPipeline", run(full))
