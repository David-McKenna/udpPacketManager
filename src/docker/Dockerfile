FROM nvidia/cuda:10.2-cudnn7-devel
SHELL ["/bin/bash", "-l", "-c"]

# Should be updated to the value in the makefile at runtime
ARG BUILD_CORES=8

# Install all required packages available from the disto package manager
RUN apt-get update && \
	apt-get install -y software-properties-common apt-utils wget && \
	add-apt-repository -y multiverse && \
	add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
	cd /tmp && wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2023.PUB && apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS-2023.PUB && \
	add-apt-repository -y "deb https://apt.repos.intel.com/oneapi all main"

# Handle tzdata dependency asking for input on server location
ARG DEBIAN_FRONTEND=noninteractive
# Install remaining packages; may be excessive after removing some of the install targets.
RUN apt-get update && \
	apt-get install -y --no-install-recommends autoconf autogen automake autotools-dev cmake csh g++-7 g++-9 g++-multilib gcc-7 gcc-9 gcc-multilib gdb gfortran gfortran git gnupg-agent gsl-bin hwloc intel-hpckit intel-oneapi-ipp intel-oneapi-ipp-devel intel-oneapi-mkl intel-oneapi-mkl-devel intel-oneapi-tbb intel-oneapi-tbb-devel intel-oneapi-openmp intel-oneapi-tbb intel-oneapi-tbb-devel libblas-dev libfftw3-3 libfftw3-bin libfftw3-dev libfftw3-single3 libglib2.0-dev libgsl-dev libgslcblas0 libtool libtool-bin locate nano python3-dev python3-matplotlib python3-pip python3-setuptools python3-wheel vim zstd libzstd-dev && \
	apt-get remove -y python2.7* libpython2* libpython-* intel-oneapi-dnn* intel-oneapi-python intel-oneapi-dal* && \
	update-alternatives --install /usr/bin/python python /usr/bin/python3 60 && \
	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9 && \
	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 90 --slave /usr/bin/g++ g++ /usr/bin/g++-7 && \
	rm -rf /var/lib/apt/lists/* && \
	pip3 install --upgrade pip numpy setuptools wheel git+https://github.com/FRBs/sigpyproc3 ipython tqdm

# Setup enviroment variables to access installed software
# lstdc++ included in c compiles to workaround a bug in PSRDADA's CUDA compile
ENV SOFT /home/soft
ENV CUDA_HOME /usr/local/cuda
ENV CUDA_TOOLKIT_ROOT_DIR $CUDA_HOME
ENV LIBRARY_PATH $CUDA_HOME/lib64:/usr/local/lib:$LIBRARY_PATH
ENV LD_LIBRARY_PATH $CUDA_HOME/lib64:/usr/local/lib:$LD_LIBRARY_PATH
ENV CFLAGS "-mtune=native -I$CUDA_HOME/include $CFLAGS -lstdc++"
ENV CPPFLAGS "$CFLAGS $CPP_FLAGS"
ENV CUDA_CFLAGS "-I$CUDA_HOME/include $CUDA_CFLAGS -lstdc++"
ENV CUDA_LIBS "-L$CUDA_HOME/lib64 -lcudart $CUDA_LIBS -lstdc++"
ENV PAKCAGES "/usr/local/cuda"
ENV CC icc
ENV CXX icpc
ENV F77 ifort
ENV FC ifort
RUN touch ~/.bash_profile && echo 'unset CMPLR_ROOT; source /opt/intel/oneapi/setvars.sh; alias icc="icx"; alias icpc="icpx"' >> ~/.bash_profile




# Get the latest version of in-active-development software
# This arg is updated at build time by the makefile and will cause
# the remainder of this dockerfile to recompile every time it is run
# on a new day

WORKDIR /home/soft
ARG BUILD_DATE=2020-01-01

RUN touch $BUILD_DATE && \
	git clone https://github.com/David-McKenna/cdmt.git && \
	git clone https://github.com/NVlabs/cub.git cub && \
	git clone https://git.code.sf.net/p/psrdada/code psrdada && \
	git clone https://github.com/AA-ALERT/psrdada-python && \
	git clone https://github.com/David-McKenna/udpPacketManager.git

# Build PSRDADA
WORKDIR $SOFT/psrdada
ENV FLAGS "-lstdc++"
RUN cp -r $SOFT/cub /usr/local/include/ && updatedb && ldconfig && \
	./bootstrap && \
	./configure --enable-shared --x-libraries=/usr/lib/x86_64-linux-gnu && \
	make -j $BUILD_CORES && \
	make install
ENV FLAGS ""

# Build and install udpPacketManager/mockHeader, riptide, PSRSalsa, CDMT, IQRM, PSRDADA-python
# Comment: I'm compiling udpPacketManager twice here (cdmt clones + compiles on it's own)
WORKDIR $SOFT/udpPacketManager
RUN echo "Building udpPacketManager / mockHeader" && \
	\
	apt-get update && \
	make mockHeader calibration-prep all install && \
	rm -rf /var/lib/apt/lists/* && \
	\
	echo "Buidling CDMT" && \
	cd $SOFT/cdmt && git pull origin && make cdmt_udp CC=icc && make cdmt_udp_stokesV CC=icc && cp ./cdmt_ud* /usr/local/bin/ &&  \
	\
	echo "Building PSRDADA-python" && \
	cd $SOFT/psrdada-python && python3 -m pip install -r ./requirements.txt && make && make test && make install

WORKDIR /home/user
ENTRYPOINT /bin/bash
