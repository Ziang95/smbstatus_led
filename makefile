compiler=g++
build_dir=build
cpp_files=$(wildcard ./*.cpp)

build/smbmonitor : build $(cpp_files) build/libchroma.so
	$(compiler) -Lbuild/ $(cpp_files) -std=c++14 -pthread -Wl,-rpath='$$ORIGIN' -o $(build_dir)/smbmonitor -lchroma

build/libchroma.so : build
	git submodule init
	git submodule foreach git fetch
	git submodule foreach git pull
	make -C Chroma_Pi
	cp Chroma_Pi/build/libchroma.so build/

build/dbg:target=dbg

build/dbg:build $(cpp_files)
	$(compiler) -g -Lbuild/ $(cpp_files) -pthread -Wl,-rpath='$$ORIGIN' -o $(build_dir)/dbg -lchroma


build :
	mkdir build