#include <stdexcept>

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "PolygonTrack.hpp"

namespace tracks {

PolygonTrack::PolygonTrack(const cv::Point2d& centerIn, int sidesIn, double widthIn, double heightIn)
: Track(),
  _center {centerIn},
  _width {widthIn},
  _height {heightIn},
  _sidesQuantity {sidesIn}
{
  generateSidesAndVertices();

  _objectPoints3D.reserve(_sidesQuantity);
  _imagePoints2D.reserve(_sidesQuantity);
  _polyLinesPoints.reserve(_sidesQuantity * 2);

  for (const auto& side: _sides)
    _objectPoints3D.emplace_back(side.getPoint1().x, side.getPoint1().y, 0);
}

PolygonTrack::PolygonTrack(const cv::FileStorage& cvFileObjectIn)
: Track()
{
  readFromConfigFile(cvFileObjectIn);
  generateSidesAndVertices();

  _objectPoints3D.reserve(_sidesQuantity);
  _imagePoints2D.reserve(_sidesQuantity);
  _polyLinesPoints.reserve(_sidesQuantity * 2);

  for (const auto& side: _sides)
    _objectPoints3D.emplace_back(side.getPoint1().x, side.getPoint1().y, 0);
}

double PolygonTrack::calculatePerpendicularDistance(const cv::Point2d& positionIn) const
{
  double distance {INFINITY};
  double newDistance {};

  for (const auto& side: _sides) {
    newDistance = side.calculatePerpendicularDistance(positionIn);

    if (newDistance >= distance)
      continue;

    distance = newDistance;
  }

  return distance;
}

void PolygonTrack::generateSidesAndVertices()
{
  _sides.clear();
  _sides.reserve(_sidesQuantity);

  double widthHalf {_width * 0.5};
  double heightHalf {_height * 0.5};
  double stepSize {(2 * M_PI) / _sidesQuantity};

  for (double theta {0}; theta <= 2 * M_PI; theta += stepSize) {
    double xVertex1 {_center.x + widthHalf * std::cos(theta)};
    double yVertex1 {_center.y + heightHalf * std::sin(theta)};

    double xVertex2 {_center.x + widthHalf * std::cos(theta + stepSize)};
    double yVertex2 {_center.y + heightHalf * std::sin(theta + stepSize)};

    _sides.emplace_back(cv::Point2d{xVertex1, yVertex1}, cv::Point2d{xVertex2, yVertex2});
  }
}

void PolygonTrack::readFromConfigFile(const cv::FileStorage& cvFileObjectIn)
{
  auto polygonTrackNode {cvFileObjectIn[Types.at(getType())]};

  if (polygonTrackNode.empty() || !polygonTrackNode.isMap()) {
    throw std::runtime_error(
      "Polygon track section is incorrectly formatted or has missing information.");
  }

  auto centerXNode {polygonTrackNode["center"]["x_meters"]};
  auto centerYnode {polygonTrackNode["center"]["y_meters"]};
  auto widthNode {polygonTrackNode["width_meters"]};
  auto heightNode {polygonTrackNode["height_meters"]};
  auto sidesQuantityNode {polygonTrackNode["sides"]};

  if (!centerXNode.isReal() || !centerYnode.isReal()
    || !widthNode.isReal() || !heightNode.isReal()
    || !sidesQuantityNode.isInt()) {

    throw std::runtime_error(
      "Polygon track section is incorrectly formatted or has missing information.");
  }

  cv::read(centerXNode, _center.x, 0.0);
  cv::read(centerYnode, _center.y, 0.0);
  cv::read(widthNode, _width, 0.0);
  cv::read(heightNode, _height, 0.0);
  cv::read(sidesQuantityNode, _sidesQuantity, 0);
}

void PolygonTrack::writeToConfigFile(cv::FileStorage& cvFileObjectOut)
{
  cvFileObjectOut.writeComment("");
  cvFileObjectOut.startWriteStruct(Types.at(getType()), cv::FileNode::MAP);

    cvFileObjectOut.startWriteStruct("center", cv::FileNode::MAP);
      cvFileObjectOut << "x_meters" << _center.x;
      cvFileObjectOut << "y_meters" << _center.y;
    cvFileObjectOut.endWriteStruct();

    cvFileObjectOut << "width_meters" << _width;
    cvFileObjectOut << "height_meters" << _height;
    cvFileObjectOut << "sides" << _sidesQuantity;

  cvFileObjectOut.endWriteStruct();
}

matplot::line_handle PolygonTrack::plot(matplot::axes_handle& axesHandleOut) const
{
  matplot::line_handle lastLineTrackPlot {};
  axesHandleOut->hold(matplot::on);

  for (const auto& side: _sides)
    lastLineTrackPlot = side.plot(axesHandleOut);

  axesHandleOut->hold(matplot::off);

  return lastLineTrackPlot;
}

void PolygonTrack::drawOnFrame(
  cv::Mat& frameOut,
  const cv::Affine3d& cameraExtrinsicIn,
  const cv::Mat& cameraIntrinsicCamMatrix,
  const cv::Mat& cameraIntrinsicDistCoeffs)
{
  if (_objectPoints3D.empty())
    return;

  _imagePoints2D.clear();
  _polyLinesPoints.clear();

  cv::projectPoints(
    _objectPoints3D,
    cameraExtrinsicIn.rvec(), cameraExtrinsicIn.translation(),
    cameraIntrinsicCamMatrix, cameraIntrinsicDistCoeffs,
    _imagePoints2D);

  for (auto pointIt {_imagePoints2D.begin()}; pointIt < _imagePoints2D.end(); pointIt++) {
    auto nextPointit {std::next(pointIt)};

    if (nextPointit == _imagePoints2D.end())
      break;

    _polyLinesPoints.push_back({*pointIt, *nextPointit});
  }

  cv::polylines(frameOut, _polyLinesPoints, true, {0, 255, 0});
}

}
