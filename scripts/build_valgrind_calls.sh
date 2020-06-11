#!/bin/sh
VALGRIND_OPTS="-v -q --tool=callgrind --num-callers=50 --error-exitcode=1  --suppressions=suppress.valgrind"
cd ..
if [ ! -f build/suppress.valgrind ]; then
    rm -rf build/*
    printf "{\n  ignore_libcrypto_conditional_jump_errors\n  Memcheck:Leak\n  ...\n  obj:*/libcrypto.so.*\n}\n"  > build/suppress.valgrind
    docker run \
      --rm \
      -v $(pwd):$(pwd) \
      docker.slock.it/build-images/cmake:valgrind \
      /bin/bash -c "cd /$(pwd)/build;  cmake -DCMAKE_BUILD_TYPE=Release -DTEST=true  .."
fi

# build
docker run --rm -v $(pwd):$(pwd)  docker.slock.it/build-images/cmake:valgrind  /bin/bash -c "cd /$(pwd)/build;  make -j8"

clear

# run tests
for f in build/test/test*; do 
  docker run --rm -v $(pwd):$(pwd)  docker.slock.it/build-images/cmake:valgrind  /bin/bash -c "cd $(pwd)/build; valgrind $VALGRIND_OPTS --callgrind-out-file=$(pwd)/$f.out $(pwd)/$f;  callgrind_annotate --tree=both --inclusive=no $(pwd)/$f.out > $(pwd)/$f.txt"
done

cd scripts
