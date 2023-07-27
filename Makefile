#Change this to your torch install path
#TODO use torch.utils.cpp_extension.include_paths()
TORCH_HOME=/home/kumpera/src/pytorch

LIB_LIST=-lm -lpthread -ltorch -lc10 -ltorch_cpu

store_barrier: store_barrier.cpp Makefile
	c++ -O2 ${LIB_LIST} -L${TORCH_HOME}/torch/lib -Wl,-rpath,${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include store_barrier.cpp -o store_barrier

local_rank: local_rank.cpp
	c++ -O2 ${LIB_LIST} -L${TORCH_HOME}/torch/lib -Wl,-rpath,${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include local_rank.cpp -o local_rank

gloo_init: gloo_init.cpp
	c++ -O2 ${LIB_LIST} -L${TORCH_HOME}/torch/lib -Wl,-rpath,${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include gloo_init.cpp -o gloo_init

run: store_barrier
	./store_barrier 2 0

run_g: gloo_init
	./gloo_init 16 4

run_l: local_rank
	./local_rank 4 2 1

.phony: run
