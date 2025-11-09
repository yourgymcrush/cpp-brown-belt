#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <optional>

using namespace std;

template <typename Iterator>
class IteratorRange {
public:
  IteratorRange(Iterator begin, Iterator end)
    : first(begin)
    , last(end)
  {
  }

  Iterator begin() const {
    return first;
  }

  Iterator end() const {
    return last;
  }

private:
  Iterator first, last;
};

template <typename Collection>
auto Head(Collection& v, size_t top) {
  return IteratorRange{v.begin(), next(v.begin(), min(top, v.size()))};
}

struct Person {
  string name;
  int age, income;
  bool is_male;
};

vector<Person> ReadPeople(istream& input) {
  int count;
  input >> count;

  vector<Person> result(count);
  for (Person& p : result) {
    char gender;
    input >> p.name >> p.age >> p.income >> gender;
    p.is_male = gender == 'M';
  }

  return result;
}

template <typename Iterator>
std::optional<std::string> findPopular(const IteratorRange<Iterator> &range) {
  if(range.begin() == range.end()) {
    return std::nullopt;
  }
  sort(range.begin(), range.end(), [](const Person &lhs, const Person &rhs) {
    return lhs.name < rhs.name;
  });
  const string* name = &range.begin()->name;
  int count = 1;
  for (auto it = range.begin(); it != range.end();) {
    auto nameEnd = find_if_not(it, range.end(), [&](const Person &person) {
      return person.name == it->name;
    });
    int nameCount = distance(it, nameEnd);
    if (nameCount > count) {
      count = nameCount;
      name = &it->name;
    }
    it = nameEnd;
  }
  return *name;
}

struct Stats {
  vector<Person> sorted_by_age;
  std::optional<std::string> male_name;
  std::optional<std::string> female_name;
  vector<int> wealth;
};

Stats BuildStats(vector<Person> people) {
  Stats stats;

// POPULAR_NAME ------------------------------
  auto it = partition(people.begin(), people.end(), [](const Person &p) {
      return p.is_male;
  });
  IteratorRange males (people.begin(), it);
  IteratorRange females(males.end(), people.end());
  stats.male_name = findPopular(males);
  stats.female_name = findPopular(females);
// WEALTH -------------------------------------
  sort(begin(people), end(people), [](const Person& lhs, const Person& rhs) {
    return lhs.income > rhs.income;
  });

  stats.wealth.resize(people.size());
  int i = 0;
  int totalIncome = 0;
  for(const auto &person : people) {
    totalIncome += person.income;
    stats.wealth[i++] = totalIncome;
  }
// AGE -----------------------------------------
  sort(begin(people), end(people), [](const Person& lhs, const Person& rhs) {
    return lhs.age < rhs.age;
  });
  stats.sorted_by_age = std::move(people);
// ---------------------------------------------
  return stats;
}

int main() {
  const Stats stats = BuildStats(ReadPeople(cin));

  for (string command; cin >> command; ) {
    if (command == "AGE") {
      int adult_age;
      cin >> adult_age;

      auto adult_begin = lower_bound(
        begin(stats.sorted_by_age), end(stats.sorted_by_age), adult_age, [](const Person& lhs, int age) {
          return lhs.age < age;
        }
      );

      cout << "There are " << std::distance(adult_begin, end(stats.sorted_by_age))
           << " adult people for maturity age " << adult_age << '\n';
    } else if (command == "WEALTHY") {
      int count;
      cin >> count;
      
      cout << "Top-" << count << " people have total income " << stats.wealth[count - 1] << '\n';
    } else if (command == "POPULAR_NAME") {
      char gender;
      cin >> gender;

      auto name = gender == 'M' ? stats.male_name : stats.female_name;
      
      if (!name) {
        cout << "No people of gender " << gender << '\n';
      } else {
        cout << "Most popular name among people of gender " << gender << " is "
              << *name << '\n';
      }
    }
  }
}
