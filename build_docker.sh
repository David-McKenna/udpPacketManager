#!/bin/bash

if [ "$#" -gt 2 ]; then
  echo "Incorrect number of parameters; "
  echo "[build_docker.sh] <optional: singularity output_dir>"
  echo ""
  exit 2
fi


LOCAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
cd "$LOCAL_DIR" || exit 1
echo "Building Docker image with tag lofar-upm:latest from $LOCAL_DIR"
docker build -f src/docker/Dockerfile -t lofar-upm:latest .

if [ "$#" -eq 2 ]; then
  echo "Building singulairty image, outputting in directory $1"
  touch "$1"/test

  if [ "$?" -ne 0 ]; then
    echo "Unable to write to output directory $1; exiting."
    exit 1
  fi
  rm "$1"/test

  docker run -v /var/run/docker.sock:/var/run/docker.sock -v "$1":/output --privileged -t --rm singularityware/docker2singularity lofar-upm:latest
fi