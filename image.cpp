/*
 * Thanks to
 * original code can be found here :
 * https://www.lost-infinity.com/night-sky-image-processing-part-1-noise-reduction-using-anisotropic-diffusion-with-c/
 * https://github.com/carsten0x51h/astro_tools
 *
 * we might have to give a try with this one, same author :
 * https://github.com/carsten0x51h/focus_finder
 *
 *
 */
#include <basedevice.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <cmath>
#include <fitsio.h>
#include <opencv2/opencv.hpp>
#include <tuple>
#include <functional>
#include <list>
#include <set>
#include <array>
#include <vector>


#include "image.h"



HICImage::HICImage() {
    ResetData();
}
HICImage::~HICImage() {
    ResetData();
}
void HICImage::ResetData(void) {
    isLoaded = false;
    xsize = 0;
    ysize = 0;
    nbpix = 0;
    bits = 0;
    min = 0;
    max = 0;
    mean = 0;
    variance = 0;
    stddev = 0;
    vollathF4 = 0;
    hfd = 0;
}


bool HICImage::LoadFromBlob(IBLOB *bp)
{
    printf("readblob %s %s %i \n",bp->label,bp->name,bp->size);
    img=nullptr;
    isLoaded=false;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus = 0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};
    size_t bsize = static_cast<size_t>(bp->bloblen);

    // load blob to CFITSIO
    if (fits_open_memfile(&fptr,
            "",
            READONLY,
            &(bp->blob),
            &bsize,
            0,
            nullptr,
            &status))
    {
        printf("Unsupported type or read error loading FITS file");
        ResetData();
        return false;
    }

    if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU)
    {
        printf("FITS file is not of an image");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }

    // Get HDUs and size
    fits_get_img_dim(fptr, &naxis, &status);
    fits_get_img_size(fptr, 2, fits_size, &status);
    xsize = (int) fits_size[0];
    ysize = (int) fits_size[1];
    nbpix = xsize*ysize;
    fits_get_num_hdus(fptr,&nhdus,&status);

    if ((nhdus != 1) || (naxis != 2))
    {
        printf("Unsupported type or read error loading FITS file");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }
    // Read image
    ImageData = nullptr;
    ImageData = new unsigned short[nbpix];
    if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, nullptr, ImageData, nullptr, &status))
    {
        printf("Error reading data");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }
    CalcStats();
    isLoaded = true;
    return true;
}
bool HICImage::LoadFromFile(char *filename)
{
    //printf("readFile %s\n",filename);
    img=nullptr;
    isLoaded=false;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus = 0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};
    // load file to CFITSIO
    if (fits_open_diskfile(&fptr,filename,READONLY,&status))
    {
        printf("Unsupported type or read error loading FITS file");
        ResetData();
        return false;
    }

    if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU)
    {
        printf("FITS file is not of an image");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }

    // Get HDUs and size
    fits_get_img_dim(fptr, &naxis, &status);
    fits_get_img_size(fptr, 2, fits_size, &status);
    xsize = (int) fits_size[0];
    ysize = (int) fits_size[1];
    nbpix = xsize*ysize;
    fits_get_num_hdus(fptr,&nhdus,&status);

    if ((nhdus != 1) || (naxis != 2))
    {
        printf("Unsupported type or read error loading FITS file");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }
    // Read image
    ImageData = nullptr;
    ImageData = new unsigned short[nbpix];
    if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, nullptr, ImageData, nullptr, &status))
    {
        printf("Error reading data");
        fits_close_file(fptr,&status);
        ResetData();
        return false;
    }

    isLoaded = true;
    CalcStats();
    return true;
}

