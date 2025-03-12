#pragma once

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace std;

// macro
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
#define MAX_OBJECT_BLOCK_NUM (5 + 1)

// global functions
static inline void print_log(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "\033[1;31m"); // 设置红色高亮
  vfprintf(stderr, format, args);// 格式化输出
  fprintf(stderr, "\033[0m");    // 恢复默认颜色
  va_end(args);
}

template<typename T>
class ConcurrencyQueue {
private:
  mutex mtx;
  condition_variable cv;
  queue<T> queue_;

public:
  void push(const T &req) {
    unique_lock<mutex> lock(mtx);
    queue_.push(req);
    cv.notify_one();
  }

  T pop() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this] { return !queue_.empty(); });
    T req = queue_.front();
    queue_.pop();
    return req;
  }

  auto size() {
    unique_lock<mutex> lock(mtx);
    return queue_.size();
  }


  bool empty() {
    unique_lock<mutex> lock(mtx);
    return queue_.empty();
  }
};