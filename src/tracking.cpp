#include <iostream>
#include <fstream>
#include <algorithm>

#include <errno.h>

// #include <Eigen/Dense>

#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>
// #include <opencv2/core/eigen.hpp>

#include "tracking.hpp"
#include "errors.hpp"
#include "fileio.hpp"
#include "plotting.hpp"

tracking::BoardDetector::BoardDetector(
  const options::MarkerDetection& detectionOptionsIn,
  const options::BoardMarkers& boardMarkersOptionsIn,
  const options::CameraIntrinsic& calibrationParamsIn
)
: _canEstimatePose {calibrationParamsIn.isNonZero()},
  _boardDetected {false},
  _boardPoseEstimated {false},
  _boardMarkerSide {boardMarkersOptionsIn.markerSideMeters},
  _frameBoard_Camera {cv::Affine3d::Identity()},
  _frameLineFollower_Board {cv::Affine3d::Identity()},
  // _eulerAnglesLineFollower_Board {0.0, 0.0, 0.0},
  _positionXYLineFollower_Board {0.0, 0.0},
  _camMatrix {calibrationParamsIn.cameraMatrix},
  _distortionCoeffs {calibrationParamsIn.distortionCoefficients},

  _lineFollowerBoard {
    getBoardMarkersPoints(boardMarkersOptionsIn),
    cv::aruco::getPredefinedDictionary(boardMarkersOptionsIn.markerDictionaryID),
    boardMarkersOptionsIn.markerIDs},

  _boardDetector {
    cv::aruco::getPredefinedDictionary(boardMarkersOptionsIn.markerDictionaryID),
    detectionOptionsIn.detectorParameters}
{}

std::vector<std::vector<cv::Point3f>> tracking::BoardDetector::getBoardMarkersPoints(
  const options::BoardMarkers& boardMarkersOptions)
{
  std::vector<cv::Point3f> markerObjPoints {
    cv::Point3f(
      -boardMarkersOptions.markerSideMeters/2.f,
      boardMarkersOptions.markerSideMeters/2.f,
      0),

    cv::Point3f(
      boardMarkersOptions.markerSideMeters/2.f,
      boardMarkersOptions.markerSideMeters/2.f,
      0),

    cv::Point3f(
      boardMarkersOptions.markerSideMeters/2.f,
      -boardMarkersOptions.markerSideMeters/2.f,
      0),

    cv::Point3f(
      -boardMarkersOptions.markerSideMeters/2.f,
      -boardMarkersOptions.markerSideMeters/2.f,
      0)
  };

  auto translateMarkerObjpoints =
    [markerObjPoints] (float xDisplacement, float yDisplacement)
  {
    std::vector<cv::Point3f> translatedPoints {};

    for (const auto& markerObjPoint: markerObjPoints) {
      translatedPoints.push_back(markerObjPoint + cv::Point3f(xDisplacement, yDisplacement, 0));
    }

    return translatedPoints;
  };

  return {
    translateMarkerObjpoints(0, boardMarkersOptions.markerSeperationMetersY),

    translateMarkerObjpoints(
      boardMarkersOptions.markerSeperationMetersX,
      boardMarkersOptions.markerSeperationMetersY),

    translateMarkerObjpoints(boardMarkersOptions.markerSeperationMetersX, 0),
    translateMarkerObjpoints(0, 0)
  };
}

bool tracking::BoardDetector::hasEnoughBoardIDs()
{
  int foundMarkersCount {0};

  for (const auto& boardMarkerID: _lineFollowerBoard.getIds()) {
    auto foundMarkerID {
      std::find(_detectedMarkerIDs.begin(), _detectedMarkerIDs.end(), boardMarkerID)};

    if (foundMarkerID != _detectedMarkerIDs.end())
      foundMarkersCount++;
  }

  if (foundMarkersCount < 3)
    return false;

  return true;
}

bool tracking::BoardDetector::detectBoard(const cv::Mat& frame)
{
  reset();

  _boardDetector.detectMarkers(
    frame, _detectedMarkerCorners, _detectedMarkerIDs, _rejectedMarkerCorners);

  _boardDetected = hasEnoughBoardIDs();

  return _boardDetected;
}

void tracking::BoardDetector::visualize(cv::Mat& frame)
{
  cv::aruco::drawDetectedMarkers(frame, _detectedMarkerCorners, _detectedMarkerIDs);

  if (!_boardPoseEstimated)
    return;

  cv::drawFrameAxes(frame, _camMatrix, _distortionCoeffs, _frameBoard_Camera.rvec(),
    _frameBoard_Camera.translation(), _boardMarkerSide * 1.5f, 2);
}

