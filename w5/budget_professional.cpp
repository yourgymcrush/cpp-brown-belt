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
    using Container = std::map<int, double>;

    Container m_budget;
    Container m_spendings;

    void Pay(const Date &from, const Date &to, const int percentage)
    {
        int day_from = ComputeDay(from);
        int day_to = ComputeDay(to);
        
        if (day_from > day_to) {
            throw std::invalid_argument("invalid date");
        }
        while (day_from <= day_to) {
            m_budget[day_from] *= 1 - (static_cast<double>(percentage) / 100);
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
            income -= m_spendings[day_from];
            day_from++;
        }
        output << std::setprecision(25) << income << "\n";
    }

    void Spend(const Date &from, const Date &to, const int value)
    {
        ModifyData(from, to, value, &BudgetManager::m_spendings);
    }

    void Earn(const Date &from, const Date &to, const int value)
    {
        ModifyData(from, to, value,  &BudgetManager::m_budget);
    }

    void ModifyData(const Date &from, const Date &to, const int value, Container BudgetManager::* data)
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
            (this->*data)[day_from] += day_value;
            day_from++;
        }
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
            input >> *value;
            Pay(from, to, *value);
        }
        else if (command == "Spend")
        {
            input >> *value;
            Spend(from, to, *value);
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