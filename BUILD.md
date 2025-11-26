
## Build Instructions

  

1. Create build-folder:
`mkdir build`
`cd build`


2. Run CMake:
`cmake ..`

3. Compile
`make`

4. Install
`sudo make install` 
`sudo ldconfig`


### Linux (Debian/Ubuntu/*-flavors)

Build debian Package
`dpkg-buildpackage -us -uc`