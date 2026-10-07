FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
  locales \
  && locale-gen en_US.UTF-8 \
  && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 \
  && rm -rf /var/lib/apt/lists/*
ENV LANG=en_US.UTF-8

RUN ln -fs /usr/share/zoneinfo/UTC /etc/localtime \
  && apt-get update \
  && apt-get install -y tzdata \
  && dpkg-reconfigure --frontend noninteractive tzdata \
  && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get -y upgrade \
  && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y --no-install-recommends \
  curl \
  gnupg2 \
  lsb-release \
  sudo \
  software-properties-common \
  wget \
  && rm -rf /var/lib/apt/lists/*

# ROS 2 Jazzy + MoveIt / ros2_control / Gazebo bridge deps
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
    -o /usr/share/keyrings/ros-archive-keyring.gpg \
  && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" \
    | tee /etc/apt/sources.list.d/ros2.list > /dev/null \
  && apt-get update && apt-get install -y ros-dev-tools \
  && apt-get install -y --no-install-recommends \
  ros-jazzy-desktop \
  ros-jazzy-xacro \
  ros-jazzy-moveit \
  ros-jazzy-moveit-servo \
  ros-jazzy-moveit-visual-tools \
  ros-jazzy-moveit-resources \
  ros-jazzy-moveit-ros-move-group \
  ros-jazzy-moveit-planners-ompl \
  ros-jazzy-moveit-kinematics \
  ros-jazzy-moveit-ros-perception \
  ros-jazzy-ros2-control \
  ros-jazzy-ros2-controllers \
  ros-jazzy-controller-manager \
  ros-jazzy-joint-state-broadcaster \
  ros-jazzy-joint-state-publisher-gui \
  ros-jazzy-joint-trajectory-controller \
  ros-jazzy-rviz-visual-tools \
  ros-jazzy-geometric-shapes \
  ros-jazzy-gz-ros2-control \
  ros-jazzy-ros-gz \
  ros-jazzy-motion-primitives-controllers \
  python3-argcomplete \
  python3-rosdep \
  && rosdep init \
  && rosdep update \
  && apt-get install -y \
  python3-colcon-common-extensions \
  python3-colcon-mixin \
  && colcon mixin add default https://raw.githubusercontent.com/colcon/colcon-mixin-repository/master/index.yaml \
  && colcon mixin update default \
  && apt-get install -y python3-vcstool \
  && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
  && apt-get install -y curl lsb-release gnupg \
  && curl https://packages.osrfoundation.org/gazebo.gpg \
    --output /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg \
  && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" \
    | tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null \
  && apt-get update \
  && apt-get install -y gz-harmonic \
  && rm -rf /var/lib/apt/lists/*

RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc

SHELL ["/bin/bash", "-c"]

# symphony_driver needs system Puloon RTPRI (find_package). Install SDK into the
# image or mount a prefix and set CMAKE_PREFIX_PATH before colcon build.
RUN mkdir -p /root/symphony_ws/src
COPY ./src /root/symphony_ws/src/

# Without Puloon RTPRI on CMAKE_PREFIX_PATH, skip symphony_driver.
# After installing the SDK: colcon build --packages-select symphony_driver
RUN cd /root/symphony_ws \
  && . /opt/ros/jazzy/setup.bash \
  && colcon build --packages-skip symphony_driver

ENV ROS_DISTRO=jazzy
ENV AMENT_PREFIX_PATH=/opt/ros/jazzy
ENV COLCON_PREFIX_PATH=/opt/ros/jazzy
ENV LD_LIBRARY_PATH=/opt/ros/jazzy/lib/x86_64-linux-gnu:/opt/ros/jazzy/lib
ENV PATH=/opt/ros/jazzy/bin:$PATH
ENV PYTHONPATH=/opt/ros/jazzy/local/lib/python3.12/dist-packages:/opt/ros/jazzy/lib/python3.12/site-packages
ENV ROS_PYTHON_VERSION=3
ENV ROS_VERSION=2
ENV ROS_AUTOMATIC_DISCOVERY_RANGE=SUBNET
ENV DEBIAN_FRONTEND=
ENV GZ_IP=127.0.0.1
