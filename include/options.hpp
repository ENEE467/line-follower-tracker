#pragma once

#include <vector>
#include <opencv2/objdetect/aruco_detector.hpp>

namespace options {

struct MarkerDetection {

  int camID;
  int frameWidthPixels;
  int frameHeightPixels;
  int frameRateFPS;
  bool showRejectedMarkers;
  cv::aruco::DetectorParameters detectorParameters;

  MarkerDetection()
  : camID {0},
    frameWidthPixels {640},
    frameHeightPixels {480},
    frameRateFPS {30},
    showRejectedMarkers {false},
    detectorParameters {cv::aruco::DetectorParameters()}
  {}

};

// TODO: Integrate this struct into the program
struct LineFollowerMarker {

  float markerSideMeters;
  int markerID;
  int markerDictionaryID;

  LineFollowerMarker()
  : markerSideMeters {0},
    markerID {0},
    markerDictionaryID {cv::aruco::DICT_ARUCO_MIP_36h12}
  {}

};

struct BoardMarkers {

  float markerSideMeters;
  float markerSeperationMetersX;
  float markerSeperationMetersY;
  std::vector<int> markerIDs;
  int markerDictionaryID;

  BoardMarkers()
  : markerSideMeters {0},
    markerSeperationMetersX {0},
    markerSeperationMetersY {0},
    markerIDs {},
    markerDictionaryID {cv::aruco::DICT_ARUCO_MIP_36h12}
  {}

};

struct LineTrack {

  cv::Point2d point1;
  cv::Point2d point2;
  double length;
  double length_inv;

  LineTrack()
  : point1 {0, 0},
    point2 {0, 0},
    length {0},
    length_inv {0}
  {}

  LineTrack(double x1In, double y1In, double x2In, double y2In)
  : point1 {x1In, y1In},
    point2 {x2In, y2In},
    length {std::hypot(x2In - x1In, y2In - y1In)},
    length_inv {length ? 1 / length : 0}
  {}

  void updateLength()
  {
    length = std::hypot(point2.x - point1.x, point2.y - point1.y);
    length_inv = length ? 1 / length : 0;
  }

  double calculatePerpendicularDistance(const cv::Point2d& positionIn) const
  {
    double y_diff {point2.y - point1.y};
    double x_diff {point2.x - point1.x};
    double c {point2.x * point1.y - point2.y * point1.x};

    return std::abs(y_diff * positionIn.x - x_diff * positionIn.y + c) * length_inv;
  }

};
  float markerSideMeters;
  float squareSideMeters;
  int squaresQuantityX;
  int squaresQuantityY;
  int markerDictionaryID;
};

struct CalibrationOutput {
  CalibrationOutput()
  : cameraMatrix {cv::Mat::zeros(cv::Size(3, 3), CV_32F)},
    distCoeffs {cv::Mat::zeros(cv::Size(5, 1), CV_32F)}
    {}

  cv::Mat cameraMatrix;
  cv::Mat distCoeffs;
};

}
