/*
 * utils.cpp
 *
 *  Created on: Apr 23, 2011
 *      Author: Ilya Lysenkov
 */

#include "edges_pose_refiner/utils.hpp"
//#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/opencv.hpp>
//#include <visualization_msgs/Marker.h>
#include "edges_pose_refiner/poseRT.hpp"
//#include <posest/pnp_ransac.h>

#include <fstream>

using std::cout;
using std::endl;

using namespace cv;

//#define USE_RVIZ

//TODO: is Projective right name?
void createProjectiveMatrix(const cv::Mat &R, const cv::Mat &t, cv::Mat &Rt)
{
  CV_Assert(R.type() == CV_64FC1);
  CV_Assert(t.type() == CV_64FC1);

  Rt.create(4, 4, CV_64FC1);
  Rt.at<double>(3, 0) = 0.;
  Rt.at<double>(3, 1) = 0.;
  Rt.at<double>(3, 2) = 0.;
  Rt.at<double>(3, 3) = 1.;

  Mat roi_R = Rt(Range(0, 3), Range(0, 3));
  CV_Assert(roi_R.rows == 3 && roi_R.cols == 3);

  if(R.rows == 3 && R.cols == 3)
  {
    R.copyTo(roi_R);
  }
  else
  {
    Mat fullR;
    Rodrigues(R, fullR);
    fullR.copyTo(roi_R);
  }

  Mat roi_t = Rt(Range(0, 3), Range(3,4));
  t.copyTo(roi_t);
}

void getRvecTvec(const Mat &projectiveMatrix, Mat &rvec, Mat &tvec)
{
  CV_Assert(projectiveMatrix.rows == 4 && projectiveMatrix.cols == 4);

  Rodrigues(projectiveMatrix(Range(0, 3), Range(0, 3)), rvec);
  projectiveMatrix(Range(0, 3), Range(3, 4)).copyTo(tvec);

  CV_Assert(rvec.rows == 3 && rvec.cols == 1);
  CV_Assert(tvec.rows == 3 && tvec.cols == 1);
  CV_Assert(rvec.type() == CV_64FC1 && tvec.type() == CV_64FC1);
}


void getTransformationMatrix(const cv::Mat &R_obj2cam, const cv::Mat &t_obj2cam, const cv::Mat &rvec_Object, const cv::Mat &tvec_Object, cv::Mat &transformationMatrix)
{
  Mat Rt_obj2cam;
  createProjectiveMatrix(R_obj2cam, t_obj2cam, Rt_obj2cam);

  getTransformationMatrix(Rt_obj2cam, rvec_Object, tvec_Object, transformationMatrix);
}

void getTransformationMatrix(const cv::Mat &Rt_obj2cam, const cv::Mat &rvec_Object, const cv::Mat &tvec_Object, cv::Mat &transformationMatrix)
{
  Mat Rt_obj;
  createProjectiveMatrix(rvec_Object, tvec_Object, Rt_obj);

  transformationMatrix = Rt_obj2cam * Rt_obj * Rt_obj2cam.inv(DECOMP_SVD);
}

void getRotationTranslation(const cv::Mat &projectiveMatrix, cv::Mat &R, cv::Mat &t)
{
  projectiveMatrix(Range(0, 3), Range(0, 3)).copyTo(R);
  projectiveMatrix(Range(0, 3), Range(3, 4)).copyTo(t);
}

void vec2mats(const vector<double> &point6d, cv::Mat &rvec, cv::Mat &tvec)
{
  //TODO: change to size_t
  const int dim = 3;

  //TODO: fix const_cast
  switch(point6d.size())
  {
    case 2*dim:
    {
      rvec = Mat(3, 1, CV_64FC1, const_cast<double*>(point6d.data()));
      tvec = Mat(3, 1, CV_64FC1, const_cast<double*>(point6d.data() + dim));
      break;
    }
    case dim:
    {
      rvec = Mat::zeros(3, 1, CV_64FC1);
      tvec = Mat(3, 1, CV_64FC1, const_cast<double*>(point6d.data()));
      break;
    }
    default:
    {
      CV_Error(CV_StsBadArg, "Invalid dimensionality of the point");
    }
  }
}

