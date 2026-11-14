# Research notes — image-histogram-equalizer

How histogram equalization, CLAHE and histogram matching work, why they
work, and where the numbers in this repository come from. Every claim
below is traceable to the references at the bottom; nothing here is
invented.

## 1. The histogram as a probability distribution

A grayscale image with L = 256 levels has histogram h(r_k) = n_k, the
number of pixels at level r_k. Dividing by the pixel count N gives the
estimated probability p(r_k) = n_k / N. All three methods in this
project are point operations driven by this distribution: they never
look at a pixel's neighbours (except CLAHE, which looks at the tile
the pixel belongs to), they only remap levels through a curve derived
from the histogram.

## 2. Global histogram equalization (HE)

Gonzalez & Woods (ch. 3) derive HE from the idea that the "ideal"
output image uses every gray level equally often. The transformation
is the image's own cumulative distribution function (CDF):

    s_k = T(r_k) = (L-1) * sum_{j=0..k} p(r_j)

i.e. the output level is the CDF value scaled to 0..255. In the
discrete implementation used here (and in most textbooks) the darkest
occupied level is pinned to 0 by subtracting the first non-zero CDF
value:

    s_k = round( (cdf(k) - cdf_min) / (N - cdf_min) * 255 )

The mapping is non-decreasing, so HE never inverts brightness order,
and it is exact only in the continuous limit: with discrete levels the
output histogram is spread out but not perfectly flat. Two
consequences, both verified by the test suite and the examples:

- An image that already uses its levels uniformly is (almost)
  unchanged: the 128x128 gradient example has PSNR 51.21 dB after HE
  (MSE 0.49 — rounding only). See `tests/test_equalize.cpp`
  (256x1 gradient is exactly the identity).
- HE uses *global* statistics, so it cannot fix an image that is dark
  in one corner and bright in another: one curve cannot serve both.
  The bimodal example shows the limit — HE stretches the two modes to
  0..255 but cannot reveal variation *inside* each mode any better
  than the global CDF allows.

HE also does not preserve mean brightness: the dark example (mean 25)
comes out at mean 127.53. Variants that preserve brightness
(BBHE, DSIHE) exist but are out of scope here.

## 3. Adaptive HE and CLAHE

Adaptive histogram equalization (AHE) computes the equalization curve
from a local neighbourhood ("contextual region") around each pixel
instead of the whole image (Pizer et al. 1987). AHE fixes the local
contrast problem but has a well-known failure: in a nearly flat
neighbourhood the local histogram is a spike, its CDF is a near-step,
and the mapping amplifies sensor noise into visible grain.

CLAHE (contrast-limited AHE) adds one mechanism to AHE: before the
CDF of a tile is computed, the tile histogram is clipped at a ceiling
(the clip limit) and the clipped-off counts are redistributed evenly
over all bins. Clipping caps the slope of the CDF, and the CDF slope
*is* the local contrast gain, so the noise amplification is bounded.
Zuiderveld (1994) adds the second ingredient used here: instead of
computing a curve per pixel, compute one curve per tile of a grid
(8x8 is the usual default) and obtain each pixel's value by bilinear
interpolation between the four surrounding tile centres, which removes
tile-boundary seams without per-pixel histograms.

Parameters and their trade-offs (Zuiderveld 1994; OpenCV docs):

- Tile grid: 8x8 is the default. Finer grids adapt more locally but
  each tile's histogram is estimated from fewer pixels (noisier curves)
  and the redistribution pedestal (below) matters more.
- Clip limit: expressed here, as in OpenCV, as a multiple of the
  tile histogram's average bin count (default 2.0). Higher values
  approach unclipped AHE (stronger contrast, more noise); lower values
  approach the identity. Values 1..4 cover most practical uses.

A subtlety worth documenting honestly: the redistribution adds a
roughly constant pedestal to every bin of the tile histogram, which
raises cdf_min and compresses the output range when tiles are small.
On the low-contrast example (tiles of 16x16 px, clip 2.0) CLAHE maps
the input range 100..140 to 102..222, while global HE reaches the
full 0..255. That is the algorithm behaving as specified, not a bug:
the pedestal is the price of the contrast cap. Larger tiles or a
higher clip limit widen the output range again.

CLAHE does not preserve a one-to-one relationship between input and
output gray values (Zuiderveld 1994), so CLAHE images are not suited
to quantitative measurements that rely on the physical meaning of
intensity.

## 4. Histogram matching (specification)

