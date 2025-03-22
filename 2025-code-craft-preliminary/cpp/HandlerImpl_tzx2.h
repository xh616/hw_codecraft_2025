//
// Created by tzx on 25-3-11.
//

#ifndef HW_CODECRAFT_2025_HANDLERIMPL_TZX2_H
#define HW_CODECRAFT_2025_HANDLERIMPL_TZX2_H

#include "IHandler.h"
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <sstream>

// 并发安全的请求队列，为了方便写成全局变量
ConcurrencyQueue<int> tzx2_request_queue;

class HandlerImpl_tzx2 : public IHandler {
private:
  class Request {
  public:
    // 这个请求的时间戳
    int req_timestamp;
    // 这个请求的id
    int id;
    // 这个请求的对象id
    int object_id;
    // 这个请求的对象块数量
    int object_size;
    // 这个请求的前一个请求id
    int prev_id;
    // 这个请求的子请求
    struct SubBlockRequest {
      // 这个子查询的对象块号
      int block_id;
      // 这个对象块在哪块磁盘上
      int disk_id;
      // 这个对象块在磁盘上的位置
      int unit_id;
      // 这个子查询是否完成
      bool is_done{false};
    };
    SubBlockRequest sub_block_requests[MAX_OBJECT_BLOCK_NUM];

    Request() = default;

    Request(int _tempstamp, int _id, int _object_id, int _object_size) : req_timestamp(_tempstamp), id(_id), object_id(_object_id), object_size(_object_size) {
      for (int i = 1; i <= object_size; i++) {
        sub_block_requests[i].block_id = i;
        sub_block_requests[i].disk_id = -1;
        sub_block_requests[i].unit_id = -1;
        sub_block_requests[i].is_done = false;
      }
    }

    auto set_sub_request(int block_id, int disk_id, int unit_id) {
      sub_block_requests[block_id].disk_id = disk_id;
      sub_block_requests[block_id].unit_id = unit_id;
    }

    auto is_done() const {
      for (int i = 1; i <= object_size; i++) {
        if (!sub_block_requests[i].is_done) {
          return false;
        }
      }
      return true;
    }
    auto finish_sub_request(int block_id) {
      sub_block_requests[block_id].is_done = true;
      if (is_done()) {
        // 如果所有的子请求都完成了，就将这个请求放入到请求队列中
        tzx2_request_queue.push(this->id);
      }
    }
  };

  struct Object {
    int id;
    int tag;
    int replica[REP_NUM + 1];
    int *unit[REP_NUM + 1];
    int size;
    int last_request_point;
    bool is_delete;

    auto add_request(Request &request) {
    }
    auto delete_request(Request &request) {
    }
  };

  // Disk
  // 使用线段树维护unit的使用情况，能够快速查询某个范围内的使用情况。
  class Disk {

  public:
    int id;
    //每个硬盘的存储单元数，存储单元编号为1 ~ 𝑉，范围 1 ≤ V ≤ 16384
    int V;
    // G：每个磁头在每个时间片可消耗的令牌数，范围 64 ≤ G ≤ 1000
    int G;
    // g: 当前时间片剩余的token
    int g;
    // pre_token: 磁头上一次动作消耗的令牌数
    int pre_token;
    // jump_times: 磁头跳跃次数
    int jump_times;
    struct DiskUnit {
      int id;
      // object_id: 对象id
      int object_id;
      // block_id: 对象块号
      int block_id;
    } unit[MAX_DISK_SIZE];
    map<int, list<pair<Request *, Request::SubBlockRequest *>>> unitId2Request_SubBlockRequests;
    //    struct subRequest_key {
    //      int obj_size;
    //      int timestamp;
    //      int unit_id;
    //      // 重载小于号
    //      bool operator<(const subRequest_key &other) const {
    //        if (obj_size != other.obj_size) {
    //          return obj_size > other.obj_size;
    //        }
    //        if (timestamp != other.timestamp) {
    //          return timestamp < other.timestamp;
    //        }
    //        return unit_id < other.unit_id;
    //      }
    //    };
    //    set<subRequest_key> subRequest_set;// 按照对象大小和时间戳排序的子请求集合
    int used[MAX_DISK_SIZE];
    // 当前磁头位置，初始值为1
    int point;
    // 当前磁头状态，true表示正在读，false表示不在读
    bool is_reading;