void HICImage::CalcStats(void)
{
    img.clear();
    //Loading into CImg
    img.resize(xsize,ysize,1,1);
    cimg_forXY(img, x, y)
        {
            img(x, img.height() - y - 1) = ImageData[img.offset(x, y)];
        }

    min = img.min();
    max = img.max();
    mean = img.mean();
    med = img.median();
    variance = img.variance(1);
    stddev=sqrt(variance);

    CImg<float> binImg;

    binImg =  nullptr;
    StarInfoListT starInfos;
    //std::vector < std::list<StarInfoT *> > starBuckets;
    thresholdOtsu(img, 16, & binImg);
    clusterStars(binImg, & starInfos);

    // Calc brightness boundaries for possible focusing stars
    //float maxPossiblePixValue = pow(2.0, bitPix) - 1;
    float maxPossiblePixValue = pow(2.0, 16) - 10;
    // For each star
    /* outerHfdDiameter depends on pixel size and focal length (and seeing...).
       Later we may calculate it automatically wihth goven focal length and pixel
       size of the camera. For now it is a "best guess" value.
    */
    const unsigned int outerHfdDiameter = 20;
    for (StarInfoListT::iterator it = starInfos.begin(); it != starInfos.end(); ++it) {
      const FrameT & frame = it->clusterFrame;
      FrameT & cogFrame = it->cogFrame;
      FrameT & hfdFrame = it->hfdFrame;
      PixSubPosT & cogCentroid = it->cogCentroid;
      PixSubPosT & subPixelInterpCentroid = it->subPixelInterpCentroid;
      float & hfd = it->hfd;
      float & fwhmHorz = it->fwhmHorz;
      float & fwhmVert = it->fwhmVert;
      float & maxPixValue = it->maxPixValue;
      bool & saturated = it->saturated;
      //if (maxPixValue > 65000) saturated=true;

      if (saturated==false) {
          FrameT squareFrame = rectify(frame);

          // Centroid calculation --> In: Handle to full noise reduced image, subimg-boundaries (x1,y1,x2,y2), Out: (x,y) - abs. centroid coordinates
          calcCentroid(img, squareFrame, & cogCentroid, & subPixelInterpCentroid, 10 /* num iterations */);
          std::get<0>(cogCentroid) += std::get<0>(squareFrame);
          std::get<1>(cogCentroid) += std::get<1>(squareFrame);
          std::get<0>(subPixelInterpCentroid) += std::get<0>(squareFrame);
          std::get<1>(subPixelInterpCentroid) += std::get<1>(squareFrame);


          // Calculate cog boundaries
          float maxClusterEdge = std::max(fabs(std::get<0>(frame) - std::get<2>(frame)), fabs(std::get<1>(frame) - std::get<3>(frame)));
          float cogHalfEdge = ceil(maxClusterEdge / 2.0);
          float cogX = std::get<0>(cogCentroid);
          float cogY = std::get<1>(cogCentroid);
          std::get<0>(cogFrame) = cogX - cogHalfEdge - 1;
          std::get<1>(cogFrame) = cogY - cogHalfEdge - 1;
          std::get<2>(cogFrame) = cogX + cogHalfEdge + 1;
          std::get<3>(cogFrame) = cogY + cogHalfEdge + 1;


          // HFD calculation --> In: image, Out: HFD value
          // Subtract mean value from image which is required for HFD calculation
          size_t hfdRectDist = floor(outerHfdDiameter / 2.0);
          std::get<0>(hfdFrame) = cogX - hfdRectDist;
          std::get<1>(hfdFrame) = cogY - hfdRectDist;
          std::get<2>(hfdFrame) = cogX + hfdRectDist;
          std::get<3>(hfdFrame) = cogY + hfdRectDist;

          CImg<float> hfdSubImg = img.get_crop(std::get<0>(hfdFrame), std::get<1>(hfdFrame), std::get<2>(hfdFrame), std::get<3>(hfdFrame));
          maxPixValue = hfdSubImg.max();
          //saturated = (maxPixValue > lowerBound && maxPixValue < upperBound);
          saturated = (maxPixValue >= maxPossiblePixValue);

          CImg<float> imgHfdSubMean(hfdSubImg);
          double mean = hfdSubImg.mean();

          cimg_forXY(hfdSubImg, x, y) {
              imgHfdSubMean(x, y) = (hfdSubImg(x, y) < mean ? 0 : hfdSubImg(x, y) - mean);
          }

          // Calc the HFD
          hfd = calcHfd(imgHfdSubMean, outerHfdDiameter /*outer diameter in px*/);

          // FWHM calculation --> In: Handle to full noise reduced image, abs. centroid coordinates, Out: FWHM value
          MyDataContainerT vertDataPoints, horzDataPoints;

          cimg_forX(imgHfdSubMean, x) {
              horzDataPoints.push_back(std::make_pair(x, imgHfdSubMean(x, floor(imgHfdSubMean.height() / 2.0 + 0.5))));
          }
          cimg_forY(imgHfdSubMean, y) {
              vertDataPoints.push_back(std::make_pair(y, imgHfdSubMean(floor(imgHfdSubMean.width() / 2.0 + 0.5), y)));
          }

          // Do the LM fit
          typedef CurveFitTmplT<GaussianFitTraitsT> GaussMatcherT;
          typedef GaussMatcherT::CurveParamsT CurveParamsT;
          CurveParamsT::TypeT gaussCurveParmsHorz, gaussCurveParmsVert;

          GaussMatcherT::fitGslLevenbergMarquart<MyDataAccessorT>(horzDataPoints, & gaussCurveParmsHorz, 0.1f /*EpsAbs*/, 0.1f /*EpsRel*/);
          fwhmHorz = gaussCurveParmsHorz[CurveParamsT::W_IDX];

          GaussMatcherT::fitGslLevenbergMarquart<MyDataAccessorT>(vertDataPoints, & gaussCurveParmsVert, 0.1f /*EpsAbs*/, 0.1f /*EpsRel*/);
          fwhmVert = gaussCurveParmsVert[CurveParamsT::W_IDX];
          hfdSubImg.clear();
          imgHfdSubMean.clear();

      }
    }

    int i = 0;
    for (StarInfoListT::iterator it = starInfos.begin(); it != starInfos.end(); ++it) {
        i++;
        StarInfoT * curStarInfo = & (*it);
        hfd = ((i-1)*hfd +curStarInfo->hfd)/i;
        //printf("%f %f %f\n", std::get<0>(it->cogCentroid), std::get<1>(it->cogCentroid),curStarInfo->hfd);
    }

    printf("min=%i;max=%i;mean=%f;med=%f;var=%f;stddev=%f;nbstars=%lu;hfd=%f\n",min,max,mean,med,variance,stddev,starInfos.size(),hfd);


}

