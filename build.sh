mkdir thirdparty
cd thirdparty

git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c
git checkout cpp_master

cd ../../examples
mkdir build

cd build
# rm -rf *
cmake .. 
make -j `nproc`

cd ..
