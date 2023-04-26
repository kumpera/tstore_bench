
ranks=(100 200 300 400 500 600 700 800 900 1000 1100 1200 1300)
for COUNT in "${ranks[@]}"; do
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/scratch/kumpera/pytorch/torch/lib ./local_rank $COUNT 8 0
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/scratch/kumpera/pytorch/torch/lib ./local_rank $COUNT 8 1
    # LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/scratch/kumpera/pytorch/torch/lib ./a.out $COUNT 1
    # LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/scratch/kumpera/pytorch/torch/lib ./a.out $COUNT 2
done

# RANKS: 100 LOCAL-RANKS: 8 method: 0 TOOK 1108 ms 
# RANKS: 200 LOCAL-RANKS: 8 method: 0 TOOK 1512 ms 
# RANKS: 300 LOCAL-RANKS: 8 method: 0 TOOK 1291 ms 
# RANKS: 400 LOCAL-RANKS: 8 method: 0 TOOK 2238 ms 
# RANKS: 500 LOCAL-RANKS: 8 method: 0 TOOK 3450 ms 
# RANKS: 600 LOCAL-RANKS: 8 method: 0 TOOK 5685 ms 
# RANKS: 700 LOCAL-RANKS: 8 method: 0 TOOK 7340 ms 
# RANKS: 800 LOCAL-RANKS: 8 method: 0 TOOK 9384 ms 
# RANKS: 900 LOCAL-RANKS: 8 method: 0 TOOK 11626 ms 
# RANKS: 1000 LOCAL-RANKS: 8 method: 0 TOOK 14274 ms 
# RANKS: 1100 LOCAL-RANKS: 8 method: 0 TOOK 17836 ms 
# RANKS: 1200 LOCAL-RANKS: 8 method: 0 TOOK 20141 ms 
# RANKS: 1300 LOCAL-RANKS: 8 method: 0 TOOK 24266 ms 