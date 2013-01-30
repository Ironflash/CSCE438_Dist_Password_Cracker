******CSCE 313-200******
Daniel Tan

compiling:
    compile using the make file command mp5: make mp5

implementation:
    For my implementation I had the closing of the slave sockets (file descriptors) handled within netreqchannel.C; thus, NOT handled by the connection handler thread function in dataserver.C.