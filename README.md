# OpcUaHalServer
 
To use it, you need to install the open62541 library.
    git clone github.com/open62541/open62541.git
    cd open62541
    mkdir build && cd build
    cmake ..
    make
    sudo make install
Then
    git clone github.com/TheRoslyak/OpcUaHalServer.git
    sudo make

Launch linuxcnc and
 enter "opcuaserver" in terminal 