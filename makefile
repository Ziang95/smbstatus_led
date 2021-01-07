compiler=g++
build_dir=build
cpp_files=$(wildcard ./*.cpp)

build/smbmonitor : build $(cpp_files)
	$(compiler) -Lbuild/ $(cpp_files) -std=c++14 -pthread -Wl,-rpath='$$ORIGIN' -o $(build_dir)/smbmonitor -lchroma

build/dbg:target=dbg

build/dbg:build $(cpp_files)
	$(compiler) -g -Lbuild/ $(cpp_files) -pthread -Wl,-rpath='$$ORIGIN' -o $(build_dir)/dbg -lchroma


build :
	mkdir build