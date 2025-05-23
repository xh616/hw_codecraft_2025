//
// Created by tzx on 25-3-11.
//

#ifndef HW_CODECRAFT_2025_HANDLERIMPL_DEFAULT_H
#define HW_CODECRAFT_2025_HANDLERIMPL_DEFAULT_H

#include "IHandler.h"
extern int timestamp; // 当前时间戳
extern int T; // 时间片数量，范围 1 ≤ T ≤ 86400
extern int M; // 对象标签数量，对象标签编号为1 ~ 𝑀，范围 1 ≤ M ≤ 16
extern int N; // 硬盘个数，硬盘编号为1 ~ 𝑁，范围 3 ≤ N ≤ 10
extern int V; // 每个硬盘的存储单元数，存储单元编号为1 ~ 𝑉，范围 1 ≤ V ≤ 16384，且至少保留10%的空闲空间
extern int G; // 每个磁头在每个时间片可消耗的令牌数，范围 64 ≤ G ≤ 1000
class HandlerImpl_sample : public IHandler {
private:
  typedef struct Request_ {
    int object_id;
    int prev_id;
    bool is_done;
  } Request;

  typedef struct Object_ {
    int replica[REP_NUM + 1];
    int *unit[REP_NUM + 1];
    int size;
    int last_request_point;
    bool is_delete;
  } Object;

  Request request[MAX_REQUEST_NUM];
  Object object[MAX_OBJECT_NUM];

  int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
  int disk_point[MAX_DISK_NUM];

  static void do_object_delete(const int *object_unit, int *disk_unit, int size) {
    for (int i = 1; i <= size; i++) {
      disk_unit[object_unit[i]] = 0;
    }
  }

  void do_object_write(int *object_unit, int *disk_unit, int size,
                       int object_id) {
    int current_write_point = 0;
    for (int i = 1; i <= V; i++) {
      if (disk_unit[i] == 0) {
        disk_unit[i] = object_id;
        object_unit[++current_write_point] = i;
        if (current_write_point == size) {
          break;
        }
      }
    }

    assert(current_write_point == size);
  }

public:
  void Init_Global_Var() override {

  };

  int Input_Global_Tag_Info() override {
    for (int i = 1; i <= M; i++) {
      for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
        scanf("%*d");
      }
    }

    for (int i = 1; i <= M; i++) {
      for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
        scanf("%*d");
      }
    }

    for (int i = 1; i <= M; i++) {
      for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
        scanf("%*d");
      }
    }

    printf("OK\n");
    fflush(stdout);
    return 0;
  };

  int Init_Global_Tag_Info() override {
    return 0;
  }

  void Init_Disk_Point() override {
    for (int i = 1; i <= N; i++) {
      disk_point[i] = 1;
    }
  };

  void Delete_action() override {
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
      scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++) {
      int id = _id[i];
      int current_id = object[id].last_request_point;
      while (current_id != 0) {
        if (request[current_id].is_done == false) {
          abort_num++;
        }
        current_id = request[current_id].prev_id;
      }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
      int id = _id[i];
      int current_id = object[id].last_request_point;
      while (current_id != 0) {
        if (request[current_id].is_done == false) {
          printf("%d\n", current_id);
        }
        current_id = request[current_id].prev_id;
      }
      for (int j = 1; j <= REP_NUM; j++) {
        do_object_delete(object[id].unit[j], disk[object[id].replica[j]],
                         object[id].size);
      }
      object[id].is_delete = true;
    }

    fflush(stdout);
  }

  void Write_action() override {
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
      int id, size;
      scanf("%d%d%*d", &id, &size);
      object[id].last_request_point = 0;
      for (int j = 1; j <= REP_NUM; j++) {
        object[id].replica[j] = (id + j) % N + 1;
        object[id].unit[j] = static_cast<int *>(malloc(sizeof(int) * (size + 1)));
        object[id].size = size;
        object[id].is_delete = false;
        do_object_write(object[id].unit[j], disk[object[id].replica[j]], size,
                        id);
      }

      printf("%d\n", id);
      for (int j = 1; j <= REP_NUM; j++) {
        printf("%d", object[id].replica[j]);
        for (int k = 1; k <= size; k++) {
          printf(" %d", object[id].unit[j][k]);
        }
        printf("\n");
      }
    }

    fflush(stdout);
  }

  void Read_action() override {
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
      scanf("%d%d", &request_id, &object_id);
      request[request_id].object_id = object_id;
      request[request_id].prev_id = object[object_id].last_request_point;
      object[object_id].last_request_point = request_id;
      request[request_id].is_done = false;
    }

    static int current_request = 0;
    static int current_phase = 0;
    if (!current_request && n_read > 0) {
      current_request = request_id;
    }
    if (!current_request) {
      for (int i = 1; i <= N; i++) {
        printf("#\n");
      }
      printf("0\n");
    } else {
      current_phase++;
      object_id = request[current_request].object_id;
      for (int i = 1; i <= N; i++) {
        if (i == object[object_id].replica[1]) {
          if (current_phase % 2 == 1) {
            printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
          } else {
            printf("r#\n");
          }
        } else {
          printf("#\n");
        }
      }

      if (current_phase == object[object_id].size * 2) {
        if (object[object_id].is_delete) {
          printf("0\n");
        } else {
          printf("1\n%d\n", current_request);
          request[current_request].is_done = true;
        }
        current_request = 0;
        current_phase = 0;
      } else {
        printf("0\n");
      }
    }

    fflush(stdout);
  }

  std::string name() override {
    return "HandlerImpl_sample";
  }
};


#endif//HW_CODECRAFT_2025_HANDLERIMPL_DEFAULT_H