/*
void findNearestPointsToBundle(const cv::Point2f &imagePoint, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, const std::vector<cv::Point3f> &pointCloud, int knn, std::vector<cv::Point3f> &nearestPoints)
{
//  Mat imagePointMat = Mat(imagePoint);
//  cout << imagePointMat.channels() << " " << imagePointMat.rows << " " << imagePointMat.cols << endl;
  vector<Point2f> undistortedPoints;
  undistortPoints(Mat(imagePoint).reshape(2), undistortedPoints, cameraMatrix, distCoeffs);

  for (size_t i = 0; i < pointCloud.size(); i++)
  {


  }

}
*/

void interpolatePointCloud(const cv::Mat &mask, const std::vector<cv::Point3f> &pointCloud, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, std::vector<cv::Point3f> &interpolatedPointCloud)
{
  interpolatedPointCloud.clear();

  vector<Point2f> projectedCloud;
  Mat zeros = Mat::zeros(3, 1, CV_64FC1);
  projectPoints(Mat(pointCloud), zeros, zeros, cameraMatrix, distCoeffs, projectedCloud);



  Mat undistortedProjectedCloud;
  undistortPoints(Mat(projectedCloud), undistortedProjectedCloud, cameraMatrix, distCoeffs);

/*
  Mat cameraMatrix_inv_t = cameraMatrix.inv().t();
  cout << "chan: " << undistortedPoints.channels() << endl;
  vector<Point3f> homogeneousUndistortedPoints;
  convertPointsHomogeneous(undistortedPoints, homogeneousUndistortedPoints);
  exit(-1);
*/

//  TODO: take into consideration distortion coefficients


  vector<Point2f> maskPoints;
  for (int i = 0; i < mask.rows; i++)
  {
    for (int j = 0; j < mask.cols; j++)
    {
      if(mask.at<uchar>(i, j) == 0)
        continue;

      maskPoints.push_back(Point2f(j, i));
    }
  }

/*
  Mat drawMask(mask.size(), CV_8UC1, Scalar(0));
  for (size_t i = 0; i < maskPoints.size(); i++)
  {
    Point pt(cvRound(maskPoints[i].x), cvRound(maskPoints[i].y));
    circle(drawMask, maskPoints[i], 0, Scalar(255));
  }

  Mat resizedDrawMask;
  resize(drawMask, resizedDrawMask, Size(), 0.5, 0.5);
  imshow("draw", resizedDrawMask);
  imwrite("draw.png", drawMask);
  waitKey();
*/

//  cout << cameraMatrix << endl;
//  cout << distCoeffs << endl;

  //TODO: why undistort returns homogeneous coordinates instead of pixel coordinates
  vector<Point2f> undistortedMaskPoints;
  undistortPoints(Mat(maskPoints), undistortedMaskPoints, cameraMatrix, distCoeffs);
  //cout << Mat(undistortedMaskPoints) << endl;
  //undistortedMaskPoints = maskPoints;

  Mat cameraMatrix_inv_t = (cameraMatrix.inv()).t();
  vector<Point3f> homogeneousUndistortedMaskPoints;
  convertPointsHomogeneous(Mat(undistortedMaskPoints), homogeneousUndistortedMaskPoints);


//  projectPoints(Mat(homogeneousUndistortedMaskPoints), zeros, zeros, cameraMatrix, distCoeffs, undistortedMaskPoints);

  Mat homogeneousUndistortedMaskPointsDouble;
  Mat(homogeneousUndistortedMaskPoints).reshape(1).convertTo(homogeneousUndistortedMaskPointsDouble, CV_64FC1);

  Mat maskBundles = homogeneousUndistortedMaskPointsDouble;
  //Mat maskBundles = homogeneousUndistortedMaskPointsDouble * cameraMatrix_inv_t;
  CV_Assert(maskBundles.type() == CV_64FC1);


  //interpolatedPointCloud = pointCloud;
//  projectPoints()


  flann::KDTreeIndexParams flannIndexParams;
  //flann::LinearIndexParams flannIndexParams;
  flann::Index projectedPointsIndex(Mat(undistortedProjectedCloud).reshape(1), flannIndexParams);

  for (size_t i = 0; i < undistortedMaskPoints.size(); i++)
  {
    vector<float> query;
    //query.push_back(maskPoints[i].x);
    //query.push_back(maskPoints[i].y);
    query.push_back(undistortedMaskPoints[i].x);
    query.push_back(undistortedMaskPoints[i].y);


    const int knn = 4;
    vector<int> indices(knn);
    vector<float> squaredDists(knn);
    projectedPointsIndex.knnSearch(query, indices, squaredDists, knn, flann::SearchParams());

    Mat dists;
    sqrt(Mat(squaredDists), dists);
    CV_Assert(dists.type() == CV_32FC1);
    double distsSum = sum(dists)[0];


    Point3d bundle;
    bundle.x = maskBundles.at<double>(i, 0);
    bundle.y = maskBundles.at<double>(i, 1);
    bundle.z = maskBundles.at<double>(i, 2);
    double meanDist = 0.0;
    for (int j = 0; j < knn; j++)
    {
      double dist = bundle.ddot(pointCloud[indices[j]]) / norm(bundle);
      double weight = distsSum - dists.at<float>(j);
      meanDist += weight * dist;
    }
    meanDist /= (distsSum * (knn - 1));

    Point3f pt3f = meanDist * bundle;




    interpolatedPointCloud.push_back(pt3f);

/*
    vector<Point3f> tmpPoints3f;
    vector<Point2f> tmpPoints2f;
    tmpPoints3f.push_back(interpolatedPointCloud[interpolatedPointCloud.size() - 1]);
    projectPoints(Mat(tmpPoints3f), zeros, zeros, cameraMatrix, distCoeffs, tmpPoints2f);
    Point pt = tmpPoints2f[0];

    if(pt.x < 0 || pt.y < 0 || pt.x >= mask.cols || pt.y >= mask.rows)
    {
      cout << "Error: " << endl;
      cout << pt << endl;
      cout << tmpPoints2f[0] << endl;
      cout << meanDist << endl;
      cout << distsSum << endl;
      cout << bundle << endl;

      cout << "start" << endl;
      for (int j = 0; j < knn; j++)
      {
        double dist = bundle.ddot(pointCloud[indices[j]]) / norm(bundle);
        cout << dist << endl;
        double weight = distsSum - dists.at<float>(i);
        cout << weight << endl;
        //meanDist += weight * dist;
      }
      cout << "end" <<endl;
      cout << dists << endl;
      cout << Mat(squaredDists) << endl;

      exit(-1);
    }
*/
//    if(i % 10000 == 0)
//      cout << i << "/" << undistortedMaskPoints.size() << endl;
  }



/*


  Point3d pt(0.0, 0.0, 0.0);
  Mat dists;
  sqrt(Mat(squaredDists), dists);
  CV_Assert(dists.type() == CV_32FC1);
  double distsSum = sum(dists)[0];
  for (int i = 0; i < knn; i++)
  {
    double weight = distsSum - dists.at<float>(i);
    Point3d weightedNeighbor = pointCloud[indices[i]] * weight;
    pt += weightedNeighbor;
  }
  pt *= (1.0 / distsSum);

  interpolatedPointCloud.push_back(pt);

*/

//  interpolatedPointCloud = pointCloud;

  cout << interpolatedPointCloud.size() << endl;
  CV_Assert(interpolatedPointCloud.size() == static_cast<size_t>(countNonZero(mask != 0)));

/*
  vector<Point2f> imagePoints;
  imagePoints.clear();
  projectPoints(Mat(interpolatedPointCloud), zeros, zeros, cameraMatrix, distCoeffs, imagePoints);


  Mat draw(mask.size(), CV_8UC1, Scalar(0));
  for (size_t i = 0; i < imagePoints.size(); i++)
  {
    Point pt = imagePoints[i];
    if(pt.x < 0 || pt.y < 0 || pt.x >= mask.cols || pt.y >= mask.rows)
    {
      cout << "Error2: " << endl;
      cout << pt << endl;
//      cout << meanDist << endl;
//      cout << bundle << endl;
      exit(-1);
    }

    circle(draw, imagePoints[i], 0, Scalar(255));
  }

  Mat resizedDraw;
  resize(draw, resizedDraw, Size(), 0.3, 0.3);
  imshow("draw", resizedDraw);
  imwrite("draw.png", draw);
  waitKey();
*/
}

