#include "../lib/test_runner.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace std;

template <typename T>
class PriorityCollection {
public:

  struct Elem {
    T object;
    int priority;

    bool operator <(const Elem& other) const {
      return priority != other.priority ? priority <= other.priority : true;
    }

    bool operator ==(const Elem& other) const {
      return priority == other.priority;
    }
  };
  
  using Data = std::set<Elem>;
  using Id = int;/* тип, используемый для идентификаторов */;

  // Добавить объект с нулевым приоритетом
  // с помощью перемещения и вернуть его идентификатор
  Id Add(T object);

  // Добавить все элементы диапазона [range_begin, range_end)
  // с помощью перемещения, записав выданные им идентификаторы
  // в диапазон [ids_begin, ...)
  template <typename ObjInputIt, typename IdOutputIt>
  void Add(ObjInputIt range_begin, ObjInputIt range_end,
           IdOutputIt ids_begin);

  // Определить, принадлежит ли идентификатор какому-либо
  // хранящемуся в контейнере объекту
  bool IsValid(Id id) const;

  // Получить объект по идентификатору
  const T& Get(Id id) const;

  // Увеличить приоритет объекта на 1
  void Promote(Id id);

  // Получить объект с максимальным приоритетом и его приоритет
  pair<const T&, int> GetMax() const;

  // Аналогично GetMax, но удаляет элемент из контейнера
  pair<T, int> PopMax();

private:
  // Приватные поля и методы
  Data mData;
  std::map<int, typename Data::iterator> ids;
  int lastId {0};
};

template <typename T>
typename PriorityCollection<T>::Id PriorityCollection<T>::Add(T object) {
  PriorityCollection<T>::Elem elem;

  elem.object = std::move(object);
  elem.priority = 0;
  
  auto it = mData.insert(std::move(elem));
  ids[++lastId] = it.first;
  
  return lastId;
}

template <typename T>
template <typename ObjInputIt, typename IdOutputIt>
void PriorityCollection<T>::Add(ObjInputIt range_begin, ObjInputIt range_end, IdOutputIt ids_begin) {
  for (auto& input = range_begin; input != range_end; input++) {
    ids_begin = Add(*input);
    ids_begin++;
  }
}

template <typename T>
bool  PriorityCollection<T>::IsValid(PriorityCollection<T>::Id id) const {
  return ids.count(id);
}

template <typename T>
void PriorityCollection<T>::Promote(PriorityCollection<T>::Id id) {
  auto it = ids.at(id);
  auto elem = mData.extract(it);

  elem.value().priority++;
  mData.insert(std::move(elem));
}

template <typename T>
pair<const T&, int> PriorityCollection<T>::GetMax() const {
  auto res = --mData.end();

  return {res->object, res->priority};
}

template <typename T>
pair<T, int> PriorityCollection<T>::PopMax() {
  auto res = mData.extract(--mData.end());

  return {std::move(res.value().object), res.value().priority};
}


class StringNonCopyable : public string {
public:
  using string::string;  // Позволяет использовать конструкторы строки
  StringNonCopyable(const StringNonCopyable&) = delete;
  StringNonCopyable(StringNonCopyable&&) = default;
  StringNonCopyable& operator=(const StringNonCopyable&) = delete;
  StringNonCopyable& operator=(StringNonCopyable&&) = default;
};

// void TestNoCopy() {
//   PriorityCollection<StringNonCopyable> strings;
//   const auto white_id = strings.Add("white");
//   const auto yellow_id = strings.Add("yellow");
//   const auto red_id = strings.Add("red");

//   strings.Promote(yellow_id);
//   for (int i = 0; i < 2; ++i) {
//     strings.Promote(red_id);
//   }
//   strings.Promote(yellow_id);
//   {
//     const auto item = strings.PopMax();
//     ASSERT_EQUAL(item.first, "red");
//     ASSERT_EQUAL(item.second, 2);
//   }
//   {
//     const auto item = strings.PopMax();
//     ASSERT_EQUAL(item.first, "yellow");
//     ASSERT_EQUAL(item.second, 2);
//   }
//   {
//     const auto item = strings.PopMax();
//     ASSERT_EQUAL(item.first, "white");
//     ASSERT_EQUAL(item.second, 0);
//   }
// }

// int main() {
//   TestRunner tr;
//   RUN_TEST(tr, TestNoCopy);
//   return 0;
// }
