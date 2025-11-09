#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <algorithm>

using namespace std;

class RouteManager
{
public:
  void AddRoute(int start, int finish)
  {
    routes[start].insert(finish);
    routes[finish].insert(start);
  }

  int FindNearestFinish(int start, int finish)
  {
    int res = abs(start - finish);
    auto startIt = routes.find(start);

    if(startIt == routes.end())
      return res;
    auto lowerIt = startIt->second.lower_bound(finish);
    if(lowerIt != startIt->second.end())
      res = min(res, abs(finish - *lowerIt));
    if(lowerIt != startIt->second.begin())
      res = min(res, abs(finish - *prev(lowerIt)));

    return res;
  }

private:
  std::unordered_map<int, std::set<int>> routes;
};


int main() {
  RouteManager routes;

  int query_count;
  cin >> query_count;

  for (int query_id = 0; query_id < query_count; ++query_id) {
    string query_type;
    cin >> query_type;
    int start, finish;
    cin >> start >> finish;
    if (query_type == "ADD") {
      routes.AddRoute(start, finish);
    } else if (query_type == "GO") {
      cout << routes.FindNearestFinish(start, finish) << "\n";
    }
  }

  return 0;
}