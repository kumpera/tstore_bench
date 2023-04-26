#include <torch/csrc/distributed/c10d/TCPStore.hpp>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <time.h>

size_t the_world_size;
size_t the_local_size;
int barrier_method;

size_t get_time(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}


int ranks_in = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void local_barrier() {
    pthread_mutex_lock(&mutex);
    ++ranks_in;
    if (ranks_in >= the_world_size) {
        pthread_cond_broadcast(&cond);
    }
    while (ranks_in < the_world_size) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}


size_t _store_based_barrier_last_rank(int rank, c10d::Store &store){
    std::string barrier_prefix = "_foo";

    std::string barrier_count_key = barrier_prefix + "/rank_count";
    std::string barrier_last_rank_key = barrier_prefix + "/last_rank";


    if(store.add(barrier_count_key, 1) == the_world_size)
        store.set(barrier_last_rank_key, "1");
    
    std::vector<std::string> keys = { barrier_last_rank_key };

    size_t retries = 0;
    while(1) {
        try {
            store.wait(keys, std::chrono::milliseconds{10000}); //10sec
            break;
        } catch(std::exception &e) {
            ++retries;
            // printf("10seconds was too much, retry\n");
        }
    }

    return retries;
}

void gloo_find_local_rank(int rank, const std::string &localHostName, c10d::Store &store){
    int localRank = 0;

    std::string localKey("rank_" + std::to_string(rank));
    const std::vector<uint8_t> value(localHostName.begin(), localHostName.end());
    store.set(localKey, value);

    for (int i = 0; i < the_world_size; i++) {
        if (i == rank) {
            break;
        }

        std::string key("rank_" + std::to_string(i));
        auto val = store.get(key);
        auto hostName = std::string((const char*)val.data(), val.size());

        if (hostName == localHostName) {
            localRank++;
        }
    }
}

void append_base_find_local_rank(int rank, const std::string &localHostName, c10d::Store &store){
    int localRank = 0;

    std::vector<uint8_t> vals(4);
    memcpy(vals.data(), &rank, 4);
    store.append(localHostName, vals);

    _store_based_barrier_last_rank(rank, store);

    vals = store.get(localHostName);
    std::vector<int> ranks;
    for(int i = 0; i < vals.size(); i += 4) {
        uint8_t *ptr = &vals[i];
        ranks.push_back(*(int*)ptr);
    }
    std::sort(ranks.begin(), ranks.end());
    for(int i = 0; i < ranks.size(); ++i) {
        if (ranks[i] == rank) {
            localRank = i;
            break;
        }
    }
}

void* run_worker(void *arg){ 
    size_t my_rank = (size_t)arg;
    size_t my_local_rank = my_rank % the_local_size;

    c10d::TCPStoreOptions opts;
    opts.port = 29501;
    opts.numWorkers = the_world_size;
    if (my_rank == 0) {
        opts.isServer = true;
    }

    c10d::TCPStore store("127.0.0.1", opts);
    size_t start = get_time();
    try {
        switch(barrier_method){
            case 0:
                gloo_find_local_rank(my_rank, std::to_string(my_local_rank), store);
                break;
            case 1:
                append_base_find_local_rank(my_rank, std::to_string(my_local_rank), store);
                break;
        }
    } catch(std::exception &e){
        printf("failed due to %s\n", e.what());
    }

    size_t end = get_time();
    auto res = end - start;
    local_barrier();
    return (void*)res;
}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Please use ./local_rank <world_size:int> <local_size:int> <barrier_method:int>\n");
        return 1;
    }
    the_world_size = atoi(argv[1]);
    the_local_size = atoi(argv[2]);
    barrier_method = atoi(argv[3]);

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
    printf("RANKS: %zu LOCAL-RANKS: %zu method: %d TOOK %zu ms \n", the_world_size, the_local_size, barrier_method, max_r);
}