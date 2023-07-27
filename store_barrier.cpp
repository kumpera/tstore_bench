#include <torch/csrc/distributed/c10d/TCPStore.hpp>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <time.h>

size_t the_world_size;
int barrier_method;
int use_libuv;

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


size_t get_time(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

size_t _store_based_barrier(int rank, c10d::TCPStore &store){
    std::string store_key = "the_key_this_doesnt_matter";

    store.add(store_key, 1);
    auto world_size = the_world_size;

    size_t retries = 0;
    auto worker_count = store.add(store_key, 0);
    while(worker_count != world_size) {
        usleep(10 * 1000); //10ms
        worker_count = store.add(store_key, 0);
        ++retries;
    }

    // printf("store barrier used %d retries\n", retries);
    return retries;
}

size_t _store_based_barrier_howard(int rank, c10d::Store &store){
    std::vector<std::string> keys;

    for(int i = 0; i <  the_world_size; ++i) {
        std::string rank_key = "rank_" + std::to_string(i);
        if (i == rank) {
            std::string value = "1";
            store.set(rank_key, value);
        }
        keys.push_back(rank_key);

    }

    size_t retries = 0;
    while(1) {
        try {
            store.wait(keys, std::chrono::milliseconds{10000}); //10sec
            break;
        } catch(std::exception &e) {
            ++retries;
            printf("10seconds was too much, retry\n");
        }
    }

    return retries;
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
            printf("10seconds was too much, retry\n");
        }
    }

    return retries;
}

void* run_worker(void *arg){
    size_t my_rank = (size_t)arg;
    try {
        c10d::TCPStoreOptions opts;
        opts.port = 29501;
        opts.numWorkers = the_world_size;
        opts.useLibUV = use_libuv == 1;
        if (my_rank == 0) {
            opts.isServer = true;
        }

        c10d::TCPStore store("127.0.0.1", opts);
        size_t start = get_time();
        switch(barrier_method){
            case 0:
                _store_based_barrier(my_rank, store);
                break;
            case 1:
                _store_based_barrier_howard(my_rank, store);
                break;
            case 2:
                _store_based_barrier_last_rank(my_rank, store);
                break;
        }
        size_t end = get_time();
        local_barrier();

        auto res = end - start;
        // if (my_rank == 0)
        //     printf("master took %zu ms\n", end - start);
        return (void*)res;
    } catch (std::exception &ex) {
        ssize_t res = -1;
        return (void*)res;
    }



}

int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Please use ./store_barrier <world_size:int> <barrier_method:int> <store_impl:int>\n");
        printf("Valid values for barrier_method: 0 1 2\n");
        printf("Valid values for store_impl: 0 1 (0 classic, 1 libuv)\n");
        return 1;
    }
    the_world_size = atoi(argv[1]);
    barrier_method = atoi(argv[2]);
    use_libuv = atoi(argv[3]);

    std::vector<pthread_t> the_threads;
    pthread_t pt;
    pthread_create(&pt, nullptr, run_worker, (void*)0);
    the_threads.push_back(pt);


    for(size_t i = 1; i < the_world_size; ++i) {
        pthread_create(&pt, nullptr, run_worker, (void*)i);
        the_threads.push_back(pt);
    }

    ssize_t max_r = 0;
    for(size_t i = 0; i < the_world_size; ++i) {
        ssize_t ret = 0;
        pthread_join(the_threads[i], (void**)&ret);
        max_r = std::max(max_r, ret);
    }
    printf("RANKS: %zu method: %d store: %d TOOK %zu ms \n", the_world_size, barrier_method, use_libuv, max_r);
}
