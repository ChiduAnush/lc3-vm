# an image where gcc & cmake already exists
FROM debian:latest


# Install necessary tools: GCC (for compilation) and CMake (for build automation)
RUN apt-get update && apt-get install -y \
    gcc cmake build-essential 


# Set working directory inside the container
WORKDIR /app

#copy all project files to the app folder(working directory)
COPY . /app



#commands to run the vm using cmake, and get the executable `chiduslc3vm`
RUN mkdir -p build && cd build && cmake .. && make



# Set the default command to run the VM(my executable)
ENTRYPOINT ["/app/build/chiduslc3vm"]
#allows to pass an .obj file as input to the VM which is requried.
CMD [""] 