Matching generalises HE: instead of the uniform distribution, the
image is remapped so its histogram follows a chosen target — a
Gaussian, or the histogram of a reference image. The mapping is CDF
inversion: each source level s maps to the smallest target level t
whose target CDF is >= the source CDF at s (Gonzalez & Woods, ch. 3).
With a uniform target this reduces to HE in the continuous limit; in
the discrete implementation the two differ because the target
histogram is apportioned over 256 bins (largest-remainder
apportionment, so the counts sum to exactly N). The 4x4 hand-computed
case in `tests/test_matching.cpp` pins this down: a source with
h(0)=8, h(255)=8 matched to a uniform target of 16 counts maps
0 -> 7 and 255 -> 15, because the discrete uniform target has one
count in bins 0..15 and cdfT(7) = 8.

## 5. Entropy as a diagnostic, not a score

Shannon entropy H = -sum p(k) log2 p(k) (Shannon 1948) measures how
spread out the gray-level distribution is; a perfectly uniform
256-level histogram has H = 8 bits. HE and CLAHE often raise entropy,
but not always: the low-contrast example already uses its 41 levels
almost uniformly (H = 5.36 of a log2(41) = 5.36 maximum), and both HE
and CLAHE keep H = 5.36 while tripling the standard deviation. The
mapping is injective on occupied levels, so the histogram keeps its
shape and the entropy is unchanged even though the contrast improves
dramatically. The test suite therefore asserts entropy only as a
sanity bound, never as "higher is better".

## 6. Verification performed

- 1191 assert-based checks in `tests/` (run with `make test`),
  including hand-computed cases documented in the test files:
  a 4x4 image (HE LUT 0 -> 0, 50 -> 170, 100 -> 255; mean 25.0,
  stddev sqrt(1250) = 35.3553, entropy 1.29879 bits), CLAHE
  clip redistribution (1000-count spike, clip 2.0 -> bin 100 holds
  12, total preserved), CLAHE bilinear interpolation on a split
  50/200 image (tile centres 52 and 202, midpoint blend 51),
  matching to a uniform target (0 -> 7, 255 -> 15) and to a
  constant reference (everything -> 100), and MSE/PSNR
  (single-pixel difference of 1 in 2x2 -> MSE 0.25, PSNR 54.15 dB).
- Degenerate cases: constant image (HE identity), full gradient
  (HE identity), 1x1-tile CLAHE with a huge clip limit == global HE.
- CLI smoke tests (`tests/run_tests.sh`): all five subcommands plus
  batch mode on the example images.
- Example outputs in `examples/RESULTS.md` are the real output of
  `ihe compare` on the generated files — no numbers were typed by hand.

## References

1. Gonzalez, R. C., & Woods, R. E. *Digital Image Processing*.
   4th edition, Pearson, 2018. ISBN 978-0133356723. Chapter 3
   (intensity transformations and spatial filtering) is the source
   for the HE formula and histogram specification used here.
2. Pizer, S. M., Amburn, E. P., Austin, J. D., Cromartie, R.,
   Geselowitz, A., Greer, T., ter Haar Romeny, B. M., Zimmerman, J. B.,
   & Zuiderveld, K. "Adaptive histogram equalization and its
   variations." *Computer Vision, Graphics, and Image Processing*
   39(3), 355–368 (1987). DOI: 10.1016/S0734-189X(87)80186-X —
   https://doi.org/10.1016/S0734-189X(87)80186-X
3. Zuiderveld, K. "Contrast Limited Adaptive Histogram Equalization."
   In P. Heckbert (ed.), *Graphics Gems IV*, Academic Press, 1994,
   pp. 474–485. ISBN 0-12-336155-9. (Scan of the chapter:
   https://www.cse.unr.edu/~bebis/CS474/StudentPaperPresentations/Constrast-Limited%20Adaptive%20Histogram%20Enhancement.pdf)
4. Netpbm. "PGM Format Specification" (P5/P2, maxval rules, 1- vs
   2-byte samples). http://netpbm.sourceforge.net/doc/pgm.html
   (accessed 2026-11-14).
5. Shannon, C. E. "A Mathematical Theory of Communication."
   *Bell System Technical Journal* 27, 379–423, 623–656 (1948).
   Source of the entropy definition used in `stats`.
6. ITU-R Recommendation BT.601-7, "Studio encoding parameters of
   digital video for standard 4:3 and wide-screen 16:9 aspect
   ratios." Source of the luma weights 0.299 R + 0.587 G + 0.114 B
   used to convert PPM colour input to grayscale.
7. "Adaptive histogram equalization." Wikipedia (overview of AHE /
   CLAHE, interpolation and clipping, with the same primary sources
   as above). https://en.wikipedia.org/wiki/Adaptive_histogram_equalization
   (accessed 2026-11-14).
