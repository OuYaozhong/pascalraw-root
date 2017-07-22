#include <math.h>
#include "mex.h"

/// Global Parameters

/// Binary Parameters
/// 1 for log gradients, 0 for linear gradients
int LOG_GRADIENTS = 0;
/// 1 for normalization, 0 for no normalization
int NORMALIZATION = 1;
/// 1 for truncation, 0 for no truncation
int TRUNCATION = 0;
/// 1 for noise, 0 for no noise
int NOISE = 0;

/// Scalar Parameters
/// fullscale log gradient magnitude for log gradient quantization
double LOG_GRADIENT_FULLSCALE_MAGNITUDE = log(8);
/// bitdepth of log gradients
int LOG_GRADIENT_BITDEPTH = 12;
/// bitdepth of image before gradient extraction 
int IMAGE_BITDEPTH = 12;
/// bitdepth of input images (typically 12 for RAW images and 8 for JPG images)
int ORIGINAL_IMAGE_BITDEPTH = 12;

/// exposure scale factor
int EXPOSURE_SCALE_FACTOR = 1;

/// effective bitdepth of noise
int NOISE_BITDEPTH = 8;
/// 0.2887 = 1/sqrt(12)
double NOISE_STANDARD_DEVIATION = (0.2887/(1 << NOISE_BITDEPTH))*((1 << (int) ORIGINAL_IMAGE_BITDEPTH) - 1);

// small value, used to avoid division by zero
#define eps 0.0001

// unit vectors used to compute gradient orientation
/// note that this assumes 18 orientation bins
double uu[9] = {1.0000, 
		0.9397, 
		0.7660, 
		0.500, 
		0.1736, 
		-0.1736, 
		-0.5000, 
		-0.7660, 
		-0.9397};
double vv[9] = {0.0000, 
		0.3420, 
		0.6428, 
		0.8660, 
		0.9848, 
		0.9848, 
		0.8660, 
		0.6428, 
		0.3420};

double randd()
{
  return (((double)rand())/(double)RAND_MAX);
}

double box_muller(double m, double s)  /* normal random variate generator */
{               /* mean m, standard deviation s */
  double x1, x2, w, y1;
  static double y2;
  static int use_last = 0;

  if (use_last)           /* use value from previous call */
  {
    y1 = y2;
    use_last = 0;
  }
  else
  {
    do {
      x1 = 2.0 * randd() - 1.0;
      x2 = 2.0 * randd() - 1.0;
      w = x1 * x1 + x2 * x2;
    } while ( w >= 1.0 );

    w = sqrt( (-2.0 * log( w ) ) / w );
    y1 = x1 * w;
    y2 = x2 * w;
    use_last = 1;
  }

  return( m + y1 * s );
}

static inline double minDouble(double x, double y) { return (x <= y ? x : y); }
static inline double maxDouble(double x, double y) { return (x <= y ? y : x); }

static inline float minFloat(float x, float y) { return (x <= y ? x : y); }
static inline float maxFloat(float x, float y) { return (x <= y ? y : x); }

static inline int minInt(int x, int y) { return (x <= y ? x : y); }
static inline int maxInt(int x, int y) { return (x <= y ? y : x); }

static inline double clipDouble(double value, double minimum, double maximum) { return minDouble(maxDouble(value, minimum), maximum); }
static inline float clipFloat(float value, float minimum, float maximum) { return minFloat(maxFloat(value, minimum), maximum); }
static inline int clipInt(int value, int minimum, int maximum) { return minInt(maxInt(value, minimum), maximum); }

static inline double quantize(double value, double bitdepth, double fullscale) { return round(value*(pow(2.0, bitdepth)-0.5-eps)/fullscale); }

static inline int decrease_bitdepth(int value, int original_bitdepth, int final_bitdepth) { 
  int final_value = value >> (original_bitdepth-final_bitdepth); 
  return final_value;
}

