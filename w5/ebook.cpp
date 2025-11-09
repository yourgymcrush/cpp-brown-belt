#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <vector>

using namespace std;

class ReadingManager
{
public:
  ReadingManager() : pageUsers(MAX_PAGE_COUNT_, 0)
  {

  }

  void Read(int user_id, int page_count)
  {
    if(userIdtoPage.count(user_id))
    {
      for(int i = userIdtoPage[user_id]; i < page_count; i++)
      {
        pageUsers[i]++;
      }
    }
    else
    {
      for(int i = 0; i < page_count; i++)
      {
        pageUsers[i]++;
      }
    }
    userIdtoPage[user_id] = page_count;
  }

  double Cheer(int user_id) const
  {
    if(!userIdtoPage.count(user_id))
      return 0;
    if(userIdtoPage.size() == 1)
      return 1;

    double users = double(userIdtoPage.size()) - pageUsers[userIdtoPage.at(user_id) - 1];
    return users / (userIdtoPage.size() - 1);
  }

private:
  static const int MAX_USER_COUNT_ = 100'000;
	static const int MAX_PAGE_COUNT_ = 1000;

  std::unordered_map<int, int> userIdtoPage;
  std::vector<int> pageUsers;
};

int main()
{
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int query_count;
  cin >> query_count;

  ReadingManager manager;

  for (int query_id = 0; query_id < query_count; ++query_id) 
  {
    string query_type;
    cin >> query_type;
    int user_id;
    cin >> user_id;

    if (query_type == "READ")
    {
      int page_count;
      cin >> page_count;
      manager.Read(user_id, page_count);
    }
    else if (query_type == "CHEER")
    {
      cout << setprecision(6) << manager.Cheer(user_id) << "\n";
    }
  }

  return 0;
}