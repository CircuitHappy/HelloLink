Initial test of Ableton Link on Raspberry pi, lighting LEDs for
clock and reset.

This is just a scratched-together example and not intended for
distribution. There's no package or dependency management, no
submodules, etc so the Pi environment must be configured exactly right for this to build.

1. Make sure you have CMake >3.0 and GCC >5.2 installed as packages.
1. Download and install [WiringPi](http://wiringpi.com/download-and-install/)
1. Clone [Ableton Link](https://github.com/Ableton/link) into `lib/`
    * `git clone https://github.com/Ableton/link.git lib/link`
1. Run CMake
    * `cmake .`
1. Build
    * `make`
1. Run with `sudo`
    * `sudo bin/link_test`

The pins used for clock/reset output are defined clearly in `main.cpp`. 
Hook up LEDs to those pins (using the [wiringPi numbering scheme](http://wiringpi.com/pins/)), 
connect the Pi to your local network and GO!
