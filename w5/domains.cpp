#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <string_view>

using namespace std;

class Filter
{
public:
  void ReadBannedDomains(std::istream& input = cin)
  {
    int count;

    for(input >> count; count > 0; count--)
    {
      std::string domain;

      input >> domain;
      banned.insert(domain);
    }
  }

  void CeckDomains(std::istream& input = std::cin, std::ostream& output = std::cout)
  {
    int count;

    for(input >> count; count > 0; count--)
    {
      std::string domain;

      input >> domain;
      if(IsBadDomain(domain))
      {
        output << "Bad\n";
      }
      else
      {
        output << "Good\n";
      }
    }
  }

  bool IsBadDomain(std::string_view domain)
  {
    while(true)
    {
      size_t dot = domain.find('.');
      std::string subdomain {domain.begin(), domain.end()};

      if(banned.count(subdomain))
      {
        return 1;
      }
      if(dot = domain.npos)
      {
        break;
      }
      else
      {
        domain.remove_prefix(dot + 1);
      }

    }
    return 0;
  }
private:
  std::unordered_set<std::string> banned;
};


int main() {
  Filter filter;

  ios::sync_with_stdio(false);
	cin.tie(nullptr);

  filter.ReadBannedDomains(std::cin);
  filter.CeckDomains(std::cin, std::cout);
}