    // 线段树，叶子节点为0表示空闲，1表示已使用。
    struct SegmentTreeNode {
      int used_unit_cnt;// [l, r]区间内使用的unit数量
      int req_cnt;      // [l, r]区间内请求的数量
      SegmentTreeNode operator+(const SegmentTreeNode &other) const {
        return {used_unit_cnt + other.used_unit_cnt, req_cnt + other.req_cnt};
      }
    } seg[4 * MAX_DISK_SIZE];
    // 更新线段树，pos位置的值更新为value。
    void update(int idx, int l, int r, int pos, int used_unit_cnt, int req_cnt) {
      if (l == r) {
        seg[idx] = {used_unit_cnt, req_cnt};
      } else {
        int mid = (l + r) >> 1;
        if (pos <= mid)
          update(idx << 1, l, mid, pos, used_unit_cnt, req_cnt);
        else
          update(idx << 1 | 1, mid + 1, r, pos, used_unit_cnt, req_cnt);
        seg[idx] = seg[idx << 1] + seg[idx << 1 | 1];
      }
    }
    // 更新线段树，pos位置的请求数量加上req_cnt，若果req_cnt为0，则置为0。
    void update_req(int idx, int l, int r, int pos, int req_cnt) {
      if (l == r) {
        if (req_cnt == 0) {
          seg[idx].req_cnt = 0;
        } else {
          seg[idx].req_cnt += req_cnt;
        }
      } else {
        int mid = (l + r) >> 1;
        if (pos <= mid)
          update_req(idx << 1, l, mid, pos, req_cnt);
        else
          update_req(idx << 1 | 1, mid + 1, r, pos, req_cnt);
        seg[idx] = seg[idx << 1] + seg[idx << 1 | 1];
      }
    }
    // Query the sum in [ql, qr].
    SegmentTreeNode query(int idx, int l, int r, int ql, int qr) const {
      if (qr < l || r < ql) return {0, 0};
      if (ql <= l && r <= qr) return seg[idx];
      int mid = (l + r) >> 1;
      return query(idx << 1, l, mid, ql, qr) + query(idx << 1 | 1, mid + 1, r, ql, qr);
    }

    auto use_unit(int unit_id, int object_id, int block_id) {
      assert(used[unit_id] == 0);
      unit[unit_id] = {unit_id, object_id, block_id};
      used[unit_id] = 1;
      // now update segtree: used flag becomes 1.
      update(1, 1, V, unit_id, 1, 0);
    }


  public:
    // Constructor to initialize used[]
    Disk(int _id, int _V, int _G) : point(1), id(_id), V(_V), G(_G) {
      for (int i = 1; i <= V; i++) {
        used[i] = 0;
        unit[i] = {-1, -1};
      }
    }

    // 删除unit_id上存储的对象块状态，删除当前unit挂载的读请求
    auto Free_unit(int unit_id) {
      assert(used[unit_id] == 1);
      unit[unit_id] = {unit_id, -1, -1};
      // 删除unit_id上挂载的读请求
      unitId2Request_SubBlockRequests.erase(unit_id);
      // 删除subRequest_set中unit_id对应的子请求
      //      for (auto it = subRequest_set.begin(); it != subRequest_set.end();) {
      //        if (it->unit_id == unit_id) {
      //          it = subRequest_set.erase(it);
      //        } else {
      //          ++it;
      //        }
      //      }

      used[unit_id] = 0;

      // update segtree: used flag becomes 0.
      update(1, 1, V, unit_id, 0, 0);
    }

    // Return how many used units in the range of len positions starting from pos (circularly).
    auto get_range_used_cnt(int pos, int len) const {
      int total_used = 0;
      if (len >= V) {
        total_used = seg[1].used_unit_cnt;
      } else {
        int end = pos + len - 1;
        if (end <= V) {
          total_used = query(1, 1, V, pos, end).used_unit_cnt;
        } else {
          total_used = query(1, 1, V, pos, V).used_unit_cnt;
          total_used += query(1, 1, V, 1, end - V).used_unit_cnt;
        }
      }
      return total_used;
    }

