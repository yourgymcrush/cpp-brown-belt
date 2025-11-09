#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include <optional>
#include <stdexcept>
#include <map>

void LeftStrip(std::string_view& sv)
{
    while (!sv.empty() && isspace(sv[0]))
    {
        sv.remove_prefix(1);
    }
}

std::string_view ReadToken(std::string_view& sv)
{
    LeftStrip(sv);

    auto pos = sv.find('-');
    auto result = sv.substr(0, pos);

    sv.remove_prefix(pos != sv.npos ? pos + 1 : sv.size());

    return result;
}

class Date
{
private:
    int year_ = 1970;
    int month_ = 1;
    int day_ = 1;

    friend std::istream& operator>>(std::istream &input, Date &date) {
        std::string s;
        std::string_view sv;

        input >> s;
        sv = s;
        date.year_ = std::stoi(std::string(ReadToken(sv)));
        date.month_ = std::stoi(std::string(ReadToken(sv)));
        date.day_ = std::stoi(std::string(ReadToken(sv)));

        return input;
    }
    friend bool operator<(const Date &lhs, const Date &rhs) {
        return std::tie(lhs.year_, lhs.month_, lhs.day_) < std::tie(rhs.year_, rhs.month_, rhs.day_);
    }
    friend bool operator<=(const Date &lhs, const Date &rhs) {
        return std::tie(lhs.year_, lhs.month_, lhs.day_) <= std::tie(rhs.year_, rhs.month_, rhs.day_);
    }
    friend bool operator>(const Date &lhs, const Date &rhs) {
        return std::tie(lhs.year_, lhs.month_, lhs.day_) > std::tie(rhs.year_, rhs.month_, rhs.day_);
    }
public:
    Date() = default;
    Date(int year, int month, int day) : 
        year_(year), month_(month), day_(day)
    {
    }

    time_t AsTimestamp() const
    {
        std::tm t;

        t.tm_sec   = 0;
        t.tm_min   = 0;
        t.tm_hour  = 0;
        t.tm_mday  = day_;
        t.tm_mon   = month_ - 1;
        t.tm_year  = year_ - 1900;
        t.tm_isdst = 0;

        return mktime(&t);
    }
};

int ComputeDaysDiff(const Date& date_from, const Date& date_to)
{
    static const int SECONDS_IN_DAY = 60 * 60 * 24;
    const time_t timestamp_to = date_to.AsTimestamp();
    const time_t timestamp_from = date_from.AsTimestamp();
    
    return (timestamp_to - timestamp_from) / SECONDS_IN_DAY;
}

int ComputeDay(const Date& date)
{
    static const int SECONDS_IN_DAY = 60 * 60 * 24;
    const time_t timestamp = date.AsTimestamp();
    const time_t shift_timestamp = Date(1999, 1, 1).AsTimestamp();

    return (timestamp - shift_timestamp) / SECONDS_IN_DAY;
}

class BudgetManager
{
private:
    constexpr static const double TAX = 13;

    std::map<int, double> m_budget;

    void Earn(const Date &from, const Date &to, const int value)
    {
        double day_value = 0;
        int days = 0;
        int day_from = ComputeDay(from);
        int day_to = ComputeDay(to);
        
        if (day_from > day_to) {
            throw std::invalid_argument("invalid date");
        }
        days = ComputeDaysDiff(from, to) + 1;
        day_value = (1.0 * value) / days;
        while (day_from <= day_to) {
            m_budget[day_from] += day_value;
            day_from++;
        }
    }
    void Pay(const Date &from, const Date &to)
    {
        int day_from = ComputeDay(from);
        int day_to = ComputeDay(to);
        
        if (day_from > day_to) {
            throw std::invalid_argument("invalid date");
        }
        while (day_from <= day_to) {
            m_budget[day_from] *= 1 - (TAX / 100);
            day_from++;
        }
    }

    void ComputeIncome(const Date &from, const Date &to, std::ostream &output = std::cout)
    {
        double income = 0;
        int day_from = ComputeDay(from);
        int day_to = ComputeDay(to);
        
        if (day_from > day_to) {
            throw std::invalid_argument("invalid date");
        }
        while (day_from <= day_to) {
            income += m_budget[day_from];
            day_from++;
        }
        output << std::setprecision(25) << income << "\n";
    }
public:
    void ProcessCommand(std::istream &input = std::cin, std::ostream &output = std::cout)
    {
        std::string command;
        Date from, to;
        std::optional<int> value;

        input >> command >> from >> to;
        if (command == "Earn")
        {
            input >> *value;
            Earn(from, to, *value);
        }
        else if (command == "ComputeIncome")
        {
            ComputeIncome(from, to, output);
        }
        else if (command == "PayTax")
        {
            Pay(from, to);
        }
    }
};

int main()
{   BudgetManager manager;
    int query_count = 0;

    std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);
    for (std::cin >> query_count; query_count > 0; --query_count) {
        manager.ProcessCommand(std::cin, std::cout);
    }
}