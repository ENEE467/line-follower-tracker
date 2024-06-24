#include <fstream>
#include <ctime>
#include <iomanip>
#include <opencv2/core.hpp>

#include "errors.hpp"
#include "fileio.hpp"

void fileio::readConfigFile(const std::string& filenameIn, options::MarkerDetection& optionsOut)
{
  cv::FileStorage configFile(filenameIn, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  auto markerDetectionNode {configFile["marker_detection"]};

  if (markerDetectionNode.empty() || !markerDetectionNode.isMap())
    throw Error::MISSING_MARKER_DETECTION_CONFIG;

  // TODO: Make it more robust by adding read checks for individual parameters

  auto camIDNode {markerDetectionNode["camera_id"]};
  auto frameWidth {markerDetectionNode["frame_width_pixels"]};
  auto frameHeight {markerDetectionNode["frame_height_pixels"]};
  auto frameRate {markerDetectionNode["frame_rate_fps"]};
  auto rejectedMarkersNode {configFile["rejected_markers"]};

  cv::read(camIDNode, optionsOut.camID, 0);
  cv::read(frameWidth, optionsOut.frameWidthPixels, 640);
  cv::read(frameHeight, optionsOut.frameHeightPixels, 480);
  cv::read(frameRate, optionsOut.frameRateFPS, 30);
  cv::read(rejectedMarkersNode, optionsOut.showRejectedMarkers, false);
}

void fileio::readConfigFile(const std::string& filenameIn, options::LineFollowerMarker& optionsOut)
{
  cv::FileStorage configFile(filenameIn, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  auto lineFollowerMarkerNode {configFile["line_follower_marker"]};

  if (lineFollowerMarkerNode.empty() || !lineFollowerMarkerNode.isMap())
    throw Error::MISSING_LINE_FOLLOWER_MARKER_CONFIG;

  // TODO: Make it more robust by adding read checks for individual parameters

  auto markerSideMetersNode {lineFollowerMarkerNode["marker_side_meters"]};
  auto markerIDNode {lineFollowerMarkerNode["marker_id"]};
  auto markerDictionaryID {lineFollowerMarkerNode["marker_dictionary_id"]};

  cv::read(markerSideMetersNode, optionsOut.markerSideMeters, 0.0);
  cv::read(markerIDNode, optionsOut.markerID, 4);
  cv::read(markerDictionaryID, optionsOut.markerDictionaryID, cv::aruco::DICT_ARUCO_MIP_36h12);
}

void fileio::readConfigFile(const std::string& filenameIn, options::BoardMarkers& optionsOut)
{
  cv::FileStorage configFile(filenameIn, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  // auto markerDetectionNode {configFile["marker_detection"]};
  auto boardParametersNode {configFile["board_markers"]};

  if (boardParametersNode.empty() || !boardParametersNode.isMap())
    throw Error::MISSING_BOARD_MARKERS_CONFIG;

  // TODO: Make it more robust by adding read checks for individual parameters

  auto markerSideMetersNode {boardParametersNode["marker_side_meters"]};
  auto markerSeperationMetersXNode {boardParametersNode["marker_seperation_meters_x"]};
  auto markerSeperationMetersYNode {boardParametersNode["marker_seperation_meters_y"]};
  auto markerIDsNode {boardParametersNode["marker_ids"]};
  auto dictionaryIDNode {boardParametersNode["marker_dictionary_id"]};

  cv::read(markerSideMetersNode, optionsOut.markerSideMeters, 0.F);
  cv::read(markerSeperationMetersXNode, optionsOut.markerSeperationMetersX, 0.F);
  cv::read(markerSeperationMetersYNode, optionsOut.markerSeperationMetersY, 0.F);
  cv::read(markerIDsNode, optionsOut.markerIDs, {});
  cv::read(dictionaryIDNode, optionsOut.markerDictionaryID, cv::aruco::DICT_ARUCO_MIP_36h12);
}

void fileio::readConfigFile(const std::string& filenameIn, options::Track& optionsOut)
{
  cv::FileStorage configFile(filenameIn, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  options::TrackSelection trackSelection {static_cast<int>(configFile["track_selection"])};
  auto lineTrackNode {configFile["line_track"]};
  auto roundTrackNode {configFile["round_track"]};

  // TODO: Make it more robust by adding read checks for individual parameters

  // Configuration error checking
  switch (trackSelection) {

  case options::TrackSelection::LINE:
    if (lineTrackNode.empty() || !lineTrackNode.isMap())
      throw Error::MISSING_LINE_TRACK_CONFIG;
    break;

  case options::TrackSelection::ROUND:
    if (roundTrackNode.empty() || !roundTrackNode.isMap())
      throw Error::MISSING_ROUND_TRACK_CONFIG;
    break;

  default:
    throw::Error::INVALID_TRACK_OPTION;

  }

  optionsOut.selection = trackSelection;

  // Read config file
  switch (optionsOut.selection) {

  case options::TrackSelection::LINE:
    cv::read(lineTrackNode["point1"]["x_meters"], optionsOut.lineTrack.point1.x, 0.0);
    cv::read(lineTrackNode["point1"]["y_meters"], optionsOut.lineTrack.point1.y, 0.0);
    cv::read(lineTrackNode["point2"]["x_meters"], optionsOut.lineTrack.point2.x, 0.0);
    cv::read(lineTrackNode["point2"]["y_meters"], optionsOut.lineTrack.point2.y, 0.0);

    optionsOut.lineTrack.updateLength(); // Don't forget to update the length after reading the
                                         // new points.
    break;

  case options::TrackSelection::ROUND:
    cv::read(roundTrackNode["center"]["x_meters"], optionsOut.roundTrack.center.x, 0.0);
    cv::read(roundTrackNode["center"]["y_meters"], optionsOut.roundTrack.center.y, 0.0);
    cv::read(roundTrackNode["major_axis_meters"], optionsOut.roundTrack.majorAxis, 0.0);
    cv::read(roundTrackNode["minor_axis_meters"], optionsOut.roundTrack.minorAxis, 0.0);
    break;

  default:
    throw Error::INVALID_TRACK_OPTION;

  }
}
{
  cv::FileStorage configFile(filename, cv::FileStorage::READ);

  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  auto markerDetectionNode {configFile["marker_detection"]};
  auto cameraCalibrationNode {configFile["camera_calibration"]};

  if (markerDetectionNode.empty() || !markerDetectionNode.isMap())
    throw Error::INCOMPLETE_INFORMATION;

  if (cameraCalibrationNode.empty() || !cameraCalibrationNode.isMap())
    throw Error::INCOMPLETE_INFORMATION;

  auto camIDNode {markerDetectionNode["camera_id"]};
  auto inputFileNode {markerDetectionNode["input_source_path"]};
  auto markerSideNode {cameraCalibrationNode["marker_side_meters"]};
  auto dictionaryIDNode {cameraCalibrationNode["marker_dictionary_id"]};
  auto squareSideNode {cameraCalibrationNode["square_side_meters"]};
  auto squaresQuantityXNode {cameraCalibrationNode["squares_quantity_x"]};
  auto squaresQuantityYNode {cameraCalibrationNode["squares_quantity_y"]};

  cv::read(camIDNode, options.camID, 0);
  cv::read(inputFileNode, options.inputFilePath, "");
  cv::read(markerSideNode, options.markerSideMeters, 0.F);
  cv::read(squareSideNode, options.squareSideMeters, 0.F);
  cv::read(squaresQuantityXNode, options.squaresQuantityX, 0);
  cv::read(squaresQuantityYNode, options.squaresQuantityY, 0);
  cv::read(dictionaryIDNode, options.markerDictionaryID, cv::aruco::DICT_ARUCO_ORIGINAL);
}

std::stringstream fileio::createTimeStampedFileName(
  const std::string& filedir,
  const std::string& prefix,
  const std::string& extension)
{
  char filedirEndChar {};

  if (filedir.back() != '/')
    filedirEndChar = '/';

  std::stringstream filenameStream;
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  filenameStream << filedir << filedirEndChar << prefix << "-"
                 << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S")
                 << "." << extension;

  return filenameStream;
}

void fileio::writeConfigFile(
  const std::string& filename,
  const options::MarkerDetection& detection_options,
  const options::LineFollowerMarker& line_follower_options,
  const options::BoardMarkers& board_options,
  const options::Calibration& calibration_options,
  const options::CalibrationOutput& calibration_output)
{
  cv::FileStorage configFile(filename, cv::FileStorage::WRITE);
  if (!configFile.isOpened())
    throw Error::CANNOT_OPEN_FILE;

  configFile.startWriteStruct("marker_detection", cv::FileNode::MAP);
    configFile << "input_source_path" << detection_options.inputFilePath;
    configFile << "camera_id" << detection_options.camID;
    configFile << "show_rejected_markers" << detection_options.showRejectedMarkers;
  configFile.endWriteStruct();

  configFile.startWriteStruct("line_follower_marker", cv::FileNode::MAP);
    configFile << "marker_side_meters" << line_follower_options.markerSideMeters;
    configFile << "marker_id" << line_follower_options.markerID;
    configFile << "marker_dictionary_id" << line_follower_options.markerDictionaryID;
  configFile.endWriteStruct();

  configFile.startWriteStruct("board_markers", cv::FileNode::MAP);
    configFile << "marker_side_meters" << board_options.markerSideMeters;
    configFile << "marker_seperation_meters_x" << board_options.markerSeperationMetersX;
    configFile << "marker_seperation_meters_y" << board_options.markerSeperationMetersY;
    configFile << "marker_ids" << board_options.markerIDs;
    configFile << "marker_dictionary_id" << board_options.markerDictionaryID;
  configFile.endWriteStruct();

  configFile.startWriteStruct("camera_calibration", cv::FileNode::MAP);
    configFile << "marker_side_meters" << calibration_options.markerSideMeters;
    configFile << "square_side_meters" << calibration_options.squareSideMeters;
    configFile << "squares_quantity_x" << calibration_options.squaresQuantityX;
    configFile << "squares_quantity_y" << calibration_options.squaresQuantityY;
    configFile << "marker_dictionary_id" << calibration_options.markerDictionaryID;
    configFile << "camera_matrix" << calibration_output.cameraMatrix;
    configFile << "distortion_coefficients" << calibration_output.distCoeffs;
  configFile.endWriteStruct();

  configFile.release();
}

void fileio::writePoseToCSV(
  std::ofstream& csv_file,
  const cv::Vec3d& tvec,
  const cv::Vec3d& rvec)
{
  csv_file << tvec[0] << ", " << tvec[1] << ", " << tvec[2] << ", "
           << rvec[0] << ", " << rvec[1] << ", " << rvec[2] << '\n';
}