    // Return how many requests in the range of len positions starting from pos (circularly).
    auto get_range_req_cnt(int pos, int len) const {
      int total_req = 0;
      if (len >= V) {
        total_req = seg[1].req_cnt;
      } else {
        int end = pos + len - 1;
        if (end <= V) {
          total_req = query(1, 1, V, pos, end).req_cnt;
        } else {
          total_req = query(1, 1, V, pos, V).req_cnt;
          total_req += query(1, 1, V, 1, end - V).req_cnt;
        }
      }
      return total_req;
    }

    // Return how many free units in the range of len(default G) positions starting from point.
    auto Get_range_free_cnt(int len = 0) const {
      len = len == 0 ? G : len;
      return min(len, V) - get_range_used_cnt(point, len);
    }

    auto Get_free_unit_cnt() const {
      return V - seg[1].used_unit_cnt;
    }

    // 返回磁头到目标位置的距离
    auto Get_distance(int target_unit) const {
      // 计算距离
      int distance = (target_unit >= point) ? (target_unit - point) : (V - point + target_unit);
      return distance;
    }

    // 在index位置(默认为当前磁头位置point)偏移offset个位置，找到第一个能够容纳object.size个单位的位置，插入的是第num个副本。
    auto _InsertObject_seq(const Object &obj, int num, int index = -1, int offset = 0) {
      index = index == -1 ? point : index;
      int current_point = (index + offset) % V + 1;
      // 从current_point开始找，第一个能够连续容纳object.size个单位的位置。
      while (query(1, 1, V, current_point, current_point + obj.size - 1).used_unit_cnt != 0) {
        current_point = (current_point) % V + 1;
      }
      for (int i = 1; i <= obj.size; i++) {
        int idx = ((current_point - 1) + (i - 1) + V) % V + 1;
        use_unit(idx, obj.id, i);
        obj.unit[num][i] = idx;
      }
    }

    // 在index位置(默认为当前磁头位置point)偏移offset个位置开始，一有空闲位置就插入obj对象的对象块。
    auto _InsertObject_disperse(const Object &obj, int num, int index = -1, int offset = 0) {
      index = index == -1 ? point : index;
      int current_point = (index + offset) % V + 1;
      for (int i = 1; i <= obj.size; i++) {
        int idx = ((current_point - 1) + (i - 1) + V) % V + 1;
        // 找到第一个空闲位置
        // TODO：有可能会死循环！！！
        while (used[idx] == 1) {
          current_point = (current_point) % V + 1;
          idx = ((current_point - 1) + (i - 1) + V) % V + 1;
        }
        use_unit(idx, obj.id, i);
        obj.unit[num][i] = idx;
      }
    }

    // TODO(决策点): 在当前磁盘插入obj对象的第num个副本。插入操作仅需分配空间，不需要移动磁头。
    auto InsertObject(const Object &obj, int num) {
      //            _InsertObject_seq(obj, num, point, (num - 1) * G);
      _InsertObject_disperse(obj, num, point, (num - 1) * 4 * G);
      //      _InsertObject_disperse(obj, num, point, (1.0 * (num - 1) / 3) * V + G);
      //            _InsertObject_disperse(obj, num, 0, (1.0 * (num - 1) / 3) * V);
    }

    // 插入一个子请求，disk_id: 磁盘id，unit_id: 磁盘位置，block_id: 对象块号。
    auto add_sub_block_request(Request &request, int block_id) {
      auto &sub_request = request.sub_block_requests[block_id];
      assert(sub_request.disk_id == id);
      assert(unit[sub_request.unit_id].block_id == sub_request.block_id);
      // 将子请求添加到unitId2SubBlockRequests中
      unitId2Request_SubBlockRequests[sub_request.unit_id].push_back({&request, &sub_request});
      // 将子请求添加到request_set中
      //      subRequest_set.insert({request.object_size, request.req_timestamp, sub_request.unit_id});
      // 更新线段树
      update_req(1, 1, V, sub_request.unit_id, 1);
    }

