==========================================
HIC Agent - Headless Indi Controller Agent
==========================================

.. image:: https://img.shields.io/github/license/gehelem/indi_hic_agent
.. image:: https://img.shields.io/travis/com/gehelem/indi_hic_agent/master

See :
https://www.webastro.net/forums/topic/180582-la-qu%C3%AAte-dun-de-mes-graal-un-setup-vraiment-headless/

Building - Debian/Ubuntu
========================

Install Pre-requisites
++++++++++++++++++++++

.. code-block:: shell

  sudo apt update
  sudo apt -y install software-properties-common build-essential git cmake
  sudo apt-add-repository ppa:mutlaqja/ppa
  sudo apt update
  sudo apt -y install libnova-dev libcfitsio-dev libusb-1.0-0-dev zlib1g-dev libgsl-dev libjpeg-dev libcurl4-gnutls-dev libtiff-dev libfftw3-dev libftdi-dev libgps-dev libraw-dev libdc1394-22-dev libgphoto2-dev libboost-dev libboost-regex-dev librtlsdr-dev liblimesuite-dev libftdi1-dev libavcodec-dev libavdevice-dev libopencv-dev software-properties-common indi-full gsc libindi-dev

Build / install
+++++++++++++++

.. code-block:: shell
    
  git clone  https://github.com/gehelem/indi_hic_agent.git
  cd ~/indi_hic_agent
  mkdir build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
  sudo make install

Testing
+++++++

Start indi server  :

.. code-block:: shell

  indiserver -v -m 100 indi_hic_agent indi_simulator_telescope indi_simulator_ccd indi_simulator_focus indi_simulator_guide indi_simulator_wheel indi_simulator_gps

And connect with any client providing access to driver capabilities (Kstars control panel, AstrophotoPlus, Mobindi ...)
