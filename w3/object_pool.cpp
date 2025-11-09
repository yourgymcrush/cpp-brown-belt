#include "test_runner.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <queue>
#include <stdexcept>
#include <set>
using namespace std;

template <class T>
class ObjectPool {
public:
  T* Allocate() {
    if (free.empty()) {
      return  *allocated.insert(new T()).first;
    }
    return GetFirstFree();
  }
  T* TryAllocate() {
    if (free.empty()) {
      return nullptr;
    }
    return GetFirstFree();
  }

  void Deallocate(T* object) {
    if (allocated.count(object)) {
      free.push_back(std::move(object));
      allocated.erase(object);
    } else {
      throw std::invalid_argument("");
    }
  }

  ~ObjectPool() {
    for (auto ob : allocated) {
      delete ob;
    }
    for (auto ob : free) {
      delete ob;
    }
  }

private:
  T* GetFirstFree() {
    auto object = *allocated.insert(std::move(free.front())).first;

    free.pop_front();
    return object;
  }

  std::set<T*> allocated {};
  std::deque<T*> free {};
};

void TestObjectPool() {
  ObjectPool<string> pool;

  auto p1 = pool.Allocate();
  auto p2 = pool.Allocate();
  auto p3 = pool.Allocate();

  *p1 = "first";
  *p2 = "second";
  *p3 = "third";

  pool.Deallocate(p2);
  ASSERT_EQUAL(*pool.Allocate(), "second");

  pool.Deallocate(p3);
  pool.Deallocate(p1);
  ASSERT_EQUAL(*pool.Allocate(), "third");
  ASSERT_EQUAL(*pool.Allocate(), "first");

  pool.Deallocate(p1);
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestObjectPool);
  return 0;
}
