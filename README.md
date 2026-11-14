# image-histogram-equalizer

Ekualisasi histogram global (HE), CLAHE, dan histogram matching —
ditulis dari nol dalam C++17, tanpa dependensi eksternal. Gambar
dibaca/ditulis sebagai Netpbm (PGM/PBM/PPM), jadi tidak butuh library
gambar apa pun.

## Latar belakang

Histogram sebuah citra grayscale adalah perkiraan distribusi
probabilitas level keabuannya. Ketiga metode di proyek ini memakai
distribusi itu sebagai kurva pemetaan nada:

- **Histogram equalization (HE)** memakai fungsi distribusi kumulatif
  (CDF) citra itu sendiri sebagai kurva pemetaan, sehingga level yang
  tadinya menumpuk tersebar merata ke 0..255 (Gonzalez & Woods, bab 3).
- **CLAHE** membagi citra menjadi grid tile, menghitung kurva
  ekualisasi per tile, memotong histogram tile pada clip limit lalu
  mendistribusikan ulang kelebihannya (ini yang membatasi penguatan
  noise di daerah datar), dan memadukan kurva antar tile dengan
  interpolasi bilinear supaya tidak ada garis sambungan (Pizer dkk.
  1987; Zuiderveld 1994).
- **Histogram matching** memetakan citra agar histogramnya mengikuti
  target yang dipilih: distribusi uniform, Gaussian, atau histogram
  sebuah citra referensi (inversi CDF).

Detail teori, rumus, dan verifikasi hitungan tangan ada di
[`docs/RESEARCH.md`](docs/RESEARCH.md).

## Struktur proyek

```
image-histogram-equalizer/
├── src/
│   ├── image.h / image.cpp        # citra grayscale + I/O Netpbm (PGM/PBM/PPM)
│   ├── histogram.h / .cpp         # histogram 256-bin, CDF, cetak histogram
│   ├── equalize.h / .cpp          # HE global (LUT dari CDF)
│   ├── clahe.h / .cpp             # CLAHE: clip, tile, interpolasi bilinear
│   ├── matching.h / .cpp          # histogram matching (uniform/Gauss/referensi)
│   ├── stats.h / .cpp             # mean, stddev, min/max, entropi, MSE, PSNR
│   └── main.cpp                   # CLI (he, clahe, match, stats, compare)
├── tests/                         # suite tes assert-based, tanpa framework luar
│   ├── test_framework.h           # runner CHECK/CHECK_NEAR minimal
│   ├── test_histogram.cpp  test_equalize.cpp  test_clahe.cpp
│   ├── test_matching.cpp   test_stats.cpp     test_io.cpp
│   ├── test_main.cpp
│   └── run_tests.sh               # build + tes unit + smoke test CLI
├── examples/
│   ├── generate_examples.py       # 5 citra PGM sintetis 128x128 (deterministik)
│   ├── run_examples.sh            # regenerasi contoh + tulis RESULTS.md
│   ├── *.pgm                      # citra contoh
│   ├── outputs/                   # hasil HE dan CLAHE (file PGM beneran)
│   └── RESULTS.md                 # statistik sebelum/sesudah dari run beneran
├── docs/
│   └── RESEARCH.md                # teori + sumber + bukti verifikasi
├── Makefile                       # make / make test / make examples
├── CMakeLists.txt                 # alternatif build dengan CMake + CTest
├── LICENSE                        # MIT
└── README.md
```

## Cara build

Butuh compiler C++17 (g++ 13 teruji) dan `make` — atau CMake 3.16+:

```bash
make            # menghasilkan biner ./ihe
# atau:
cmake -S . -B build && cmake --build build
```

## Cara pakai

```
ihe he      <in> [out] [--ascii] [--out-dir DIR] [--batch DIR]
ihe clahe   <in> [out] [--tiles NxM] [--clip L] [--ascii] [--out-dir DIR] [--batch DIR]
ihe match   <in> <ref> <out> [--ascii]
ihe match   <in> [out] --target uniform|gaussian [--mu M] [--sigma S]
ihe stats   <in> [--hist]
ihe compare <a> <b>
```

Contoh:

```bash
./ihe he examples/low_contrast.pgm out_he.pgm
./ihe clahe examples/dark.pgm out_clahe.pgm --tiles 8x8 --clip 2.0
./ihe match a.pgm b.pgm matched.pgm            # ikuti histogram b.pgm
./ihe match a.pgm out.pgm --target gaussian --mu 127.5 --sigma 64
./ihe stats examples/bimodal.pgm --hist
./ihe compare original.pgm enhanced.pgm
./ihe he --batch examples --out-dir outputs    # proses semua .pgm/.ppm/.pbm
```

Tanpa `out`, output ditulis sebagai `<nama>_he.pgm` /
`<nama>_clahe.pgm` di samping input. Format yang didukung: PGM P5/P2
(natif), PBM P1/P4 (dibaca sebagai 0/255), PPM P3/P6 (dikonversi ke
grayscale dengan luma BT.601). Maxval 1..65535 diterima dan diskalakan
ke 0..255; output selalu PGM (P5, atau P2 dengan `--ascii`).

## Contoh output (dari run beneran)

Lima citra sintetis 128x128 di `examples/` diproses dengan HE dan
CLAHE (`--tiles 8x8 --clip 2.0`). Ringkasan dari
`examples/RESULTS.md` (angka lengkap di sana):