    // TODO(决策点): 磁头如何工作？返回动作序列，以#结尾。
    auto Work(int timestamp) -> string {
      g = G;
      stringstream ss;

      // 移除过期的请求，过期的请求是指已经过了105个时间片的请求。
      auto update_unitId2Request_SubBlockRequests = [&]() {
        for (auto it = unitId2Request_SubBlockRequests.begin(); it != unitId2Request_SubBlockRequests.end();) {
          list<pair<Request *, Request::SubBlockRequest *>> &Request_SubBlockRequests = it->second;
          for (auto it2 = Request_SubBlockRequests.begin(); it2 != Request_SubBlockRequests.end();) {
            auto &[request, sub_request] = *it2;
            if (timestamp - request->req_timestamp > 105) {
              // 删除过期的请求
              //              request->finish_sub_request(sub_request->block_id);
              it2 = Request_SubBlockRequests.erase(it2);
              // 维护线段树，删除当前unit_id的请求
              update_req(1, 1, V, it->first, -1);
            } else {
              ++it2;
            }
          }
          if (Request_SubBlockRequests.empty()) {
            it = unitId2Request_SubBlockRequests.erase(it);
          } else {
            ++it;
          }
        }
      };
      update_unitId2Request_SubBlockRequests();

      auto next_distance = [&]() {
        if (unitId2Request_SubBlockRequests.empty()) {
          return make_pair(-1, -1);
        }
        // Use binary search (lower_bound) on the sorted map to find the first unit with a read request
        auto it = unitId2Request_SubBlockRequests.lower_bound(point);
        int target_unit = -1;
        if (it != unitId2Request_SubBlockRequests.end()) {
          target_unit = it->first;
        } else {
          // Wrap around: take the first key if none found beyond the current head
          target_unit = unitId2Request_SubBlockRequests.begin()->first;
        }

        // 磁头到下一个有读请求的unit的距离
        return make_pair(target_unit, (target_unit >= point) ? (target_unit - point) : (V - point + target_unit));
      };
      auto [target_unit, distance] = next_distance();
      // 如果当前磁盘没有读请求，直接返回。可以优化
      if (target_unit == -1) {
        ss << '#';
        return ss.str();
      }
#ifdef DEBUG
      print_log("disk: %d ", id);
      print_log("target_unit: %d, point: %d, distance: %d\n", target_unit, point, distance);
#endif

      if (distance >= G) {
        // 下一个有请求的unit在当前磁头位置G个位置之外
        // 将磁头移动到一个更优的位置

        for (int i = 1; i <= V; i++) {
          if (get_range_req_cnt(target_unit, G) < get_range_req_cnt(i, G)) {
            target_unit = i;
          }
        }

        point = target_unit;
        is_reading = false;
        ss << "j " << target_unit;
        jump_times++;
        return ss.str();
      }

      while (g > 0 && distance != -1) {
        if (distance > 0 && g > 0) {
          // 磁头移动到下一个unit,p
          is_reading = false;
          ss << 'p';
          point = point % V + 1;
          g--;
          distance--;
        } else {
          // 已经到达了有读请求的unit
          if (!is_reading && g >= 64) {
            // 磁头上一次动作不是Read
            ss << 'r';
            is_reading = true;
            g -= 64;
            pre_token = 64;
            {
              // 完成当前point下所有的读请求
              for (auto &[request, sub_request]: unitId2Request_SubBlockRequests[point]) {
                request->finish_sub_request(sub_request->block_id);
              }
              unitId2Request_SubBlockRequests.erase(point);
              // 维护线段树，删除当前unit_id的请求
              update_req(1, 1, V, point, 0);
            }
            point = point % V + 1;
            // update distance to next unit_id
            auto [f, s] = next_distance();
            target_unit = f;
            distance = s;
          } else if (is_reading && g >= max(16.0, ceil(pre_token * 0.8))) {
            ss << 'r';
            int need_token = max(16.0, ceil(pre_token * 0.8));
            g -= need_token;
            pre_token = need_token;
            {
              // 完成当前point下所有的读请求
              for (auto &[request, sub_request]: unitId2Request_SubBlockRequests[point]) {
                request->finish_sub_request(sub_request->block_id);
              }
              unitId2Request_SubBlockRequests.erase(point);
              // 维护线段树，删除当前unit_id的请求
              update_req(1, 1, V, point, 0);
            }
            point = point % V + 1;
            // update distance to next unit_id
            auto [f, s] = next_distance();
            target_unit = f;
            distance = s;
          } else {
            // 剩余token不足，磁头不动
            break;
          }
        }
      }

      ss << '#';
#ifdef DEBUG
      print_log("disk %d, point: %d, g: %d\n", id, point, g);
      print_log("%s\n", ss.str().c_str());
#endif
      return ss.str();
    }

