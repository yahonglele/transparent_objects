/*
 * transparentDetector.hpp
 *
 *  Created on: Dec 13, 2011
 *      Author: ilysenkov
 */

#ifndef TRANSPARENTDETECTOR_HPP_
#define TRANSPARENTDETECTOR_HPP_

#include "edges_pose_refiner/poseEstimator.hpp"
#include "edges_pose_refiner/glassSegmentator.hpp"

namespace transpod
{
  struct PlaneSegmentationParams
  {
    /** \brief leaf size of the voxel grid used to downsample a test point cloud */
    float downLeafSize;

    /** \brief number of nearest neighbors used to estimate normals of a test point cloud */
    int kSearch;

    /** \brief maximum distance for a point to be considered as an inlier when finding a plane */
    float distanceThreshold;

    /** \brief spatial cluster tolerance when clustering points
     *
     * Points belonging to a plane are clustered to find a table as the largest cluster
     */
    float clusterTolerance;

    PlaneSegmentationParams()
    {
      downLeafSize = 0.002f;
      kSearch = 10;
      distanceThreshold = 0.02f;
      clusterTolerance = 0.05f;
    }
  };

  struct DetectorParams
  {
    /** \brief parameters to segment a plane in a test scene */
    PlaneSegmentationParams planeSegmentationParams;

    /** \brief parameters to segment glass */
    GlassSegmentatorParams glassSegmentationParams;

    DetectorParams()
    {
      glassSegmentationParams = GlassSegmentatorParams();
    }
  };

  class Detector
  {
  public:
    struct DebugInfo;

    /** \brief The constructor
     *
     * \param pinholeCamera test camera
     * \param params parameters of the detector
     */
    Detector(const PinholeCamera &camera = PinholeCamera(), const DetectorParams &params = DetectorParams());

    /** \brief Initializes the detector
     *
     * \param pinholeCamera test camera
     * \param params parameters of the detector
     */
    void initialize(const PinholeCamera &camera, const DetectorParams &params = DetectorParams());

    /** \brief Add a new train object to the detector which will be searched at the test stage
     *
     * \param objectName name of the train object
     * \param points point cloud of the train object
     * \param isModelUpsideDown true if the object is upside down in the passed point cloud
     * \param centralize true if the object is needed to be transformed to the origin of the coordinate system
     */
    void addTrainObject(const std::string &objectName, const std::vector<cv::Point3f> &points,
                        bool isModelUpsideDown, bool centralize);

    /** \brief Add a new train object to the detector which will be searched at the test stage
     *
     * \param objectName name of the train object
     * \param edgeModel edge model of the train object
     */
    void addTrainObject(const std::string &objectName, const EdgeModel &edgeModel);

    /** \brief Add a new train object to the detector which will be searched at the test stage
     *
     * \param objectName name of the train object
     * \param poseEstimator trained pose estimator for the train object
     */
    void addTrainObject(const std::string &objectName, const PoseEstimator &poseEstimator);

    /** \brief Detect objects on a test scene
     *
     * \param bgrImage BGR image of the test scene
     * \param depth depth image of the test scene
     * \param registrationMask mask of invalid depth when registering BGR and depth images of Kinect
     * \param sceneCloud point cloud of the test scene
     * \param poses_cam detected poses of objects, one for each train object
     * \param posesQualities qualities of the corresponding detected poses (less is better)
     * \param objectNames names of the corresponding detected objects
     * \param debugInfo optional information for debugging
     */
    void detect(const cv::Mat &bgrImage, const cv::Mat &depth, const cv::Mat &registrationMask, const pcl::PointCloud<pcl::PointXYZ> &sceneCloud,
                std::vector<PoseRT> &poses_cam, std::vector<float> &posesQualities, std::vector<std::string> &objectNames,
                DebugInfo *debugInfo = 0) const;

    /** \brief Visualize detected poses
     *
     * \param poses detected poses to be visualized
     * \param objectNamens names of corresponding detected objects
     * \param image image to draw visualized poses
     */
    void visualize(const std::vector<PoseRT> &poses, const std::vector<std::string> &objectNames,
                   cv::Mat &image) const;

    /** \brief Visualize detected poses in 3D
     *
     * \param poses detected poses to be visualized
     * \param objectNamens names of corresponding detected objects
     * \param cloud point cloud to add detected poses
     */
    void visualize(const std::vector<PoseRT> &poses, const std::vector<std::string> &objectNames,
                   pcl::PointCloud<pcl::PointXYZ> &cloud) const;
  private:
    bool tmpComputeTableOrientation(const PinholeCamera &camera, const cv::Mat &centralBgrImage,
                                    cv::Vec4f &tablePlane) const;

    DetectorParams params;
    PinholeCamera srcCamera;
    std::map<std::string, PoseEstimator> poseEstimators;
    cv::Size validTestImageSize;
  };

  struct Detector::DebugInfo
  {
      cv::Mat glassMask;
      std::vector<cv::Mat> initialSilhouettes;
  };
}

#endif /* TRANSPARENTDETECTOR_HPP_ */