bool tracking::BoardDetector::estimateFrameBoard_Camera()
{
  if (!_boardDetected || !_canEstimatePose) {
    _boardPoseEstimated = false;
    return _boardPoseEstimated;
  }

  _lineFollowerBoard.matchImagePoints(
    _detectedMarkerCorners, _detectedMarkerIDs, _boardObjPoints, _boardImgPoints);

  cv::Vec3d rVecBoard_Camera;
  cv::Vec3d tVecBoard_Camera;

  cv::solvePnP(
    _boardObjPoints, _boardImgPoints, _camMatrix, _distortionCoeffs, rVecBoard_Camera,
    tVecBoard_Camera);

  _frameBoard_Camera.translation(tVecBoard_Camera);
  _frameBoard_Camera.rotation(rVecBoard_Camera);

  _boardPoseEstimated = true;

  return _boardPoseEstimated;
}

bool tracking::BoardDetector::estimateFrameLineFollower_Board(
  const cv::Affine3d& frameLineFollower_Camera)
{
  if (!_boardPoseEstimated)
    return false;

  // _frameLineFollower_Board = _frameBoard_Camera * frameLineFollower_Camera.inv();
  _frameLineFollower_Board = _frameBoard_Camera.inv() * frameLineFollower_Camera;

  auto _positionXYZ {_frameLineFollower_Board.translation()};

  _positionXYLineFollower_Board.x = _positionXYZ[0];
  _positionXYLineFollower_Board.y = _positionXYZ[1];

  // Eigen::Matrix3d rotationMatrix;
  // cv::cv2eigen(_frameLineFollower_Board.rotation(), rotationMatrix);

  // Eigen::Vector3d eulerAngles {rotationMatrix.eulerAngles(0, 1, 2)};
  // _eulerAnglesLineFollower_Board = {eulerAngles(0), eulerAngles(1), eulerAngles(2)};

  return true;
}

tracking::LineFollowerDetector::LineFollowerDetector(
  const options::MarkerDetection& detectionOptions,
  const options::LineFollowerMarker& lineFollowerOptions,
  const options::CameraIntrinsic& calibrationParamsIn
)
: _canEstimatePose {calibrationParamsIn.isNonZero()},
  _lineFollowerDetected {false},
  _lineFollowerPoseEstimated {false},
  _markerID {lineFollowerOptions.markerID},
  _markerSide {lineFollowerOptions.markerSideMeters},
  _iteratorToLineFollowerMarkerCorners {_detectedMarkersCorners.begin()},
  // _lineFollowerTVec {0.0, 0.0, 0.0},
  // _lineFollowerRVec {0.0, 0.0, 0.0},
  _frameLineFollower_Camera {cv::Affine3d::Identity()},
  _camMatrix {calibrationParamsIn.cameraMatrix},
  _distortionCoeffs {calibrationParamsIn.distortionCoefficients},

  _lineFollowerMarkerDetector {
    cv::aruco::getPredefinedDictionary(lineFollowerOptions.markerDictionaryID),
    detectionOptions.detectorParameters},

  _markerObjPoints {
    cv::Vec3f(-_markerSide / 2.f, _markerSide / 2.f, 0),
    cv::Vec3f(_markerSide / 2.f, _markerSide / 2.f, 0),
    cv::Vec3f(_markerSide / 2.f, -_markerSide / 2.f, 0),
    cv::Vec3f(-_markerSide / 2.f, -_markerSide / 2.f, 0)
  }
{}

bool tracking::LineFollowerDetector::detectLineFollower(const cv::Mat& frame)
{
  reset();

  _lineFollowerMarkerDetector.detectMarkers(
    frame, _detectedMarkersCorners, _detectedMarkerIDs, _rejectedMarkersCorners);

  _lineFollowerDetected = hasCorrectID();

  // std::cout << "Detected IDs: ";
  // for (const auto& markerID: _detectedMarkerIDs)
  //   std::cout << markerID << " ";
  // std::cout << '\n';

  // std::cout << "Line follower " << _markerID << " detected: " << _lineFollowerDetected << '\n';

  return _lineFollowerDetected;
}

bool tracking::LineFollowerDetector::estimateFrameLineFollower_Camera()
{
  if (!_lineFollowerDetected || !_canEstimatePose) {
    _lineFollowerPoseEstimated = false;

    return _lineFollowerPoseEstimated;
  }

  // std::cout << "Marker corners: ";
  // for (const auto& corner: *_detectedMarkerCornersIterator)
  //   std::cout << " " << corner.x << ", " << corner.y << " ";
  // std::cout << '\n';

  cv::Vec3d rVecLineFollower_Camera;
  cv::Vec3d tVecLineFollower_Camera;

  cv::solvePnP(
    _markerObjPoints, *_iteratorToLineFollowerMarkerCorners, _camMatrix,
    _distortionCoeffs, rVecLineFollower_Camera, tVecLineFollower_Camera);

  _frameLineFollower_Camera.translation(tVecLineFollower_Camera);
  _frameLineFollower_Camera.rotation(rVecLineFollower_Camera);

  _lineFollowerPoseEstimated = true;

  return _lineFollowerPoseEstimated;
}