    //    auto Work() -> string {
    //      g = G;
    //      stringstream ss;
    //
    //      auto next_distance = [&]() {
    //        if (unitId2Request_SubBlockRequests.empty() || subRequest_set.empty()) {
    //          return make_pair(-1, -1);
    //        }
    //        // 找到set中第一个unit_id
    //        const auto& it = subRequest_set.begin();
    //        int target_unit = it->unit_id;
    //        // 磁头到target_unit的距离
    //        int distance = (target_unit >= point) ? (target_unit - point) : (V - point + target_unit);
    //        return make_pair(target_unit, distance);
    //      };
    //      auto [target_unit, distance] = next_distance();
    //      // 如果当前磁盘没有读请求，直接返回。可以优化
    //      if (target_unit == -1) {
    //        ss << '#';
    //        return ss.str();
    //      }
    //
    //      if (distance >= G) {
    //        // 下一个有请求的unit在当前磁头位置G个位置之外，直接移动磁头到目标位置。
    //        point = target_unit;
    //        is_reading = false;
    //        ss << "j " << target_unit;
    //        return ss.str();
    //      }
    //
    //      while (g > 0 && distance != -1) {
    //#ifdef DEBUG
    //        print_log("disk: %d ", id);
    //        print_log("target_unit: %d, point: %d, distance: %d\n", target_unit, point, distance);
    //#endif
    //        if (distance > 0 && g > 0) {
    //          // 磁头移动到下一个unit,p
    //          is_reading = false;
    //          ss << 'p';
    //          point = point % V + 1;
    //          g--;
    //          distance--;
    //        } else if (distance == 0) {
    //          // 已经到达了有读请求的unit
    //          if (!is_reading && g >= 64) {
    //            // 磁头上一次动作不是Read
    //            ss << 'r';
    //            is_reading = true;
    //            g -= 64;
    //            pre_token = 64;
    //
    //            // 完成当前point下所有的读请求
    //            for (auto &[request, sub_request]: unitId2Request_SubBlockRequests[point]) {
    //              request->finish_sub_request(sub_request->block_id);
    //              subRequest_set.erase({request->object_size, request->req_timestamp, point});
    //            }
    //            unitId2Request_SubBlockRequests.erase(point);
    //            point = point % V + 1;
    //            // update distance to next unit_id
    //            auto [f, s] = next_distance();
    //            target_unit = f;
    //            distance = s;
    //          } else if (is_reading && g >= max(16.0, ceil(pre_token * 0.8))) {
    //            ss << 'r';
    //            int need_token = max(16.0, ceil(pre_token * 0.8));
    //            g -= need_token + 1;
    //            pre_token = need_token;
    //
    //            // 完成当前point下所有的读请求
    //            for (auto &[request, sub_request]: unitId2Request_SubBlockRequests[point]) {
    //              request->finish_sub_request(sub_request->block_id);
    //              subRequest_set.erase({request->object_size, request->req_timestamp, point});
    //            }
    //            unitId2Request_SubBlockRequests.erase(point);
    //            point = point % V + 1;
    //            // update distance to next unit_id
    //            auto [f, s] = next_distance();
    //            target_unit = f;
    //            distance = s;
    //          } else {
    //            // 剩余token不足，磁头不动
    //            break;
    //          }
    //        }
    //      }
    //
    //      ss << '#';
    //#ifdef DEBUG
    //      print_log("disk %d, point: %d, g: %d\n", id, point, g);
    //      print_log("%s\n", ss.str().c_str());
    //#endif
    //      return ss.str();
    //    }

    auto Print() {
      print_log("Disk %d, point: %d, is_reading: %s\n", id, point, is_reading ? "↓" : "↑");
      for (int i = 1; i <= V; i++) {
        if (used[i] == 1) {
          print_log("(%d, %d) ", unit[i].object_id, unit[i].block_id);
        } else {
          print_log("() ");
        }
      }
      print_log("\n");
    }
  };

