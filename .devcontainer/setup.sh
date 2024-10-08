# !/bin/bash

set -e

PROJECT_ROOT=$HOME/line-follower-tracker

if [ ! -d $PROJECT_ROOT ]
then
  printf "Setup failed: Project root directory not found\n"
fi

# Grants webcam access
sudo usermod -aG video 467-dev

# Sets the XAUTHORITY variable correctly everytime a new shell opens
echo $'\nexport XAUTHORITY=$(ls /run/user/$UID/.mutter-*)' >> ~/.bashrc

# Install some dependencies for OpenCV and its modules
sudo apt update && sudo apt install -y \
  python2 \
  default-jdk \
  default-jre \
  ffmpeg \
  qtbase5-dev \
  qt5-qmake \
  libglu1-mesa-dev \
  mesa-common-dev \
  freeglut3-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-bad1.0-dev \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav \
  gstreamer1.0-tools \
  gstreamer1.0-x \
  gstreamer1.0-alsa \
  gstreamer1.0-gl \
  gstreamer1.0-gtk3 \
  gstreamer1.0-qt5 \
  gstreamer1.0-pulseaudio \
  libjpeg-dev \
  libtiff-dev \
  libopenjp2-7 \
  libopenjp2-7-dev \
  libopenjp2-tools \
  libopenjp3d-tools \
  libopenjp3d7 \
  libopenjpip-dec-server \
  libopenjpip-server \
  libopenjpip-viewer \
  libopenjpip7 \
  openjpeg-doc \
  libglfw3-dev \
  v4l-utils \
  libgtk-3-dev

pip3 install pylint flake8 vtk

# Create a libs directory to clone the source code of libraries ------------------------------------
if [ ! -d $PROJECT_ROOT/libs ]
then
  mkdir -p $PROJECT_ROOT/libs
fi

cd $PROJECT_ROOT/libs

# $PROJECT_ROOT/libs is now the current working directory

# Clone OpenCV Source ------------------------------------------------------------------------------
if [ ! -d opencv-4.9.0 ]
then
    wget -O opencv.zip https://github.com/opencv/opencv/archive/refs/tags/4.9.0.zip
    unzip opencv.zip && rm opencv.zip
fi

# Clone OpenCV modules source ----------------------------------------------------------------------
if [ ! -d opencv_contrib ]
then
    git clone -b 4.x https://github.com/opencv/opencv_contrib.git
fi

# Build the library with the aruco module and install it
if [ -d opencv-4.9.0/build ]
then
  rm -rf opencv-4.9.0/build
fi

mkdir -p opencv-4.9.0/build && cd opencv-4.9.0/build

# $PROJECT_ROOT/libs/opencv-4.9.0/build is now the current working directory

cmake -D OPENCV_EXTRA_MODULES_PATH=$PROJECT_ROOT/libs/opencv_contrib/modules \
      -D BUILD_LIST=calib3d,highgui,objdetect,aruco,videoio \
      -D WITH_OPENGL=ON \
      -D WITH_QT=ON \
      ..

make -j$(nproc)
sudo make install

cd $PROJECT_ROOT/libs

# $PROJECT_ROOT/libs is now the current working directory

# Clone matplot++ source ---------------------------------------------------------------------------
if [ ! -d matplotplusplus ]
then
  git clone https://github.com/alandefreitas/matplotplusplus.git
fi

cd matplotplusplus

# $PROJECT_ROOT/libs/matplotplusplus is now the current working directory

if [ -d build ]
then
  rm -rf build
fi

# Build the library and install it
cmake --preset=system
cmake --build --preset=system
sudo cmake --install build/system

cd $PROJECT_ROOT/libs

# $PROJECT_ROOT/libs is now the current working directory

# Clone ImGui source -------------------------------------------------------------------------------
if [ ! -d imgui ]
then
  git clone https://github.com/ocornut/imgui.git
fi

# Clone ImPlot source ------------------------------------------------------------------------------
if [ ! -d implot ]
then
  git clone https://github.com/epezent/implot.git
fi

# Clone nativefiledialog-extended source -----------------------------------------------------------
if [ ! -d nativefiledialog-extended ]
then
  git clone https://github.com/btzy/nativefiledialog-extended.git
fi

printf "\nSetup complete, environment is now ready to use! \n"