void HICImage::FindStars(void)
{

}

void HICImage::thresholdOtsu(const CImg<float> & inImg, long inBitPix, CImg<float> * outBinImg)
{
  CImg<> hist = inImg.get_histogram(pow(2.0, inBitPix));

  float sum = 0;
  cimg_forX(hist, pos) { sum += pos * hist[pos]; }

  float numPixels = inImg.width() * inImg.height();
  float sumB = 0, wB = 0, max = 0.0;
  float threshold1 = 0.0, threshold2 = 0.0;

  cimg_forX(hist, i) {
    wB += hist[i];

    if (! wB) { continue; }

    float wF = numPixels - wB;

    if (! wF) { break; }

    sumB += i * hist[i];

    float mF = (sum - sumB) / wF;
    float mB = sumB / wB;
    float diff = mB - mF;
    float bw = wB * wF * pow(diff, 2.0);

    if (bw >= max) {
      threshold1 = i;
      if (bw > max) {
         threshold2 = i;
      }
      max = bw;
    }
  } // end loop

  float th = (threshold1 + threshold2) / 2.0;

  *outBinImg = inImg; // Create a copy
  outBinImg->threshold(th);
}

void HICImage::getAndRemoveNeighbours(PixelPosT inCurPixelPos, PixelPosSetT * inoutWhitePixels, PixelPosListT * inoutPixelsToBeProcessed)
{
  const size_t _numPixels = 8, _x = 0, _y = 1;
  const int offsets[_numPixels][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 },
                                       { -1, 0 },              { 1, 0 },
                                       { -1, 1 }, { 0, 1 }, { 1, 1 } };

  for (size_t p = 0; p < _numPixels; ++p) {
    PixelPosT curPixPos(std::get<0>(inCurPixelPos) + offsets[p][_x], std::get<1>(inCurPixelPos) + offsets[p][_y]);
    PixelPosSetT::iterator itPixPos = inoutWhitePixels->find(curPixPos);

    if (itPixPos != inoutWhitePixels->end()) {
      const PixelPosT & curPixPos = *itPixPos;
      inoutPixelsToBeProcessed->push_back(curPixPos);
      inoutWhitePixels->erase(itPixPos); // Remove white pixel from "white set" since it has been now processed
    }
  }
  return;
}

