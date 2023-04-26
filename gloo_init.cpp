#include <torch/csrc/distributed/c10d/TCPStore.hpp>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <time.h>

size_t the_world_size;
size_t host_num;

size_t get_time(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

int _get_local_rank(size_t rank, c10d::TCPStore& store, std::string localHostName){
    std::string localKey("rank_" + std::to_string(rank));
    // printf(localHostName);
    const std::vector<uint8_t> value(localHostName.begin(), localHostName.end());
    store.set(localKey, value);
    int localRank = 0;
    printf("Store Set finished.\n");
    for (int i = 0; i < the_world_size; i++) {
        if (i == rank) {
            break;
        }

        std::string key("rank_" + std::to_string(i));
        auto val = store.get(key);
        printf("Store Get finished.\n");
        std::string hostName("");
        for(auto i : val)
            hostName += std::to_string(i);
        // auto hostName = std::string((const uint8_t*)val.data(), val.size());

        if (hostName == localHostName) {
        localRank++;
        }

        return localRank;
    }
}

void* run_worker(void *arg){ 
    size_t my_rank = (size_t)arg;
    // printf("running rank %zu\n", my_rank);

    c10d::TCPStoreOptions opts;
    opts.port = 29501;
    opts.numWorkers = the_world_size;
    if (my_rank == 0) {
        opts.isServer = true;
    }
    int hostId = my_rank % host_num;
    std::string hostName(std::to_string(hostId));
    printf("am I server? %d\n", opts.isServer);

    c10d::TCPStore store("127.0.0.1", opts);
    size_t start = get_time();
    _get_local_rank(my_rank, store, hostName); 

    size_t end = get_time();
    sleep(5);
    auto res = end - start;
    if (my_rank == 0)
        printf("master took %zu ms\n", end - start);
    return (void*)res;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Please use ./amazing_benchmark <world_size:int> <host_num:int>\n");
        return 1;
    }
    the_world_size = atoi(argv[1]);
    host_num = atoi(argv[2]);
    // printf("lets rendezvour %zu ranks\n", the_world_size);

    std::vector<pthread_t> the_threads;
    pthread_t pt;
    pthread_create(&pt, nullptr, run_worker, (void*)0);
    the_threads.push_back(pt);

    // sleep(1);

    for(size_t i = 1; i < the_world_size; ++i) {
        pthread_create(&pt, nullptr, run_worker, (void*)i);
        the_threads.push_back(pt);
    }

    size_t max_r = 0;
    for(size_t i = 0; i < the_world_size; ++i) {
        size_t ret = 0;
        pthread_join(the_threads[i], (void**)&ret);
        max_r = std::max(max_r, ret);
    }
    printf("RANKS: %zu method: %d TOOK %zu ms \n", the_world_size, host_num, max_r);
}