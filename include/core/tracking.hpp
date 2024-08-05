/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#pragma once

#include <chrono>

#include <opencv2/highgui.hpp>
#include <opencv2/core/affine.hpp>

#include "options.hpp"
#include "fileio.hpp"
#include "plotting.hpp"

namespace tracking {

class BoardDetector {

public:
  BoardDetector(
    const options::MarkerDetection& detectionOptionsIn,
    const options::BoardMarkers& boardMarkersOptionsIn,
    const options::Track& trackOptionsIn,
    const options::CameraIntrinsic& calibrationParamsIn);

  bool detectBoard(const cv::Mat& frame);
  void visualize(cv::Mat& frame);

  bool estimateFrameBoard_Camera();
  bool estimateFrameLineFollower_Board(const cv::Affine3d& frameLineFollower_Camera);

  const cv::Affine3d& getFrameLineFollower_Board() const {return _frameLineFollower_Board;}
  const cv::Point2d& getPositionXYLineFollower_Board() const {return _positionXYLineFollower_Board;}
  const cv::Affine3d& getFrameBoard_Camera() const {return _frameBoard_Camera;}

private:
  void reset()
  {
    _detectedMarkerCorners.clear();
    _detectedMarkerIDs.clear();
    _rejectedMarkerCorners.clear();
    // _trackObjPoints_Camera.clear();
    _trackImgPoints.clear();
    _boardDetected = false;
    _boardPoseEstimated = false;
    _frameBoard_Camera = cv::Affine3d::Identity();
    _frameLineFollower_Board = cv::Affine3d::Identity();
    _positionXYLineFollower_Board = {0.0, 0.0};
  }

  std::vector<std::vector<cv::Point3f>> getBoardMarkersPoints(
    const options::BoardMarkers& boardMarkersOptions);

  void setTrackObjBoardPoints();
  bool hasEnoughBoardIDs();
  void drawTrack(cv::Mat& frame);

  bool _canEstimatePose;
  bool _boardDetected;
  bool _boardPoseEstimated;
  float _boardMarkerSide;


  /**
   * Variable name convention used here:
   *
   * _frame[of the object]_[relative to this frame]
   * _position[axes][of the object]_[relative to this frame]
  **/
  cv::Affine3d _frameBoard_Camera;

  cv::Affine3d _frameLineFollower_Board;
  cv::Point2d _positionXYLineFollower_Board;

  cv::Mat _camMatrix;
  cv::Mat _distortionCoeffs;

  cv::aruco::Board _trackBoard;
  cv::aruco::ArucoDetector _boardDetector;
  options::Track _trackOptions;

  std::vector<cv::Point3d> _trackObjPoints_Board;
  // std::vector<cv::Point3d> _trackObjPoints_Camera;
  std::vector<cv::Point2d> _trackImgPoints;
  std::vector<int> _detectedMarkerIDs;
  std::vector<std::vector<cv::Point2f>> _detectedMarkerCorners;
  std::vector<std::vector<cv::Point2f>> _rejectedMarkerCorners;
  cv::Mat _boardObjPoints;
  cv::Mat _boardImgPoints;

};

class LineFollowerDetector {

public:
  LineFollowerDetector(
    const options::MarkerDetection& detectionOptions,
    const options::LineFollowerMarker& lineFollowerOptions,
    const options::CameraIntrinsic& calibtrationParamsIn);

  bool detectLineFollower(const cv::Mat& frame);
  bool estimateFrameLineFollower_Camera();
  void visualize(cv::Mat& frame);

  const cv::Affine3d& getFrameLineFollower_Camera() const
  {
    return _frameLineFollower_Camera;
  }

private:
  void reset()
  {
    _iteratorToLineFollowerMarkerCorners = _detectedMarkersCorners.begin();
    _detectedMarkersCorners.clear();
    _detectedMarkerIDs.clear();
    _rejectedMarkersCorners.clear();
    _lineFollowerDetected = false;
    _lineFollowerPoseEstimated = false;
    _frameLineFollower_Camera = cv::Affine3d::Identity();
  }

  bool hasCorrectID();

  bool _canEstimatePose;
  bool _lineFollowerDetected;
  bool _lineFollowerPoseEstimated;
  int _markerID;
  float _markerSide;
  std::vector<std::vector<cv::Point2f>>::iterator _iteratorToLineFollowerMarkerCorners;

  cv::Affine3d _frameLineFollower_Camera;

  cv::Mat _camMatrix;
  cv::Mat _distortionCoeffs;

  cv::aruco::ArucoDetector _lineFollowerMarkerDetector;

  std::vector<cv::Vec3f> _markerObjPoints;
  std::vector<int> _detectedMarkerIDs;
  std::vector<std::vector<cv::Point2f>> _detectedMarkersCorners;
  std::vector<std::vector<cv::Point2f>> _rejectedMarkersCorners;

};

struct Output {

public:
  std::unique_ptr<fileio::CSVFile> positionsOutput;
  std::unique_ptr<fileio::CSVFile> errorsOutput;
  std::unique_ptr<plotting::Plotter> plotsOutput;

  void open(
    const options::Tracking& trackingOptionsIn,
    const std::string& outputParentDirectoryPathIn,
    const std::string& outputNameIn);

  const bool isOpen() const {return _isOpen;}

  void close();

private:
  bool _isOpen {false};
  std::string _outputDirectoryPath;
  std::string _outputName;

};

class Tracker {

public:
  Tracker(
    const std::string& outputParentDirectoryPathIn = "",
    const std::string& outputName = "",
    const options::Tracking& optionsIn = options::Tracking());

  void start();
  void run(unsigned int& imageTextureOut);
  void startTracking();
  const bool isTracking() const {return _trackingOutput.isOpen();}
  void stopTracking();

private:
  Output _trackingOutput;
  options::Tracking _options;
  std::string _outputParentDirectory;
  std::string _outputName;

  std::chrono::_V2::system_clock::time_point _startTime;
  std::chrono::_V2::system_clock::time_point _currentTime;
  std::chrono::duration<double, std::ratio<1L, 1L>> _elapsedTime;

  cv::VideoCapture _inputVideo;
  BoardDetector _trackBoardDetector;
  LineFollowerDetector _lineFollowerDetector;
  cv::Mat _frame;

};

double calculateTrackingError(const cv::Point2d& positionIn, const options::Track& trackOptionsIn);

void trackLineFollower(
  const options::Tracking& optionsIn,
  const std::string& outputParentDirectoryPathIn,
  const std::string& outputName);


}
