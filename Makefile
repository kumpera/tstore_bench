#Change this to your torch install path
TORCH_HOME=/scratch/kumpera/pytorch

store_barrier: store_barrier.cpp
	clang++ -lm -lpthread -ltorch -ltorch_cpu -L${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include store_barrier.cpp -o store_barrier

local_rank: local_rank.cpp
	clang++ -lm -lpthread -ltorch -ltorch_cpu -L${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include local_rank.cpp -o local_rank

gloo_init: gloo_init.cpp
	clang++ -lm -lpthread -ltorch -ltorch_cpu -L${TORCH_HOME}/torch/lib -I${TORCH_HOME}/torch/include gloo_init.cpp -o gloo_init

run: store_barrier
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TORCH_HOME}/torch/lib ./store_barrier 2 0

run_g: gloo_init
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TORCH_HOME}/torch/lib ./gloo_init 16 4

run_l: local_rank
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TORCH_HOME}/torch/lib ./local_rank 4 2 1

.phony: run