void publishPoints(const std::vector<cv::Point3f>& points, const ros::Publisher &points_pub, int id, cv::Scalar color)
{
#ifdef USE_RVIZ
  visualization_msgs::Marker cammark;
  cammark.header.frame_id = "/high_def_frame";
  cammark.header.stamp = ros::Time();
  cammark.ns = "high_def_frame";
  cammark.id = id;
  cammark.action = visualization_msgs::Marker::ADD;
  cammark.pose.position.x = 0;
  cammark.pose.position.y = 0;
  cammark.pose.position.z = 0;
  cammark.pose.orientation.x = 0.0;
  cammark.pose.orientation.y = 0.0;
  cammark.pose.orientation.z = 0.0;
  cammark.pose.orientation.w = 1.0;
//  cammark.scale.x = 0.1;
//  cammark.scale.y = 0.1;
//  cammark.scale.z = 0.1;

  cammark.scale.x = 1;
  cammark.scale.y = 1;
  cammark.scale.z = 1;

  const int maxVal = 255;
  cammark.color.r = color[2] / maxVal;
  cammark.color.g = color[1] / maxVal;
  cammark.color.b = color[0] / maxVal;
  cammark.color.a = 1.0f;

  cammark.lifetime = ros::Duration();
  cammark.type = visualization_msgs::Marker::POINTS;
  cammark.points.resize(points.size());
  //const float scale = 1000.0f;
  const float scale = 400.0f;

  //TODO: why rviz doesn't publish point without this line?
  sleep(1);
  for (size_t m = 0; m < points.size(); m++)
  {
    cammark.points[m].x = points[m].x * scale;
    cammark.points[m].y = points[m].y * scale;
    cammark.points[m].z = points[m].z * scale;
  }
  points_pub.publish(cammark);
#endif
}

