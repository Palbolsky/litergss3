FROM debian:latest

USER root

# Ensures repositories are up-to-date
RUN apt-get update

# Install SFML
RUN apt-get install -y libsfml-dev

# Setup build system
RUN apt-get install -y \
cmake \
build-essential

# Setup x VNC server
RUN apt-get install -y xvfb x11vnc fluxbox

USER root
# Upgrade everything
RUN apt-get -y upgrade

COPY ./entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

WORKDIR /litecgss