template<typename T> void HICImage::clusterStars(const CImg<T> & inImg, StarInfoListT * outStarInfos)
{
  PixelPosSetT whitePixels;

  cimg_forXY(inImg, x, y) {
    if (inImg(x, y)) {
      whitePixels.insert(whitePixels.end(), PixelPosT(x, y));
    }
  }

  // Iterate over white pixels as long as set is not empty
  while (whitePixels.size()) {
    PixelPosListT pixelsToBeProcessed;

    PixelPosSetT::iterator itWhitePixPos = whitePixels.begin();
    pixelsToBeProcessed.push_back(*itWhitePixPos);
    whitePixels.erase(itWhitePixPos);

    FrameT frame(inImg.width(), inImg.height(), 0, 0);

    while(! pixelsToBeProcessed.empty()) {
      PixelPosT curPixelPos = pixelsToBeProcessed.front();

      // Determine boundaries (min max in x and y directions)
      if (std::get<0>(curPixelPos) /*x*/ < std::get<0>(frame) /*x1*/) {	std::get<0>(frame) = std::get<0>(curPixelPos); }
      if (std::get<0>(curPixelPos) /*x*/ > std::get<2>(frame) /*x2*/) { std::get<2>(frame) = std::get<0>(curPixelPos); }
      if (std::get<1>(curPixelPos) /*y*/ < std::get<1>(frame) /*y1*/) {	std::get<1>(frame) = std::get<1>(curPixelPos); }
      if (std::get<1>(curPixelPos) /*y*/ > std::get<3>(frame) /*y2*/) { std::get<3>(frame) = std::get<1>(curPixelPos); }

      getAndRemoveNeighbours(curPixelPos, & whitePixels, & pixelsToBeProcessed);
      pixelsToBeProcessed.pop_front();
    }

    // Create new star-info and set cluster-frame.
    // NOTE: we may use new to avoid copy of StarInfoT...
    StarInfoT starInfo;
    starInfo.clusterFrame = frame;
    outStarInfos->push_back(starInfo);

  }
}

void HICImage::calcCentroid(const CImg<float> & inImg, const FrameT & inFrame, PixSubPosT * outPixelPos, PixSubPosT * outSubPixelPos, size_t inNumIterations)
{
  // Get frame sub img
  CImg<float> subImg = inImg.get_crop(std::get<0>(inFrame), std::get<1>(inFrame), std::get<2>(inFrame), std::get<3>(inFrame));

  float & xc = std::get<0>(*outPixelPos);
  float & yc = std::get<1>(*outPixelPos);

  // 1. Calculate the IWC
  calcIntensityWeightedCenter(subImg, & xc, & yc);

  if (outSubPixelPos) {
    // 2. Round to nearest integer and then iteratively improve.
    int xi = floor(xc + 0.5);
    int yi = floor(yc + 0.5);

    CImg<float> img3x3 = inImg.get_crop(xi - 1 /*x0*/, yi - 1 /*y0*/, xi + 1 /*x1*/, yi + 1 /*y1*/);

    // 3. Interpolate using sub-pixel algorithm
    float xsc = xi, ysc = yi;
    calcSubPixelCenter(img3x3, & xsc, & ysc, inNumIterations);

    std::get<0>(*outSubPixelPos) = xsc;
    std::get<1>(*outSubPixelPos) = ysc;
  }
}

FrameT HICImage::rectify(const FrameT & inFrame)
{
  float border = 3;
  float border2 = 2.0 * border;
  float width = fabs(std::get<0>(inFrame) - std::get<2>(inFrame)) + border2;
  float height = fabs(std::get<1>(inFrame) - std::get<3>(inFrame)) + border2;
  float L = std::max(width, height);
  float x0 = std::get<0>(inFrame) - (fabs(width - L) / 2.0) - border;
  float y0 = std::get<1>(inFrame) - (fabs(height - L) / 2.0) - border;
  return FrameT(x0, y0, x0 + L, y0 + L);
}

/**
* Get all pixels inside a radius: http://stackoverflow.com/questions/14487322/get-all-pixel-array-inside-circle
* Algorithm: http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
*/
bool insideCircle(float inX /*pos of x*/, float inY /*pos of y*/, float inCenterX, float inCenterY, float inRadius)
{
  return (pow(inX - inCenterX, 2.0) + pow(inY - inCenterY, 2.0) <= pow(inRadius, 2.0));
}

float HICImage::calcHfd(const CImg<float> & inImage, unsigned int inOuterDiameter)
{
  // Sum up all pixel values in whole circle
  float outerRadius = inOuterDiameter / 2;
  float sum = 0, sumDist = 0;
  int centerX = ceil(inImage.width() / 2.0);
  int centerY = ceil(inImage.height() / 2.0);

  cimg_forXY(inImage, x, y) {
    if (insideCircle(x, y, centerX, centerY, outerRadius)) {
      sum += inImage(x, y);
      sumDist += inImage(x, y) * sqrt(pow((float) x - (float) centerX, 2.0f) + pow((float) y - (float) centerY, 2.0f));
    }
  }
  // NOTE: Multiplying with 2 is required since actually just the HFR is calculated above
  return (sum ? 2.0 * sumDist / sum : sqrt(2.0) * outerRadius);
}

float calcIx2(const CImg<float> & img, int x)
{
  float Ix = 0;
  cimg_forY(img, y) { Ix += pow(img(x, y), 2.0) * (float) x; }
  return Ix;
}

