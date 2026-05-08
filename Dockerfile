# qBittorrent Android Build Environment
# Ubuntu 24.04 with all required tools

FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install base packages
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    unzip \
    git \
    build-essential \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    clang \
    llvm \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Install OpenJDK 17
RUN apt-get update && apt-get install -y \
    openjdk-17-jdk \
    && rm -rf /var/lib/apt/lists/*

# Set JAVA_HOME
ENV JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# Create Android SDK directory
ENV ANDROID_HOME=/opt/android-sdk
RUN mkdir -p $ANDROID_HOME

# Download and install Android command line tools
WORKDIR /tmp
RUN wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmdline-tools.zip && \
    unzip -q cmdline-tools.zip && \
    mkdir -p $ANDROID_HOME/cmdline-tools && \
    mv cmdline-tools $ANDROID_HOME/cmdline-tools/latest && \
    rm cmdline-tools.zip

# Install Android SDK components
ENV PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools
RUN yes | sdkmanager --licenses > /dev/null 2>&1 || true && \
    sdkmanager "platforms;android-35" "build-tools;35.0.0" "ndk;27.0.12077973" "cmake;3.22.1"

# Install Gradle 8.4
WORKDIR /tmp
RUN wget -q https://services.gradle.org/distributions/gradle-8.4-bin.zip -O gradle.zip && \
    unzip -q gradle.zip && \
    mv gradle-8.4 /opt/gradle && \
    rm gradle.zip

# Add Gradle to PATH
ENV PATH=$PATH:/opt/gradle/bin

# Set Android NDK home
ENV ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973

# Create working directory
WORKDIR /workspace
RUN mkdir -p /workspace

# Default command
CMD ["/bin/bash"]