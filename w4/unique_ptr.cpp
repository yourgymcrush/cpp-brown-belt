#include "test_runner.h"

#include <cstddef>  // нужно для nullptr_t
#include <utility>

using namespace std;


// Реализуйте шаблон класса UniquePtr
template <typename T>
class UniquePtr {
private:
  T* m_data;
public:
  UniquePtr() : m_data(nullptr) {}
  UniquePtr(T * ptr)
  {
    m_data = ptr;
  }
  UniquePtr(const UniquePtr&) = delete;
  UniquePtr(UniquePtr&& other) : m_data(other.m_data)
  {
    other.m_data = nullptr;
  }
  UniquePtr& operator = (const UniquePtr&) = delete;
  UniquePtr& operator = (nullptr_t)
  {
    Reset(nullptr);
    return *this;
  }
  UniquePtr& operator = (UniquePtr&& other)
  {
    if (this != &other)
    {
      Reset(other.m_data);
      other.m_data = nullptr;
    }
    return *this;
  }
  ~UniquePtr()
  {
    delete m_data;
  }

  T& operator * () const
  {
    return *m_data;
  }

  T * operator -> () const
  {
    return m_data;
  }

  T * Release()
  {
    T* res = m_data;
    m_data = nullptr;
    return res;
  }

  void Reset(T * ptr)
  {
    delete m_data;
    m_data = ptr;
  }

  void Swap(UniquePtr& other)
  {
    std::swap(m_data, other.m_data);
  }

  T * Get() const
  {
    return m_data;
  }
};


struct Item {
  static int counter;
  int value;
  Item(int v = 0): value(v) {
    ++counter;
  }
  Item(const Item& other): value(other.value) {
    ++counter;
  }
  ~Item() {
    --counter;
  }
};

int Item::counter = 0;


void TestLifetime() {
  Item::counter = 0;
  {
    UniquePtr<Item> ptr(new Item);
    ASSERT_EQUAL(Item::counter, 1);

    ptr.Reset(new Item);
    ASSERT_EQUAL(Item::counter, 1);
  }
  ASSERT_EQUAL(Item::counter, 0);

  {
    UniquePtr<Item> ptr(new Item);
    ASSERT_EQUAL(Item::counter, 1);

    auto rawPtr = ptr.Release();
    ASSERT_EQUAL(Item::counter, 1);

    delete rawPtr;
    ASSERT_EQUAL(Item::counter, 0);
  }
  ASSERT_EQUAL(Item::counter, 0);
}

void TestGetters() {
  UniquePtr<Item> ptr(new Item(42));
  ASSERT_EQUAL(ptr.Get()->value, 42);
  ASSERT_EQUAL((*ptr).value, 42);
  ASSERT_EQUAL(ptr->value, 42);
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestLifetime);
  RUN_TEST(tr, TestGetters);
}