float calcJy2(const CImg<float> & img, int y)
{
  float Iy = 0;
  cimg_forX(img, x) { Iy += pow(img(x, y), 2.0) * (float) y; }
  return Iy;
}

// Calculate Intensity Weighted Center (IWC)
void calcIntensityWeightedCenter(const CImg<float> & inImg, float * outX, float * outY)
{
  assert(outX && outY);

  // Determine weighted centroid - See http://cdn.intechopen.com/pdfs-wm/26716.pdf
  float Imean2 = 0, Jmean2 = 0, Ixy2 = 0;

  for(size_t i = 0; i < inImg.width(); ++i) {
    Imean2 += calcIx2(inImg, i);
    cimg_forY(inImg, y) { Ixy2 += pow(inImg(i, y), 2.0); }
  }

  for(size_t i = 0; i < inImg.height(); ++i) {
    Jmean2 += calcJy2(inImg, i);
  }

  *outX = Imean2 / Ixy2;
  *outY = Jmean2 / Ixy2;
}

void calcSubPixelCenter(const CImg<float> & inImg, float * outX, float * outY, size_t inNumIter/*num iterations*/)
{
  // Sub pixel interpolation
  float c, a1, a2, a3, a4, b1, b2, b3, b4;
  float a1n, a2n, a3n, a4n, b1n, b2n, b3n, b4n;

  assert(inImg.width() == 3 && inImg.height() == 3);

  b1 = inImg(0, 0); a2 = inImg(1, 0); b2 = inImg(2, 0);
  a1 = inImg(0, 1);  c = inImg(1, 1); a3 = inImg(2, 1);
  b4 = inImg(0, 2); a4 = inImg(1, 2); b3 = inImg(2, 2);

  for (size_t i = 0; i < inNumIter; ++i) {
    float c2 = 2 * c;
    float sp1 = (a1 + a2 + c2) / 4;
    float sp2 = (a2 + a3 + c2) / 4;
    float sp3 = (a3 + a4 + c2) / 4;
    float sp4 = (a4 + a1 + c2) / 4;

    // New maximum is center
    float newC = std::max({ sp1, sp2, sp3, sp4 });

    // Calc position of new center
    float ad = pow(2.0, -((float) i + 1));

    if (newC == sp1) {
      *outX = *outX - ad; // to the left
      *outY = *outY - ad; // to the top

      // Calculate new sub pixel values
      b1n = (a1 + a2 + 2 * b1) / 4;
      b2n = (c + b2 + 2 * a2) / 4;
      b3n = sp3;
      b4n = (b4 + c + 2 * a1) / 4;
      a1n = (b1n + c + 2 * a1) / 4;
      a2n = (b1n + c + 2 * a2) / 4;
      a3n = sp2;
      a4n = sp4;

    } else if (newC == sp2) {
      *outX = *outX + ad; // to the right
      *outY = *outY - ad; // to the top

      // Calculate new sub pixel values
      b1n = (2 * a2 + b1 + c) / 4;
      b2n = (2 * b2 + a3 + a2) / 4;
      b3n = (2 * a3 + b3 + c) / 4;
      b4n = sp4;
      a1n = sp1;
      a2n = (b2n + c + 2 * a2) / 4;
      a3n = (b2n + c + 2 * a3) / 4;
      a4n = sp3;
    } else if (newC == sp3) {
      *outX = *outX + ad; // to the right
      *outY = *outY + ad; // to the bottom

      // Calculate new sub pixel values
      b1n = sp1;
      b2n = (b2 + 2 * a3 + c) / 4;
      b3n = (2 * b3 + a3 + a4) / 4;
      b4n = (2 * a4 + b4 + c) / 4;
      a1n = sp4;
      a2n = sp2;
      a3n = (b3n + 2 * a3 + c) / 4;
      a4n = (b3n + 2 * a4 + c) / 4;
    } else {
      *outX = *outX - ad; // to the left
      *outY = *outY + ad; // to the bottom

      // Calculate new sub pixel values
      b1n = (2 * a1 + b1 + c) / 4;
      b2n = sp2;
      b3n = (c + b3 + 2 * a4) / 4;
      b4n = (2 * b4 + a1 + a4) / 4;
      a1n = (b4n + 2 * a1 + c) / 4;
      a2n = sp1;
      a3n = sp3;
      a4n = (b4n + 2 * a4 + c) / 4;
    }

    c = newC; // Oi = Oi+1

    a1 = a1n;
    a2 = a2n;
    a3 = a3n;
    a4 = a4n;

    b1 = b1n;
    b2 = b2n;
    b3 = b3n;
    b4 = b4n;
  }
}
