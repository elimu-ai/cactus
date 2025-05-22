cd cactus-tests

rm -rf build
mkdir -p build
cd build
cmake ..
make

# create default.metallib symlink for Metal backend
ln -sf ../../cactus/ggml-llama.metallib default.metallib
./cactus_test