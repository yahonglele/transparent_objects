/*
 * pinholeCamera.cpp
 *
 *  Created on: Oct 14, 2011
 *      Author: Ilya Lysenkov
 */

#include "edges_pose_refiner/pinholeCamera.hpp"
#include <opencv2/opencv.hpp>

using namespace cv;

PinholeCamera::PinholeCamera(const cv::Mat &_cameraMatrix, const cv::Mat &_distCoeffs, const PoseRT &_extrinsics, const cv::Size &_imageSize)
{
  cameraMatrix = _cameraMatrix;
  distCoeffs = _distCoeffs;
  extrinsics = _extrinsics;
  imageSize = _imageSize;
}

void PinholeCamera::projectPoints(const std::vector<cv::Point3f> &points, const PoseRT &pose_cam, std::vector<cv::Point2f> &projectedPoints) const
{
  PoseRT fullPose = extrinsics * pose_cam;
  cv::projectPoints(Mat(points), fullPose.getRvec(), fullPose.getTvec(), cameraMatrix, distCoeffs, projectedPoints);
}

PinholeCamera::PinholeCamera(const PinholeCamera &camera)
{
  *this = camera;
}

PinholeCamera& PinholeCamera::operator=(const PinholeCamera &camera)
{
  if (this != &camera)
  {
    cameraMatrix = camera.cameraMatrix.clone();
    distCoeffs = camera.distCoeffs.clone();
    extrinsics = camera.extrinsics;
    imageSize = camera.imageSize;
  }
  return *this;
}

void PinholeCamera::write(const std::string &filename) const
{
  FileStorage fs(filename, FileStorage::WRITE);
  if(!fs.isOpened())
  {
    CV_Error(CV_StsBadArg, "Cannot open pinhole camera file: " + filename);
  }
  write(fs);
  fs.release();
}

void PinholeCamera::write(cv::FileStorage &fs) const
{
  fs << "camera" << "{";
  fs << "K" << cameraMatrix;
  fs << "D" << distCoeffs;
  fs << "width" << imageSize.width;
  fs << "height" << imageSize.height;

  fs << "pose" << "{";
  fs << "rvec" << extrinsics.getRvec();
  fs << "tvec" << extrinsics.getTvec();
  fs << "}" << "}";
}

void PinholeCamera::read(const std::string &filename)
{
  FileStorage fs(filename, FileStorage::READ);
  if(!fs.isOpened())
  {
    CV_Error(CV_StsBadArg, "Cannot open pinhole camera file: " + filename);
  }

  read(fs.root());
  fs.release();
}

void PinholeCamera::read(const cv::FileNode &fn)
{
  fn["camera"]["K"] >> cameraMatrix;
  fn["camera"]["D"] >> distCoeffs;
  fn["camera"]["width"] >> imageSize.width;
  fn["camera"]["height"] >> imageSize.height;

  Mat rvec, tvec;
  fn["camera"]["pose"]["rvec"] >> rvec;
  fn["camera"]["pose"]["tvec"] >> tvec;
  
  Mat rvec64, tvec64;
  rvec.convertTo(rvec64, CV_64FC1);
  tvec.convertTo(tvec64, CV_64FC1);
  extrinsics = PoseRT(rvec64, tvec64);
}