| Citra | Metode | mean | stddev | min–max | entropi |
|---|---|---|---|---|---|
| low_contrast (100–140) | sebelum | 120.00 | 11.83 | 100–140 | 5.36 |
| | HE | 127.47 | **75.46** | 0–255 | 5.36 |
| | CLAHE | 162.00 | 35.50 | 102–222 | 5.36 |
| dark (5–45) | sebelum | 25.00 | 11.83 | 5–45 | 5.36 |
| | HE | 127.53 | **75.44** | 0–255 | 5.36 |
| | CLAHE | 66.99 | 35.50 | 7–127 | 5.36 |
| bimodal (20–220) | sebelum | 119.95 | 80.91 | 20–220 | 6.36 |
| | HE | 127.52 | 74.52 | 0–255 | 6.36 |
| | CLAHE | 142.96 | 77.51 | 21–255 | **7.06** |
| gradient (0–255) | HE | 127.50 | 74.33 | 0–255 | 7.00 |
| checker (40–203) | CLAHE | 126.15 | 81.44 | 41–211 | 3.87 |

Yang patut diperhatikan (semuanya dijelaskan di `docs/RESEARCH.md`):

- **HE nyaris tidak mengubah gradient** (PSNR 51.21 dB, MSE 0.49):
  citra yang levelnya sudah uniform memang titik tetap HE.
- **HE tidak menjaga mean**: citra gelap (mean 25) keluar dengan mean
  127.53. Ini sifat algoritmanya, bukan bug.
- **CLAHE lebih lembut daripada HE** pada citra yang kontrasnya sudah
  tinggi (checker: PSNR 33.99 dB vs 15.95 dB untuk HE), tapi
  **menaikkan entropi bimodal** 6.36 -> 7.06 karena tiap tile
  meregangkan variasi di dalam modenya masing-masing — hal yang tidak
  bisa dilakukan HE global.
- **Entropi bukan skor**: HE menaikkan stddev low_contrast dari 11.83
  ke 75.46 sementara entropinya tetap 5.36, karena pemetaan injektif
  pada level yang terisi mempertahankan bentuk histogram.
- **CLAHE mengompres rentang output** pada tile kecil: low_contrast
  CLAHE hanya mencapai 102–222 (bukan 0–255) karena pedestal dari
  redistribusi clip menaikkan cdf_min. Makin besar tile atau makin
  tinggi clip limit, makin lebar lagi rentangnya.

## Tes

Suite tes assert-based tanpa framework eksternal
(`make test` atau `bash tests/run_tests.sh`, yang juga menjalankan
smoke test CLI):

```
image-histogram-equalizer unit tests: 1191/1191 checks passed -- OK
```

Tes mencakup kasus hitungan tangan yang didokumentasikan di tiap file
tes: gambar 4x4 (LUT HE 0 -> 0, 50 -> 170, 100 -> 255; mean 25.0,
stddev 35.3553, entropi 1.29879 bit), redistribusi clip CLAHE
(spike 1000 count, clip 2.0 -> bin 100 berisi 12, total tetap 1000),
interpolasi bilinear CLAHE pada citra belah 50/200 (pusat tile 52 dan
202, titik tengah campuran 51), matching ke target uniform
(0 -> 7, 255 -> 15) dan ke referensi konstan (semua -> 100), serta
MSE/PSNR (beda 1 piksel sebesar 1 pada 2x2 -> MSE 0.25, PSNR
54.15 dB). Kasus degenerat juga diuji: citra konstan (HE identitas),
gradient penuh (HE identitas), CLAHE 1x1 tile dengan clip besar yang
persis sama dengan HE global.

## Keputusan desain

- **C++17, nol dependensi.** Format Netpbm dipilih supaya proyek bisa
  membaca/menulis gambar tanpa library eksternal; konsekuensinya hanya
  grayscale yang diproses (PPM warna dikonversi ke luma BT.601).
- **Modul terpisah per algoritma** (`equalize`, `clahe`, `matching`,
  `stats`) dengan header yang mendokumentasikan rumus dan sumbernya;
  CLI hanyalah lapisan tipis di atas modul-modul itu.
- **Clip limit mengikuti konvensi OpenCV**: kelipatan dari rata-rata
  isi bin histogram tile (default 2.0), bukan jumlah absolut.
- **Redistribusi clip deterministik**: kelebihan dibagi rata ke semua
  bin, sisanya satu-satu ke bin terendah — hasil run dapat direproduksi
  persis.
- **Tes tanpa framework**: satu header `CHECK`/`CHECK_NEAR` agar
  `make test` jalan di mana saja tanpa instalasi tambahan.

## Keterbatasan & pengembangan

- Hanya grayscale 8-bit; citra warna diproses sebagai luma (tidak ada
  mode per-channel).
- Tidak ada dukungan JPEG/PNG (butuh library eksternal); konversi dari
  format itu bisa dilakukan dulu dengan alat Netpbm (`jpegtopnm`,
  `pngtopnm`).
- CLAHE memakai grid tile seragam; ukuran tile adaptif dan clip limit
  per tile (berbasis statistik lokal) belum ada.
- Belum ada BBHE/DSIHE (HE yang menjaga kecerahan) dan belum ada
  equalisasi berbasis histogram lokal berbobot (sliding window AHE).
- `match` dengan target Gaussian memakai apportionment
  largest-remainder; target dari file histogram eksternal belum
  didukung (baru dari citra referensi).