// main function:
// takes a double color image and a bin size 
// returns HOG features
mxArray *process(const mxArray *mximage, const mxArray *mxsbin) {

  /// create deep copy of image
  const mxArray *mximagecopy = mxDuplicateArray(mximage);

  double *im = (double *)mxGetPr(mximagecopy);
  const int *dims = mxGetDimensions(mximagecopy);
  const int numPixels = mxGetNumberOfElements(mximagecopy);
  if (mxGetNumberOfDimensions(mximagecopy) != 3 ||
      dims[2] != 3 ||
      mxGetClassID(mximagecopy) != mxDOUBLE_CLASS)
    mexErrMsgTxt("Invalid input");

  int sbin = (int)mxGetScalar(mxsbin);

  // memory for caching orientation histograms & their norms
  /// 'blocks' here refers to 'cells' as defined in Dalal & Triggs.
  /// note that the use of "round" here is a little odd. basically, if the value is rounded up,
  /// it is possible to have more cells than there are corresponding pixels. 
  int blocks[2];
  blocks[0] = (int)round((double)dims[0]/(double)sbin);
  blocks[1] = (int)round((double)dims[1]/(double)sbin);
  /// memory for histogram (18 signed orientation bins)
  float *hist = (float *)mxCalloc(blocks[0]*blocks[1]*18, sizeof(float));
  float *norm = (float *)mxCalloc(blocks[0]*blocks[1], sizeof(float));

  // memory for HOG features
  int out[3];
  /// blocks[0]-2 and blocks[1]-2 accounts for the fact that the actual blocks (as defined by 
  /// Dalal & Triggs) are of 2x2 cells (referred to as blocks here) and have 50% overlap. 
  /// 27 + 4 + 1 refers to the enhanced HOG features described in "From Rigid Templates to Grammars"
  /// pages 27-29. 18 signed bins + 9 unsigned bins = 27, 4 texture features, and 1 truncation feature.
  out[0] = maxInt(blocks[0]-2, 0);
  out[1] = maxInt(blocks[1]-2, 0);
  out[2] = 27+4+1;
  mxArray *mxfeat = mxCreateNumericArray(3, out, mxSINGLE_CLASS, mxREAL);
  float *feat = (float *)mxGetPr(mxfeat);
  
  /// visible includes all pixels that belong to cells. 
  /// note that in the case that the value of blocks[0] or blocks[1] was determined by a 
  /// rounding up, visible[0] or visible[1] can actually be larger than dims[0] or dims[1].
  /// so actually, visible is a bad name. 
  int visible[2];
  visible[0] = blocks[0]*sbin;
  visible[1] = blocks[1]*sbin;

  /// scale exposure
  if (EXPOSURE_SCALE_FACTOR != 1) {
    int maxPixelValue = (1 << ORIGINAL_IMAGE_BITDEPTH) - 1;
    for (int i = 0; i < numPixels; i++) {
      im[i] = (double) clipInt((int) im[i] * EXPOSURE_SCALE_FACTOR, 0, maxPixelValue);
    }
  }

  /// add noise
  if (NOISE) {
    int maxPixelValue = (1 << ORIGINAL_IMAGE_BITDEPTH) - 1;
    for (int i = 0; i < numPixels; i++) {
      im[i] = im[i] + box_muller(0, NOISE_STANDARD_DEVIATION);
      im[i] = clipInt((int) im[i], 0, maxPixelValue);
    }
  }

  /// decrease image bitdepth
  if (IMAGE_BITDEPTH != ORIGINAL_IMAGE_BITDEPTH) {
    for (int i = 0; i < numPixels; i++) {
      im[i] = (double) decrease_bitdepth( (int) im[i], ORIGINAL_IMAGE_BITDEPTH, IMAGE_BITDEPTH);
    }
  }

  /// log compress image
  if (LOG_GRADIENTS) {
    for (int i = 0; i < numPixels; i++) {
        // add eps to avoid log(0) = -inf
        im[i] = log(im[i] + eps);
    }
  }
  
  /// compute gradients and contrast sensitive orientation bins for each pixel, then add
  /// these to the histograms of the four nearest cells using linear interpolation.
  for (int x = 1; x < visible[1]-1; x++) {
    for (int y = 1; y < visible[0]-1; y++) {
      // first color channel
      /// this line is odd. basically if blocks[0] or blocks[1] was the result of rounding down, 
      /// x or y will always be returned by min, and some pixels at the far right or bottom of the
      /// image will not contribute to cells. but if blocks[0] or blocks[1] was the result of 
      /// rounding up, then dims[1]-2 or dims[0]-2 will be returned by min for the values of x and 
      /// y that exceed the actual image dimensions (minus 1, since the gradient must be taken with
      /// a kernel of [-1 0 1] or [-1; 0; 1]). In these cases, and the gradients corresponding to the
      /// furthest right or bottom pixels of the image may be counted more than once for the furthest 
      /// right or bottom cells. 
      double *s = im + minInt(x, dims[1]-2)*dims[0] + minInt(y, dims[0]-2);
      double dy = *(s+1) - *(s-1);
      double dx = *(s+dims[0]) - *(s-dims[0]);
      double v = fabs(dx) + fabs(dy);

      // second color channel
      s += dims[0]*dims[1];
      double dy2 = *(s+1) - *(s-1);
      double dx2 = *(s+dims[0]) - *(s-dims[0]);
      double v2 = fabs(dx2) + fabs(dy2);

      // third color channel
      s += dims[0]*dims[1];
      double dy3 = *(s+1) - *(s-1);
      double dx3 = *(s+dims[0]) - *(s-dims[0]);
      double v3 = fabs(dx3) + fabs(dy3);

      // pick channel with strongest gradient
      if (v2 > v) {
        v = v2;
        dx = dx2;
        dy = dy2;
      } 
      if (v3 > v) {
        v = v3;
        dx = dx3;
        dy = dy3;
      }

      /// quantize log gradients
      if (LOG_GRADIENTS) {
        /// note that 1 is subtracted from LOG_GRADIENT_BITDEPTH since gradients are signed, and thus require a sign bit.
        /// also, the clipDouble function is used before quantization to ensure all inputs are in range.
        dx = quantize(clipDouble(dx, -LOG_GRADIENT_FULLSCALE_MAGNITUDE, LOG_GRADIENT_FULLSCALE_MAGNITUDE), (double) LOG_GRADIENT_BITDEPTH - 1, LOG_GRADIENT_FULLSCALE_MAGNITUDE);
        dy = quantize(clipDouble(dy, -LOG_GRADIENT_FULLSCALE_MAGNITUDE, LOG_GRADIENT_FULLSCALE_MAGNITUDE), (double) LOG_GRADIENT_BITDEPTH - 1, LOG_GRADIENT_FULLSCALE_MAGNITUDE);
        v = fabs(dx) + fabs(dy);
      }

      // snap to one of 18 orientations
      double best_dot = 0;
      int best_o = 0;
      for (int o = 0; o < 9; o++) {
        double dot = uu[o]*dx + vv[o]*dy;
        if (dot > best_dot) {
          best_dot = dot;
          best_o = o;
        } else if (-dot > best_dot) {
          best_dot = -dot;
          best_o = o+9;
        }
      }
      
      // add to 4 histograms around pixel using linear interpolation
      /// vx0, vy0, vx1, and vy1 are used to weight the pixel gradient magnitude
      /// contribution to the cell histogram based on the pixel's spatial 
      /// distance from the cell. 
      double xp = ((double)x+0.5)/(double)sbin;
      double yp = ((double)y+0.5)/(double)sbin;
      int ixp = (int)floor(xp);
      int iyp = (int)floor(yp);

      if (ixp >= 0 && iyp >= 0 && ixp < blocks[1] && iyp < blocks[0]) {
        *(hist + ixp*blocks[0] + iyp + best_o*blocks[0]*blocks[1]) += v;
      }
    }
  }

  // compute energy in each block by summing over orientations
  /// For each cell, the sum of magnitudes for each orientation is added to the sum of magnitudes for 
  /// the opposing orientation (orientation + 180 degrees) and this sum is squared. The 9 squared sums 
  /// for each cell are added together to obtain the total cell (histogram) energy.
  for (int o = 0; o < 9; o++) {
    float *src1 = hist + o*blocks[0]*blocks[1];
    float *src2 = hist + (o+9)*blocks[0]*blocks[1];
    float *dst = norm;
    float *end = norm + blocks[1]*blocks[0];
    while (dst < end) {
      *(dst++) += (*src1 + *src2) * (*src1 + *src2);
      src1++;
      src2++;
    }
  }

  // compute features
  for (int x = 0; x < out[1]; x++) {
    for (int y = 0; y < out[0]; y++) {
      float *dst = feat + x*out[0] + y;      
      float *src, *p, n1, n2, n3, n4;

      /// p is a pointer into the array of cell histogram energies (norm)
      /// n1, n2, n3, and n4 are inverses of the sums of 4 cell histogram energies corresponding 2x2 cell
      /// blocks to which the cell described by (x,y) belongs. The cell described by (x,y) is the top-left,
      /// bottom-left, top-right, and bottom-right cell in the 2x2 cell blocks corresponding to n1, n2, n3, 
      /// and n4, respectively.
      p = norm + (x+1)*blocks[0] + y+1;
      n1 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + (x+1)*blocks[0] + y;
      n2 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + x*blocks[0] + y+1;
      n3 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
      p = norm + x*blocks[0] + y;      
      n4 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);

      float t1 = 0;
      float t2 = 0;
      float t3 = 0;
      float t4 = 0;

      // contrast-sensitive features
      src = hist + (x+1)*blocks[0] + (y+1);
      for (int o = 0; o < 18; o++) {
        /*if (NORMALIZATION) {
          /// Truncation at 0.2 as described in "From Rigid Templates to Grammars," page 25.
          float h1 = minFloat(*src * n1, 0.2);
          float h2 = minFloat(*src * n2, 0.2);
          float h3 = minFloat(*src * n3, 0.2);
          float h4 = minFloat(*src * n4, 0.2);
          /// Analytic projection (sum) as described in "From Rigid Templates to Grammars," page 27.
          *dst = 0.5 * (h1 + h2 + h3 + h4);
          /// Computing texture features by summing all normalized gradient magnitudes (for all orientations)
          /// in each cell as described in "From Rigid Templates to Grammars," page 27.
          t1 += h1;
          t2 += h2;
          t3 += h3;
          t4 += h4;
        } else {
          *dst = *src;
        }*/

        /// Set all contrast-sensitive features to 0
        *dst = 0;

        dst += out[0]*out[1];
        src += blocks[0]*blocks[1];
      }

      // contrast-insensitive features
      src = hist + (x+1)*blocks[0] + (y+1);
      for (int o = 0; o < 9; o++) {
        /// Adding the sum of magnitudes for each orientation to the sum of magnitudes for the opposing
        /// orientation (orientation + 180 degrees) to achieve contrast-insensitivity
        float sum = *src + *(src + 9*blocks[0]*blocks[1]);
        if (NORMALIZATION) {
          /// Truncation at 0.2 as described in "From Rigid Templates to Grammars," page 25.
          float h1 = TRUNCATION ? minFloat(sum * n1, 0.2) : sum * n1;
          float h2 = TRUNCATION ? minFloat(sum * n2, 0.2) : sum * n2;
          float h3 = TRUNCATION ? minFloat(sum * n3, 0.2) : sum * n3;
          float h4 = TRUNCATION ? minFloat(sum * n4, 0.2) : sum * n4;
          /// Analytic projection (sum) as described in "From Rigid Templates to Grammars," page 27.
          *dst = 0.5 * (h1 + h2 + h3 + h4);
          /// Computing texture features by summing all normalized gradient magnitudes (for all orientations)
          /// in each cell as described in "From Rigid Templates to Grammars," page 27.
          t1 += h1;
          t2 += h2;
          t3 += h3;
          t4 += h4;
        } else {
          *dst = sum;
        }
        dst += out[0]*out[1];
        src += blocks[0]*blocks[1];
      }

      // texture features
      /// Overall gradient energy in square blocks of 4 cells around (x,y) as described in 
      /// "From Rigid Templates to Grammars," page 27. The significance of 0.2357 is unclear.
      if (NORMALIZATION) {
        *dst = 0.2357 * t1;
        dst += out[0]*out[1];
        *dst = 0.2357 * t2;
        dst += out[0]*out[1];
        *dst = 0.2357 * t3;
        dst += out[0]*out[1];
        *dst = 0.2357 * t4;
      } else {
        *dst = 0;
        dst += out[0]*out[1];
        *dst = 0;
        dst += out[0]*out[1];
        *dst = 0;
        dst += out[0]*out[1];
        *dst = 0;
      }

      // truncation feature
      /// truncation feature as described in "From Rigid Templates to Grammars," page 28.
      dst += out[0]*out[1];
      *dst = 0;
    }
  }

  mxFree(hist);
  mxFree(norm);
  return mxfeat;
}

// matlab entry point
// F = features(image, bin)
// image should be color with double values
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) { 
  if (nrhs != 2)
    mexErrMsgTxt("Wrong number of inputs"); 
  if (nlhs != 1)
    mexErrMsgTxt("Wrong number of outputs");
  plhs[0] = process(prhs[0], prhs[1]);
}