  Request requests[MAX_REQUEST_NUM];
  Object objects[MAX_OBJECT_NUM];
  unique_ptr<Disk> disks[MAX_DISK_NUM];

  map<int, int> debug_req_times[MAX_OBJECT_BLOCK_NUM];

public:
  void Init_Global_Var() override {
    // 初始化硬盘
    for (int i = 1; i <= N; i++) {
      disks[i] = make_unique<Disk>(i, V, G);
    }
  }


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
  }

  int Init_Global_Tag_Info() override {
    return 0;
  }

  void Init_Disk_Point() override {
  }

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
      int current_id = objects[id].last_request_point;
      while (current_id != 0) {
        if (requests[current_id].is_done() == false) {
          abort_num++;
        }
        current_id = requests[current_id].prev_id;
      }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
      int id = _id[i];
      int current_id = objects[id].last_request_point;
      while (current_id != 0) {
        if (requests[current_id].is_done() == false) {
          objects[id].delete_request(requests[current_id]);
          printf("%d\n", current_id);
        }
        current_id = requests[current_id].prev_id;
      }
      for (int j = 1; j <= REP_NUM; j++) {
        for (int k = 1; k <= objects[id].size; k++) {
          disks[objects[id].replica[j]]->Free_unit(objects[id].unit[j][k]);
        }
      }
      objects[id].is_delete = true;
      //#ifdef DEBUG
      //      print_log("timestamp: %d\n", timestamp);
      //      print_log("deleted object %d, size: %d, tag: %d\n", id, objects[id].size, objects[id].tag);
      //      for (int j = 1; j <= REP_NUM; j++) {
      //        disks[objects[id].replica[j]]->Print();
      //      }
      //#endif
    }

    fflush(stdout);
  }

  void Write_action() override {
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
      int id, size, tag;
      scanf("%d%d%d", &id, &size, &tag);
      objects[id].id = id;
      objects[id].tag = tag;
      objects[id].last_request_point = 0;
      objects[id].size = size;
      objects[id].is_delete = false;

      // TODO(决策点): 选REP_NUM个不同的硬盘，返回一个数组array<int, REP_NUM + 1>，表示选择的硬盘编号。
      auto get_disks_num = [&]() {
        vector<pair<int, int>> disk_free_cnt;
        for (int j = 1; j <= N; j++) {
          //          disk_free_cnt.emplace_back(make_pair(disks[j]->Get_range_free_cnt(), j));// 选择磁头后G个位置空闲最多的磁盘。
          disk_free_cnt.emplace_back(make_pair(disks[j]->Get_free_unit_cnt(), j));// 选择空闲空间最多的磁盘。
        }
        sort(disk_free_cnt.begin(), disk_free_cnt.end(), [&](const pair<int, int> &a, const pair<int, int> &b) {
          return a.first > b.first;
        });
        return array<int, REP_NUM + 1>{0, disk_free_cnt[0].second, disk_free_cnt[1].second, disk_free_cnt[2].second};
      };

      auto disks_num = get_disks_num();

      for (int j = 1; j <= REP_NUM; j++) {
        objects[id].replica[j] = disks_num[j];
        objects[id].unit[j] = static_cast<int *>(malloc(sizeof(int) * (size + 1)));
        disks[disks_num[j]]->InsertObject(objects[id], j);
      }

      printf("%d\n", id);
      for (int j = 1; j <= REP_NUM; j++) {
        printf("%d", objects[id].replica[j]);
        for (int k = 1; k <= size; k++) {
          printf(" %d", objects[id].unit[j][k]);
        }
        printf("\n");
      }

#ifdef DEBUG
      print_log("timestamp: %d\n", timestamp);
      print_log("inserted object %d, size: %d, tag: %d\n", id, size, tag);
      print_log("disks_num: %d %d %d\n", disks_num[1], disks_num[2], disks_num[3]);
//      for (int j = 1; j <= REP_NUM; j++) {
//        disks[disks_num[j]]->Print();
//      }
#endif
    }

    fflush(stdout);
  }

  void Read_action() override {
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    vector<int> curr_requests(n_read + 1);
    // 读入请求，构造Request对象，维护每个对象的last_request_point。
    for (int i = 1; i <= n_read; i++) {
      scanf("%d%d", &request_id, &object_id);
      requests[request_id] = Request(timestamp, request_id, object_id, objects[object_id].size);
      requests[request_id].prev_id = objects[object_id].last_request_point;
      objects[object_id].last_request_point = request_id;
      curr_requests[i] = request_id;
    }

    // TODO(决策点): 对于每一个对象块，构造具体的子请求，选择一个磁盘和一个磁盘位置 && 维护对象之间的关系。
    for (int read_id = 1; read_id <= n_read; read_id++) {
      auto &request = requests[curr_requests[read_id]];
      auto &object = objects[request.object_id];
      for (int _block_id = 1; _block_id <= object.size; _block_id++) {
        // 为第_block_id个对象块选择一个磁盘和磁盘位置。
        int _disk_id, _unit_id;
        // 遍历所有的副本
        array<pair<int, int>, REP_NUM + 1> disk_point_dist;
        for (int k = 1; k <= REP_NUM; k++) {
          // 当前request的查询对象的第k个副本在磁盘disk上的unit_id位置。
          auto &disk = disks[object.replica[k]];
          int unit_id = object.unit[k][_block_id];
          auto dist = disk->Get_distance(unit_id);
          disk_point_dist[k] = {dist, k};
        }
        sort(disk_point_dist.begin() + 1, disk_point_dist.end(), [&](const pair<int, int> &a, const pair<int, int> &b) {
          return a.first < b.first;
        });

        // 选择第一个副本
        _disk_id = object.replica[disk_point_dist[1].second];
        // 选择第一个副本的unit_id
        _unit_id = object.unit[disk_point_dist[1].second][_block_id];
        request.set_sub_request(_block_id, _disk_id, _unit_id);
        // 维护disk
        disks[_disk_id]->add_sub_block_request(request, _block_id);
      }
#ifdef DEBUG
      print_log("timestamp: %d request_id: %d, object_id: %d\n", timestamp, request_id, object.id);
      for (int j = 1; j <= object.size; j++) {
        print_log("block_id: %d, disk_id: %d, unit_id: %d\n", j, request.sub_block_requests[j].disk_id, request.sub_block_requests[j].unit_id);
      }
#endif
    }


    // 将已经构造好的请求与object进行关联
    for (int read_id = 1; read_id <= n_read; read_id++) {
      auto &request = requests[curr_requests[read_id]];
      auto &object = objects[request.object_id];
      object.add_request(request);
    }

    // 对每个磁头进行移动
    for (int disk_id = 1; disk_id <= N; disk_id++) {
      auto &disk = disks[disk_id];
      printf("%s\n", disk->Work(timestamp).c_str());
    }


    // 输出所有完成的请求id
    vector<int> finished_requests;
    while (!tzx2_request_queue.empty()) {
      auto request_id = tzx2_request_queue.pop();
      finished_requests.push_back(request_id);
    }
    printf("%ld\n", finished_requests.size());
    for (const auto request_id: finished_requests) {
      assert(requests[request_id].is_done());
      printf("%d\n", request_id);
      debug_req_times[requests[request_id].object_size][timestamp - requests[request_id].req_timestamp]++;
    }

#ifdef DEBUG
    print_log("timestamp: %d\nDisk head position: ", timestamp);
    for (int i = 1; i <= N; i++) {
      print_log("[%d]%d ", i, disks[i]->point);
    }
    print_log("\n");
#endif

    fflush(stdout);
  }

  void End() override {
    for (int i = 1; i < MAX_OBJECT_BLOCK_NUM; i++) {
      print_log("obj size: %d\n", i);
      int total_time = 0;
      int total_count = 0;
      for (const auto &it: debug_req_times[i]) {
        //        print_log("time: %d, cnt: %d\n", it.first, it.second);
        total_time += it.first * it.second;
        total_count += it.second;
      }
      if (total_count > 0) {
        print_log("count: %d\n", total_count);
        print_log("average time: %.2f\n", static_cast<double>(total_time) / total_count);
      }
    }
    for (int i = 1; i <= N; i++) {
      print_log("disk[%d] jump times: %d\n", i, disks[i]->jump_times);
    }
  }

  std::string name() override {
    return "HandlerImpl_tzx2";
  }
};


#endif//HW_CODECRAFT_2025_HANDLERIMPL_TZX2_H