void publishPoints(const std::vector<std::vector<cv::Point3f> >& points, const ros::Publisher &points_pub)
{
  cout << "publising..." << endl;
  const int minVal = 128;
  const int maxVal = 255;
  const int colorDim = 3;
  for (size_t i = 0; i < points.size(); i++)
  {
    cout << "size: " << points[i].size() << endl;
    Scalar color;
    for (int j = 0; j < colorDim; j++)
    {
      color[j] = minVal + rand() % (maxVal - minVal + 1);
    }

    publishPoints(points[i], points_pub, i, color);
  }
}

void publishPoints(const std::vector<cv::Point3f>& points, const cv::Mat &rvec, const cv::Mat &tvec, const ros::Publisher &points_pub, int id, cv::Scalar color, const cv::Mat &extrinsicsRt)
{
  PoseRT pose = PoseRT(extrinsicsRt) * PoseRT(rvec, tvec);
  vector<Point3f> rotatedPointCloud;
  project3dPoints(points, pose.getRvec(), pose.getTvec(), rotatedPointCloud);

  publishPoints(rotatedPointCloud, points_pub, id, color);
}

void pcl2cv(const pcl::PointCloud<pcl::PointXYZ> &pclCloud, std::vector<cv::Point3f> &cvCloud)
{
  cvCloud.resize(pclCloud.size());

  for(size_t i=0; i<pclCloud.size(); i++)
  {
    cvCloud[i] = cv::Point3f(pclCloud.points[i].x, pclCloud.points[i].y, pclCloud.points[i].z);
  }
}

void cv2pcl(const std::vector<cv::Point3f> &cvCloud, pcl::PointCloud<pcl::PointXYZ> &pclCloud)
{
  pclCloud.points.resize(cvCloud.size());
  for(size_t i=0; i<cvCloud.size(); i++)
  {
    cv::Point3f pt = cvCloud[i];
    pclCloud.points[i] = pcl::PointXYZ(pt.x, pt.y, pt.z);
  }
}

void transformPoint(const cv::Mat &Rt, const cv::Point3d &point, cv::Point3d &transformedPoint)
{
  Mat transformedPointMat;
  perspectiveTransform(Mat(vector<Point3d>(1, point)), transformedPointMat, Rt);
  vector<Point3d> transformedPointVec = transformedPointMat;
  transformedPoint = transformedPointVec[0];
}

