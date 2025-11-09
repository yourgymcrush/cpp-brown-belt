#include "../lib/test_runner.h"

#include <iostream>
#include <map>
#include <string>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <set>

using namespace std;

struct Record {
  string id;
  string title;
  string user;
  int timestamp;
  int karma;

  bool operator<(const Record& other) const {
    return id < other.id;
  }
};

// Реализуйте этот класс
class Database {
public:
  template <typename Type>
  using HelperIndexes = std::multimap<Type, const Record*>;

  struct IndexInfo {
    HelperIndexes<int>::iterator timestampIndex;
    HelperIndexes<int>::iterator karmaIndex;
    HelperIndexes<std::string>::iterator userIndex;
  };

  struct Data {
    Record record;
    IndexInfo info;
  };

  using ID = std::string;
  using Storage = std::unordered_map<ID, Data>;

  bool Put(const Record& record);
  const Record* GetById(const string& id) const;
  bool Erase(const string& id);

  template <typename Callback>
  void RangeByTimestamp(int low, int high, Callback callback) const;

  template <typename Callback>
  void RangeByKarma(int low, int high, Callback callback) const;

  template <typename Callback>
  void AllByUser(const string& user, Callback callback) const;

private:
  Storage mData;
  HelperIndexes<int> timestampIndexes;
  HelperIndexes<int> karmaIndexes;
  HelperIndexes<std::string> userIndexes;
};

bool Database::Put(const Record& record) {
  auto [it, success] = mData.insert({record.id, {record, {}}});
  
  if (success) {
    auto recordIt = &it->second.record;
    auto karmaIt = karmaIndexes.insert({record.karma, recordIt});
    it->second.info.karmaIndex = karmaIt;
    auto timestampIt = timestampIndexes.insert({record.timestamp, recordIt});
    it->second.info.timestampIndex = timestampIt;
    auto userIt = userIndexes.insert({record.user, recordIt});
    it->second.info.userIndex = userIt;
  }
  return success;
}

bool Database::Erase(const string& id) {
  auto it = mData.find(id);

  if (it == mData.end())
    return 0;
  timestampIndexes.erase(it->second.info.timestampIndex);
  karmaIndexes.erase(it->second.info.karmaIndex);
  userIndexes.erase(it->second.info.userIndex);
  mData.erase(it);
  return 1;
}

const Record* Database::GetById(const string& id) const {
  try {
    return &mData.at(id).record;
  }
  catch (std::exception) {
    return nullptr;
  }
}

template <typename Callback>
void Database::RangeByTimestamp(int low, int high, Callback callback) const {
  auto lower = timestampIndexes.lower_bound(low);
  auto higher = timestampIndexes.upper_bound(high);

  while (lower != higher) {
    if (!callback(*lower->second))
      break;
    lower++;
  }
}

template <typename Callback>
void Database::RangeByKarma(int low, int high, Callback callback) const {
  auto lower = karmaIndexes.lower_bound(low);
  auto higher = karmaIndexes.upper_bound(high);

  while (lower != higher) {
    if (!callback(*lower->second))
      break;
    lower++;
  }
}

template <typename Callback>
void Database::AllByUser(const string& user, Callback callback) const {
  auto lower = userIndexes.lower_bound(user);
  auto higher = userIndexes.upper_bound(user);

  while (lower != higher) {
    if (!callback(*lower->second))
      break;
    lower++;
  }
}

void TestRangeBoundaries() {
  const int good_karma = 1000;
  const int bad_karma = -10;

  Database db;
  db.Put({"id1", "Hello there", "master", 1536107260, good_karma});
  db.Put({"id2", "O>>-<", "general2", 1536107260, bad_karma});
  db.Put({"id3", "Hello there", "master", 1536107260, 20});
  db.Put({"id4", "O>>-<", "general2", 1536107260, -20});
  db.Put({"id5", "Hello there", "master", 1536107260, 30});
  db.Put({"id6", "O>>-<", "general2", 1536107260, 1001});
  db.Put({"id7", "Hello there", "master", 1536107260, 1001});
  db.Erase("id3");
  db.Put({"id8", "O>>-<", "general2", 1536107260, 5});

  int count = 0;
  db.RangeByKarma(bad_karma, good_karma, [&count](const Record& r) {
    ++count;
    return true;
  });

  ASSERT_EQUAL(4, count);
}

void TestSameUser() {
  Database db;
  db.Put({"id1", "Don't sell", "master", 1536107260, 1000});
  db.Put({"id2", "Rethink life", "master", 1536107260, 2000});

  int count = 0;
  db.AllByUser("master", [&count](const Record&) {
    ++count;
    return true;
  });

  ASSERT_EQUAL(2, count);
}

void TestReplacement() {
  const string final_body = "Feeling sad";

  Database db;
  db.Put({"id", "Have a hand", "not-master", 1536107260, 10});
  db.Erase("id");
  db.Put({"id", final_body, "not-master", 1536107260, -10});

  auto record = db.GetById("id");
  ASSERT(record != nullptr);
  ASSERT_EQUAL(final_body, record->title);
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestRangeBoundaries);
  RUN_TEST(tr, TestSameUser);
  RUN_TEST(tr, TestReplacement);
  return 0;
}
