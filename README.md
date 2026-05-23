# Tạo thư mục build riêng để không rác source code

mkdir build && cd build

# Sinh cấu hình và Build

cmake ..
make

# Chạy thử để lấy ảnh Output

./run_main

# Chạy Benchmark để lấy số liệu hiệu năng

./run_benchmark