void tracking::LineFollowerDetector::visualize(cv::Mat& frame)
{
  if (!_lineFollowerDetected)
    return;

  std::vector<std::vector<cv::Point2f>> lineFollowerMarkerCorners {
    *_iteratorToLineFollowerMarkerCorners};

  cv::aruco::drawDetectedMarkers(frame, lineFollowerMarkerCorners, _detectedMarkerIDs);

  if (!_lineFollowerPoseEstimated)
    return;

  cv::drawFrameAxes(
    frame, _camMatrix, _distortionCoeffs, _frameLineFollower_Camera.rvec(),
    _frameLineFollower_Camera.translation(), _markerSide * 1.5f, 2);
}

bool tracking::LineFollowerDetector::hasCorrectID()
{
  auto foundID {std::find(_detectedMarkerIDs.begin(), _detectedMarkerIDs.end(), _markerID)};

  if (foundID == _detectedMarkerIDs.end()) {
    _lineFollowerDetected = false;
    return _lineFollowerDetected;
  }

  auto index = foundID - _detectedMarkerIDs.begin();
  // std::cout << "Index: " << index << '\n';
  _iteratorToLineFollowerMarkerCorners = _detectedMarkersCorners.begin() + index;

  _lineFollowerDetected = true;
  return _lineFollowerDetected;
}

double tracking::calculateTrackingError(
  const cv::Point2d& positionIn,
  const options::Track& trackOptionsIn)
{
  double trackingError {0.0};

  switch (trackOptionsIn.selection) {

  case options::TrackSelection::LINE:
    if (trackOptionsIn.lineTrack.length == 0)
      break;

    trackingError = trackOptionsIn.lineTrack.calculatePerpendicularDistance(positionIn);
    break;

  case options::TrackSelection::ROUND:
    // TODO: Call the perpendicular distance method here
    break;

  default:
    throw Error::INVALID_TRACK_OPTION;
    break;

  }

  return trackingError;
}

void tracking::trackLineFollower(
  const options::Tracking& optionsIn,
  const fileio::OutputPath& outputPathIn)
{
  bool hasOutputDir {!outputPathIn.directoryPath.str().empty()};
  fileio::CSVFile positionsOutput {};
  fileio::CSVFile errorsOutput {};

  if (hasOutputDir) {
    positionsOutput.setOutputPath(outputPathIn, "positions");
    errorsOutput.setOutputPath(outputPathIn, "errors");

    std::cout << "Tracking output will be saved to: " << outputPathIn.directoryPath.str() << '\n';
  }
  else {
    std::cout << "No output directory is given, tracking output will not be saved." << '\n';
  }

  plotting::Plotter plotter {optionsIn.boardMarkers};

  std::cout << "Hit ESC key or Crtl + C to exit if a window opens." << '\n';

  cv::VideoCapture inputVideo;
  int waitTime {10};
  inputVideo.open(optionsIn.detection.camID);
  inputVideo.set(cv::CAP_PROP_FRAME_WIDTH, optionsIn.detection.frameWidthPixels);
  inputVideo.set(cv::CAP_PROP_FRAME_HEIGHT, optionsIn.detection.frameHeightPixels);
  inputVideo.set(cv::CAP_PROP_FPS, optionsIn.detection.frameRateFPS);

  tracking::BoardDetector lineFollowerBoardDetector {
    optionsIn.detection, optionsIn.boardMarkers, optionsIn.calibrationParams};

  tracking::LineFollowerDetector lineFollowerDetector {
    optionsIn.detection, optionsIn.lineFollowerMarker, optionsIn.calibrationParams};

  int totalIterations {};
  double totalTime {};

  cv::Mat frame;
  while(inputVideo.grab()) {
    inputVideo.retrieve(frame);

    double tick {static_cast<double>(cv::getTickCount())};

    lineFollowerBoardDetector.detectBoard(frame);
    lineFollowerBoardDetector.estimateFrameBoard_Camera();

    lineFollowerDetector.detectLineFollower(frame);

    if (lineFollowerDetector.estimateFrameLineFollower_Camera()) {
      lineFollowerBoardDetector.estimateFrameLineFollower_Board(
        lineFollowerDetector.getFrameLineFollower_Camera());
    }

    double trackingError {
      calculateTrackingError(
        lineFollowerBoardDetector.getPositionXYLineFollower_Board(), optionsIn.track)};

    lineFollowerBoardDetector.visualize(frame);
    lineFollowerDetector.visualize(frame);

    cv::imshow("Line Follower tracking", frame);

    char key {static_cast<char>(cv::waitKey(waitTime))};
    if(key == 27)
      break;

    double currentTime {
      1000 * (static_cast<double>(cv::getTickCount()) - tick) / cv::getTickFrequency()};

    totalTime += currentTime;
    totalIterations++;

    // Output specific stuff only from here.
    if (!hasOutputDir)
      continue;

    if (totalIterations % 10 != 0)
      continue;

    positionsOutput.writePosition(
      lineFollowerBoardDetector.getPositionXYLineFollower_Board(), totalTime);

    errorsOutput.writeError(trackingError, totalTime);

    plotter.savePosition(lineFollowerBoardDetector.getPositionXYLineFollower_Board());
    plotter.saveError(trackingError, totalTime);
  }

  if (hasOutputDir) {
    plotter.savePlots(outputPathIn);
  }
}
