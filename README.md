# RawForge

[![CI](https://github.com/colenator123-design/rawforge-cpp-isp/actions/workflows/ci.yml/badge.svg)](https://github.com/colenator123-design/rawforge-cpp-isp/actions/workflows/ci.yml)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C)
![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C)
![License](https://img.shields.io/badge/License-MIT-green)

**High-Performance C++ Quad-Bayer Remosaic & RAW Denoising Pipeline**

RawForge 是一套以 C++20 從零實作的 RAW image processing prototype，題目取自 MIPI 2022 Quad Joint Remosaic and Denoise Challenge：將含感光雜訊的 Quad-Bayer RAW 轉換成標準 Bayer RAW，讓後續 ISP 能正常執行 demosaicing、white balance 與 color processing。

> 第一版 scalar CPU pipeline 在可重現的合成 RAW benchmark 上，PSNR 從 **20.063 dB 提升到 20.421 dB（+0.358 dB）**。所有核心影像 kernel 均以標準 C++ 實作，不依賴 OpenCV。

## 結果展示

![RawForge comparison](docs/images/comparison.png)

由左到右分別是：

1. 乾淨 RGB ground truth
2. Noisy Quad-Bayer 被直接當成 Bayer 顯示，可看到嚴重 CFA 色彩與雜訊 artifact
3. Color-aware remosaic baseline
4. Same-color RAW denoise + remosaic

下排為相同區域的放大比較。第四張在平滑漸層中的彩色顆粒較少，同時保留圓形與細格邊緣。

## 公開問題來源

MIPI 2022 官方任務要求將 Quad CFA interpolation 成 full-resolution Bayer，並同時處理 RAW noise。官方資料包含 70 個 training scenes、15 個 validation scenes，以及 0／24／42 dB 三種 noise levels；評估指標包括 PSNR、SSIM、LPIPS 與 KLD。

- [MIPI 2022 官方 repository](https://github.com/mipi-challenge/MIPI2022)
- [Quad-Bayer Challenge report](https://arxiv.org/abs/2209.07060)
- [CodaLab Challenge](https://codalab.lisn.upsaclay.fr/competitions/4955)

競賽本身已結束，而且官方資料需登入並同意研究用途條款。因此 repository 只提交自行產生的合成 RAW 與程式碼，不重新發布官方影像。

## 為什麼需要 Remosaic

一般 RGGB Bayer 每個 `2×2` 區塊包含 R、G、G、B：

```text
R G R G
G B G B
R G R G
G B G B
```

Quad-Bayer 則把相同顏色排列成 `2×2` block：

```text
R R G G
R R G G
G G B B
G G B B
```

Quad-Bayer 有利於 low-light pixel binning，但不能直接交給預期標準 Bayer pattern 的 ISP。Remosaic 必須在 full resolution 重新估計正確 CFA 位置；若只改像素排列，很容易產生 false color、zipper artifact 與細節損失。

## Pipeline

```text
Linear RGB ground truth
        │
        ├── Standard Bayer ──────────────── Evaluation target
        │
        └── Quad-Bayer
               │
               ▼
        Poisson-Gaussian noise
               │
               ▼
      Same-color bilateral denoise
               │
               ▼
      Color-aware full-res remosaic
               │
               ▼
          Standard Bayer
               │
               ▼
       Bilinear demosaic preview
```

### 1. Sensor noise model

合成資料使用 signal-dependent noise：

```text
sigma(pixel) = read_sigma + shot_scale × sqrt(signal)
noisy_pixel  = clamp(signal + sigma × N(0, 1))
```

亮度相關的 shot noise 與固定 read noise，比對所有像素加入相同 Gaussian noise 更接近真實 sensor 行為。固定 random seed 讓測試可以完全重現。

### 2. Same-color bilateral RAW denoising

Filter 只在相同 Quad CFA color 的鄰居之間取樣，避免直接混合 R、G、B sensor measurements。權重同時考慮空間距離與像素差異：

```text
weight = spatial_weight × range_weight
```

因此平坦區域會獲得較強降噪，高反差邊緣則較不容易被抹平。

### 3. Color-aware remosaic

每個輸出位置先查詢標準 RGGB Bayer 應有的顏色。如果 Quad CFA 該位置已有相同顏色就保留量測值；否則只從鄰近相同顏色 samples 做距離加權插值。

## 實測效能

Release build，Apple M3 CPU，單執行緒 scalar C++：

| Resolution | Megapixels | Remosaic baseline | Joint pipeline | Joint ms/MP | PSNR gain |
|---:|---:|---:|---:|---:|---:|
| 320×240 | 0.0768 | 6.42 ms | 28.70 ms | 373.76 | +0.334 dB |
| 640×480 | 0.3072 | 26.96 ms | 121.47 ms | 395.40 | **+0.358 dB** |
| 1280×720 | 0.9216 | 83.31 ms | 371.15 ms | 402.73 | +0.356 dB |

完整數據位於 [`benchmarks/apple_m3_release.csv`](benchmarks/apple_m3_release.csv)。數值是目前 correctness-first baseline，不代表 MIPI 官方 leaderboard 成績；下一階段將以 tiling、multithreading 與 ARM NEON 降低 latency。

## Build and Run

需求：支援 C++20 的 compiler 與 CMake 3.20 以上。

```bash
git clone https://github.com/colenator123-design/rawforge-cpp-isp.git
cd rawforge-cpp-isp

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

產生 RAW、RGB previews 與 metrics：

```bash
./build/rawforge_demo outputs/demo
python3 scripts/make_comparison.py
```

執行多解析度 benchmark：

```bash
./build/rawforge_benchmark
```

## 專案結構

```text
rawforge-cpp-isp/
├── include/rawforge/
│   ├── image.hpp       # Contiguous generic image buffer
│   ├── cfa.hpp         # Quad-RGGB and Bayer-RGGB layouts
│   ├── pipeline.hpp    # Denoise, remosaic and demosaic API
│   ├── synthetic.hpp   # Test chart and sensor noise
│   ├── metrics.hpp     # MSE and PSNR
│   └── io.hpp          # PGM / PPM output
├── src/                # C++ implementations
├── apps/
│   ├── demo.cpp
│   └── benchmark.cpp
├── tests/test_main.cpp
├── scripts/make_comparison.py
└── benchmarks/
```

## 測試內容

- Quad-RGGB 與 Bayer-RGGB CFA mapping
- Constant RAW 經過 denoise／remosaic 後保持不變
- Joint pipeline 必須比 remosaic-only baseline 有更高 PSNR
- 固定 seed 必須產生相同 sensor noise
- CI 在 Ubuntu Release build 中編譯、測試並執行完整 demo

另外可啟用 AddressSanitizer 與 UndefinedBehaviorSanitizer：

```bash
cmake -S . -B build-asan \
  -DRAWFORGE_ENABLE_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

## 技術重點

- C++20、RAII、contiguous image storage
- CFA-aware neighborhood sampling
- RAW-domain bilateral filtering
- Full-resolution Quad-Bayer remosaic
- Poisson-Gaussian sensor noise simulation
- PSNR-based reproducible evaluation
- CMake、CTest、sanitizers、GitHub Actions

## Roadmap

1. Cache tiling 與避免重複 CFA／weight 計算
2. `std::execution`／OpenMP multi-threading
3. Apple Silicon ARM NEON SIMD kernels
4. Edge-directed interpolation，降低 zipper artifact
5. 讀取 10／12／14-bit packed RAW
6. 加入 SSIM 與 MIPI 官方資料 adapter
7. ONNX Runtime C++ learned residual refinement

## License and Data Policy

RawForge 程式碼使用 MIT License，與 MIPI Challenge 主辦單位無隸屬關係。MIPI datasets 僅限學術研究及非商業用途，且不得重新散布；請從官方 CodaLab 頁面申請與下載。

