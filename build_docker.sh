#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Incorrect number of parameters; "
  echo "[build_docker.sh] <output_dir>"
  echo ""
  exit 2
fi

touch "$1"/test
if [ "$?" -ne 0 ]; then
  echo "Unable to write to output directory $1; exiting."
  exit 1
fi
rm "$1"/test


LOCAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
cd "$LOCAL_DIR" || exit 1
docker build -f src/docker/Dockerfile -t lofar-upm:latest .
docker run -v /var/run/docker.sock:/var/run/docker.sock -v "$1":/output --privileged -t --rm singularityware/docker2singularity lofar-upm:latest