void readLinesInFile(const string &filename, std::vector<string> &lines)
{
  lines.clear();
  std::ifstream file(filename.c_str());
  if(!file.is_open())
  {
    CV_Error(CV_StsBadArg, "Cannot open file " + filename);
  }

  while(!file.eof())
  {
    string curLine;
    file >> curLine;
    if(curLine.empty())
    {
      break;
    }

    lines.push_back(curLine);
  }
  file.close();
}

bool isPointInside(const cv::Mat &image, cv::Point pt)
{
  return (0 <= pt.x && pt.x < image.cols && 0 <= pt.y && pt.y < image.rows);
}

void mask2contour(const cv::Mat &mask, std::vector<cv::Point2f> &contour)
{
  Mat maskClone = mask.clone();
  vector<vector<Point> > allContours;
  findContours(maskClone, allContours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

  contour.clear();
  for (size_t i = 0; i < allContours.size(); ++i)
  {
    std::copy(allContours[i].begin(), allContours[i].end(), std::back_inserter(contour));
  }
}

void hcat(const Mat &A, const Mat &B, Mat &result)
{
  result = A.t();
  Mat bt = B.t();
  result.push_back(bt);
  result = result.t();
}

void readFiducial(const string &filename, Mat &blackBlobsObject, Mat &whiteBlobsObject, Mat &allBlobsObject)
{
  FileStorage fiducialFS(filename, FileStorage::READ);
  CV_Assert(fiducialFS.isOpened());
  fiducialFS["fiducial"]["templates"][0] >> whiteBlobsObject;
  fiducialFS["fiducial"]["templates"][1] >> blackBlobsObject;
  fiducialFS.release();

//  hcat(blackBlobsObject, whiteBlobsObject, allBlobsObject);
  allBlobsObject = blackBlobsObject.clone();
  allBlobsObject.push_back(whiteBlobsObject);

  CV_Assert(!blackBlobsObject.empty() && !whiteBlobsObject.empty());
}

cv::Mat drawSegmentation(const cv::Mat &image, const cv::Mat &mask)
{
  Mat drawImage = image.clone();
  CV_Assert(drawImage.channels() == 3);

  Mat glassMask = mask.clone();
  vector<vector<Point> > contours;
  findContours(glassMask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
  drawContours(drawImage, contours, -1, Scalar(0, 255, 0), 1);
  return drawImage;
}

vector<Mat> displayEdgels(const std::vector<cv::Mat> &images, const vector<Point3f> &edgels3d,
                          const PoseRT &pose_cam,
                          const std::vector<PinholeCamera> &cameras,
                          const string &title)
{
  vector<Mat> drawImages(images.size());
  for(size_t i=0; i<images.size(); i++)
  {
    if (images[i].channels() == 1)
    {
      cvtColor(images[i], drawImages[i], CV_GRAY2BGR);
    }
    else
    {
      drawImages[i] = images[i].clone();
    }
    PoseRT curPose = cameras[i].extrinsics * pose_cam;
    vector<Point2f> projectedEdgels;
    projectPoints(Mat(edgels3d), curPose.getRvec(), curPose.getTvec(), cameras[i].cameraMatrix, cameras[i].distCoeffs, projectedEdgels);

    for(size_t j=0; j<projectedEdgels.size(); j++)
    {
      //circle(drawImages[i], projectedEdgels[j], 2, Scalar(0, 0, 255), -1);
      circle(drawImages[i], projectedEdgels[j], 0, Scalar(0, 0, 255), -1);
    }

//#ifdef VISUALIZE_POSE_REFINEMENT
    std::stringstream titleStream;
    titleStream << title << " " << i;
    imshow(titleStream.str(), drawImages[i]);
//#endif
  }

  return drawImages;
}

Mat displayEdgels(const cv::Mat &image, const vector<Point3f> &edgels3d, const PoseRT &pose_cam, const PinholeCamera &camera, const string &title)
{
  vector<Mat> images(1, image);
  vector<PinholeCamera> allCameras(1, camera);
  return displayEdgels(images, edgels3d, pose_cam, allCameras, title)[0];
}

void publishTable(const Vec4f &tablePlane, int id, Scalar color, ros::Publisher *pt_pub)
{
  float bound = 0.5;
  float step = 0.01;
  vector<Point3f> points;
  for (float x = -bound; x < bound; x += step)
  {
    for (float y = -bound; y < bound; y += step)
    {
      float z = (-tablePlane[3] - x * tablePlane[0] - y * tablePlane[1]) / tablePlane[2];
      points.push_back(Point3f(x, y, z));
    }

  }

  publishPoints(points, *pt_pub, id, color);
}

void writePointCloud(const string &filename, const std::vector<cv::Point3f> &pointCloud)
{
  std::ofstream fout(filename.c_str());
  fout << format(Mat(pointCloud), "csv");
  fout.close();
}

void readPointCloud(const string &filename, std::vector<cv::Point3f> &pointCloud, std::vector<cv::Point3f> *normals)
{
  pointCloud.clear();
  if(normals != 0)
    normals->clear();
  std::ifstream file(filename.c_str());
  CV_Assert(file.is_open());

  const int extSz = 3;
  string ext = filename.substr(filename.size() - extSz, extSz);
  if(ext == "ply")
  {
    while(!file.eof())
    {
      const int sz = 1024;
      char line[sz];
      file.getline(line, sz);
      int res = strcmp("end_header", line);

      if(res == 0)
        break;
    }
  }

  while(!file.eof())
  {
    Point3f pt;
    file >> pt.x >> pt.y;
    if(file.eof())
      break;
    file >> pt.z;
    pointCloud.push_back(pt);

    if(normals != 0)
    {
      Point3f pt;
      file >> pt.x >> pt.y >> pt.z;
      normals->push_back(pt);
    }
  }

  if(normals != 0)
  {
    CV_Assert(normals->size() == pointCloud.size());
  }
}

void readPointCloud(const std::string &filename, std::vector<cv::Point3f> &pointCloud, std::vector<cv::Point3i> &colors, std::vector<cv::Point3f> &normals)
{
  pointCloud.clear();
  colors.clear();
  normals.clear();

  std::ifstream file(filename.c_str());
  CV_Assert(file.is_open());

  const int extSz = 3;
  string ext = filename.substr(filename.size() - extSz, extSz);
  CV_Assert(ext == "ply");

  while(!file.eof())
  {
    const int sz = 1024;
    char line[sz];
    file.getline(line, sz);
    int res = strcmp("end_header", line);

    if(res == 0)
      break;
  }

  while(!file.eof())
  {
    Point3f pt;
    file >> pt.x >> pt.y;
    if(file.eof())
      break;
    file >> pt.z;
    pointCloud.push_back(pt);

    {
      Point3i pt;
      file >> pt.x >> pt.y >> pt.z;
      colors.push_back(pt);
    }

    {
      Point3f pt;
      file >> pt.x >> pt.y >> pt.z;
      normals.push_back(pt);
    }
  }

  CV_Assert(pointCloud.size() == colors.size());
  CV_Assert(pointCloud.size() == normals.size());
}

void project3dPoints(const vector<Point3f>& points, const Mat& rvec, const Mat& tvec, vector<Point3f>& modif_points)
{
  modif_points.clear();
  modif_points.resize(points.size());
  Mat R(3, 3, CV_64FC1);
  Rodrigues(rvec, R);
  for (size_t i = 0; i < points.size(); i++)
  {
    modif_points[i].x = R.at<double> (0, 0) * points[i].x + R.at<double> (0, 1) * points[i].y + R.at<double> (0, 2)
        * points[i].z + tvec.at<double> (0, 0);
    modif_points[i].y = R.at<double> (1, 0) * points[i].x + R.at<double> (1, 1) * points[i].y + R.at<double> (1, 2)
        * points[i].z + tvec.at<double> (1, 0);
    modif_points[i].z = R.at<double> (2, 0) * points[i].x + R.at<double> (2, 1) * points[i].y + R.at<double> (2, 2)
        * points[i].z + tvec.at<double> (2, 0);
  }
}



