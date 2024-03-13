#include "tracker.hpp"

void tracker::readConfigFile(const std::string& filename, tracker::detectionOptions& options)
{
  cv::FileStorage configFile(filename, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  auto markerDetectionNode {configFile["marker_detection"]};

  if (markerDetectionNode.empty() || !markerDetectionNode.isMap())
    throw Error::INCOMPLETE_INFORMATION;

  // read camera id
  auto camIDNode {markerDetectionNode["camera_id"]};
  read(camIDNode, options.camID, 0);

  // read input file path
  auto inputFileNode {markerDetectionNode["input_source_path"]};
  read(inputFileNode, options.inputFilePath, "");

  // read marker side length
  auto markerSideNode {markerDetectionNode["marker_side_meters"]};
  read(markerSideNode, options.markerSideMeters, 0.1);

  // read aruco dictionary option
  auto dictionaryIDNode {markerDetectionNode["marker_dictionary"]};
  int dictionaryID {};
  cv::read(dictionaryIDNode, dictionaryID, 16);
  options.arucoDictionary = cv::aruco::getPredefinedDictionary(dictionaryID);

  auto cameraCalibrationNode {configFile["camera_calibration"]};

  // read camera calibration parameters
  if (!cameraCalibrationNode.empty() && cameraCalibrationNode.isMap()) {
    // read camera matrix
    auto cameraMatrixNode {cameraCalibrationNode["camera_matrix"]};
    cv::read(cameraMatrixNode, options.camMatrix);

    // read distortion coefficients
    auto distortionCoefficientNode {cameraCalibrationNode["distortion_coefficients"]};
    cv::read(distortionCoefficientNode, options.distCoeffs);
  }

  // TODO: print for debug
  std::cout << "-- Parsed parameters --" << std::endl;
  std::cout << "Camera ID: " << options.camID << std::endl;
  std::cout << "Input path: " << options.inputFilePath << std::endl;
  std::cout << "Camera Matrix: " << options.camMatrix << std::endl;
  std::cout << "Distortion Coefficients: " << options.distCoeffs << std::endl;
  std::cout << "Marker side in meters: " << options.markerSideMeters << std::endl;
  std::cout << "Marker dictionary: " << dictionaryID << std::endl;
  std::cout << " -- " << std::endl;
}

void tracker::trackLineFollower(const tracker::detectionOptions& options)
{
  cv::VideoCapture inputVideo;
  int waitTime;

  if(options.inputFilePath != "none") {
    inputVideo.open(options.inputFilePath);
    std::cout << "Using input source file instead of camera stream" << std::endl;
    waitTime = 0;
  }
  else {
    inputVideo.open(options.camID);
    waitTime = 10;
  }

  bool estimatePose {false};

  cv::Mat zero_cam_matrix {
    cv::Mat::zeros(
      options.camMatrix.rows,
      options.camMatrix.cols,
      options.camMatrix.type()
    )
  };
  cv::Mat cam_matrix_comp {options.camMatrix != zero_cam_matrix};

  cv::Mat zero_dist_coeff {
    cv::Mat::zeros(
      options.distCoeffs.rows,
      options.distCoeffs.cols,
      options.camMatrix.type()
    )
  };
  cv::Mat dist_coeff_comp {options.distCoeffs != zero_dist_coeff};

  if (cv::countNonZero(cam_matrix_comp) && cv::countNonZero(dist_coeff_comp)) {
    std::cout << "Camera matrix and distortion coefficients are not empty" << std::endl;
    estimatePose = true;
  }

  double totalTime = 0;
  int totalIterations = 0;

  // Set coordinate system
  cv::Mat objPoints(4, 1, CV_32FC3);
  objPoints.ptr<cv::Vec3f>(0)[0] = cv::Vec3f(-options.markerSideMeters/2.f, options.markerSideMeters/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[1] = cv::Vec3f(options.markerSideMeters/2.f, options.markerSideMeters/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[2] = cv::Vec3f(options.markerSideMeters/2.f, -options.markerSideMeters/2.f, 0);
  objPoints.ptr<cv::Vec3f>(0)[3] = cv::Vec3f(-options.markerSideMeters/2.f, -options.markerSideMeters/2.f, 0);

  std::cout << "Hit ESC key or Crtl + C to exit if a window opens." << std::endl;

  cv::aruco::ArucoDetector detector(options.arucoDictionary, options.detectorParameters);

  while(inputVideo.grab()) {
    cv::Mat image, imageCopy, resizedImage;
    inputVideo.retrieve(image);

    cv::resize(image, resizedImage, cv::Size(426, 240));

    double tick = (double)cv::getTickCount();

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners, rejected;

    // detect markers and estimate pose
    detector.detectMarkers(resizedImage, corners, ids, rejected);

    size_t  nMarkers = corners.size();
    std::vector<cv::Vec3d> rvecs(nMarkers), tvecs(nMarkers);

    if(estimatePose && !ids.empty()) {
      // Calculate pose for each marker
      for (size_t  i = 0; i < nMarkers; i++) {
        solvePnP(objPoints, corners.at(i), options.camMatrix, options.distCoeffs, rvecs.at(i), tvecs.at(i));
      }
    }

    double currentTime = ((double)cv::getTickCount() - tick) / cv::getTickFrequency();
    totalTime += currentTime;
    totalIterations++;
    if(totalIterations % 30 == 0) {
      std::cout << "Detection Time = " << currentTime * 1000 << " ms "
        << "(Mean = " << 1000 * totalTime / double(totalIterations) << " ms)" << std::endl;
    }

    // draw results
    resizedImage.copyTo(imageCopy);
    if(!ids.empty()) {
      cv::aruco::drawDetectedMarkers(imageCopy, corners, ids);

      if(estimatePose) {
        for(unsigned int i = 0; i < ids.size(); i++)
          cv::drawFrameAxes(imageCopy, options.camMatrix, options.distCoeffs, rvecs[i], tvecs[i], options.markerSideMeters * 1.5f, 2);
      }
    }

    if(options.showRejectedMarkers == true && !rejected.empty())
      cv::aruco::drawDetectedMarkers(imageCopy, rejected, cv::noArray(), cv::Scalar(100, 0, 255));

    cv::imshow("out", imageCopy);
    char key = (char)cv::waitKey(waitTime);
    if(key == 27) break;
  }
}

void tracker::calibrateCamera()
{

}
