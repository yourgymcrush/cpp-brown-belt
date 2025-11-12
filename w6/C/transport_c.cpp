#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <string_view>
#include <vector>
#include <map>
#include <iomanip>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <charconv>
#include <functional> 

#include "../test_runner.h"

constexpr double P = 3.1415926535;
constexpr int EarthR = 6371;

constexpr std::string_view ERR = "ERROR";
constexpr std::string_view STOP = "Stop";
constexpr std::string_view BUS = "Bus";
constexpr std::string_view WHITESPACE = " ";
constexpr std::string_view LINEAR_DELIM = "-";
constexpr std::string_view CIRCLE_DELIM = ">";
constexpr std::string_view COLON = ":";
constexpr std::string_view COMMA = ",";

template<typename T> class Singleton
{
public:
    static T* get()
    {
        static std::once_flag flag;
        std::call_once(flag, []() { mInstance = new T{}; });
        return mInstance;
    }

private:
    Singleton() {}
    Singleton( const Singleton& ) = delete;
    const Singleton& operator=( const Singleton& ) = delete;

    static T* mInstance;
};

template<typename T> T* Singleton<T>::mInstance = nullptr;

class Trimer final
{
public:
    static std::string_view trim(const std::string_view str)
    {
        return rightTrim(leftTrim(str));
    }
private:
    static std::string_view leftTrim(const std::string_view str)
    {
        size_t start = str.find_first_not_of(WHITESPACE);

        return (start == std::string::npos) ? "" : str.substr(start);
    }
    
    static std::string_view  rightTrim(const std::string_view str)
    {
        size_t end = str.find_last_not_of(WHITESPACE);

        return (end == std::string::npos) ? "" : str.substr(0, end + 1);
    }
};

class Reader final
{
public:
    static std::string_view readToken(std::string_view& s, std::string_view delimiter = WHITESPACE)
    {
        s = Trimer::trim(s);
        const auto [lhs, rhs] = splitTwo(s, delimiter);

        s = rhs;

        return lhs;
    }

    static int convertToInt(std::string_view str)
    {
        int value = 0;
        auto result = std::from_chars(str.data(), str.data() + str.size(), value);

        if (result.ec != std::errc{})
        {
            std::stringstream error;
            error << "string " << str << " contains " << " trailing chars";
            throw std::invalid_argument(error.str());
        }

        return value;
    }

    static double convertToDouble(std::string_view str)
    {
        double value;
        std::string info;
        std::stringstream ss((std::string(str)));

        ss >> value;
        return value;
    }

private:
    static std::pair<std::string_view, std::optional<std::string_view>> splitTwoStrict(std::string_view s, std::string_view delimiter = " ")
    {
        const size_t pos = s.find(delimiter);

        if (pos == s.npos)
        {
            return {s, std::nullopt};
        }
        else
        {
            return {s.substr(0, pos), s.substr(pos + delimiter.length())};
        }
    }

    static std::pair<std::string_view, std::string_view> splitTwo(std::string_view s, std::string_view delimiter = " ")
    {
        const auto [lhs, rhs_opt] = splitTwoStrict(s, delimiter);

        return {lhs, rhs_opt.value_or("")};
    }
};

using Response = std::string;

struct Request;
using RequestHolder = std::unique_ptr<Request>;

class DB;

struct Request {
    enum class Type
    {
        POST,
        GET
    };
    enum class Option
    {
        STOP,
        BUS,
    };

    Request(Type type, Option option) : type(type), option(option)
    {
        //
    }
    static RequestHolder Create(Type type, Option option);
    virtual void ParseFrom(std::string_view input) = 0;
    virtual ~Request() = default;

    const Type type;
    const Option option;
};

struct PostRequest : Request
{
    PostRequest(Option option) : Request(Type::POST, option) {}
    virtual void Process(DB& db) const = 0;
};

struct GetRequest : Request
{
    GetRequest(Option option) : Request(Type::GET, option) {}
    virtual std::string Process(const DB& db) const = 0;
};

struct BusRequest
{
    struct Route
    {
        enum class Type
        {
            LINEAR,
            CIRCLE
        };
        Type type;
        size_t bus_number;
        std::vector<std::string> stops;
    };

    Route route;
};

struct PostBusRequest : PostRequest, BusRequest
{
    PostBusRequest() : PostRequest(Option::BUS), BusRequest() {}
    void ParseFrom(std::string_view input) override
    {
        std::string_view delim;
        
        route.bus_number = Reader::convertToInt(Reader::readToken(input, COLON));
        if (input.find(LINEAR_DELIM) != input.npos)
        {
            delim = LINEAR_DELIM;
            route.type = Route::Type::LINEAR;
        }
        else if (input.find(CIRCLE_DELIM) != input.npos)
        {
            delim = CIRCLE_DELIM;
            route.type = Route::Type::CIRCLE;
        }
        else
        {
            std::stringstream error;
            error << "string " << input << " has wrong format";
            throw std::invalid_argument(error.str());
        }
        route.stops.clear();
        while (input.size())
        {
            auto stop = Trimer::trim(Reader::readToken(input, delim));

            route.stops.push_back(std::string(stop));
        }
        if (route.type == Route::Type::LINEAR)
        {
            std::vector<std::string> stops = route.stops;
            
            stops.reserve(route.stops.size() * 2 - 1);
            for (auto it = route.stops.rbegin() + 1; it != route.stops.rend(); it++)
            {
                stops.push_back(*it);
            }
            route.stops = std::move(stops);
        }
    }

    virtual void Process(DB& db) const override;
};

struct StopRequest
{
    struct Stop
    {
        std::string name;
        std::pair<double, double> coords;
        std::unordered_map<string, size_t> nearbyStops;
    };

    Stop stop;
};

struct PostStopRequest : PostRequest, StopRequest
{
    PostStopRequest() : PostRequest(Option::STOP) {}
    void ParseFrom(std::string_view input) override
    {
        stop.name = Reader::readToken(input, COLON);
        stop.coords.first = Reader::convertToDouble(Reader::readToken(input, COMMA));
        stop.coords.second = Reader::convertToDouble(Reader::readToken(input, COMMA));

        while(!input.empty())
        {
            auto nearbyStopData = Reader::readToken(input, COMMA);
            const auto& distance =  Reader::convertToDouble(Reader::readToken(nearbyStopData, "m"));
            Reader::readToken(nearbyStopData, "to");
            const auto& stopName = std::string(Reader::readToken(nearbyStopData, COMMA));

            stop.nearbyStops[std::string(stopName)] = distance;
        }
    }

    virtual void Process(DB& db) const override;

    Stop stop;
};

struct GetBusRequest : GetRequest, BusRequest
{
    GetBusRequest() : GetRequest(Option::BUS) {}
    void ParseFrom(std::string_view input) override
    {
        route.bus_number = Reader::convertToInt(Reader::readToken(input, COLON));
    }

    virtual std::string Process(const DB& db) const override;
};

struct GetStopRequest : GetRequest, StopRequest
{
    GetStopRequest() : GetRequest(Option::BUS) {}
    void ParseFrom(std::string_view input) override
    {
        stop.name = Reader::readToken(input, COLON);
    }

    virtual std::string Process(const DB& db) const override;
};


const std::unordered_map<std::string_view, Request::Option> STR_TO_REQUEST_OPTION =
{
    {"Bus", Request::Option::BUS},
    {"Stop", Request::Option::STOP}
};

RequestHolder Request::Create(Type type, Option option)
{
    switch (type)
    {
        case Type::POST:
        {
            switch (option)
            {
                case Option::BUS:
                {
                    return std::make_unique<PostBusRequest>();
                }
                case Option::STOP:
                {
                    return std::make_unique<PostStopRequest>();
                }
                default:
                    return nullptr;
            }
        }
        case Type::GET:
        {
            switch (option)
            {
                case Option::BUS:
                {
                    return std::make_unique<GetBusRequest>();
                }
                case Option::STOP:
                {
                    return std::make_unique<GetStopRequest>();
                }
                default:
                    return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

class DB final
{
    using Stop = std::string;
    using StopCoords = std::pair<double, double>;
    using BusNumber = size_t;

    struct StopData
    {   
        std::set<BusNumber> buses;
        StopCoords coords;
    };

    struct Route
    {
        std::vector<Stop> stops;
        std::unordered_set<Stop> unique_stops;
        double LengthGeo = 0;
        double LengthRoad = 0;
        double Curvature = 1;

    };
public:
    DB() = default;
    ~DB() = default;

    void processGetRequests(const std::vector<RequestHolder>& requests,
                            std::vector<std::string>& responses)
    {
        processRequests(requests, responses);
    }

     void processPostRequests(const std::vector<RequestHolder>& requests)
    {
        processRequests(requests);
        updateRoutes();
    }

    void processRequests(const std::vector<RequestHolder>& requests,
                        std::optional<std::reference_wrapper<std::vector<std::string>>> responses = std::nullopt)
    {
        for (const auto& requestHolder : requests)
        {
            if (requestHolder->type == Request::Type::GET)
            {
                const auto& request = static_cast<const GetRequest&>(*requestHolder);
                auto response = request.Process(*this);

                if (responses)
                {
                    responses->get().push_back(response);
                }
            }
            else
            {
                const auto& request = static_cast<const PostRequest&>(*requestHolder);
                request.Process(*this);
            }
        }
    }
    void addBus(const PostBusRequest::Route& route)
    {
        auto& newRoute = routes[route.bus_number];

        newRoute.stops = route.stops;
        for (const auto& stop : newRoute.stops)
        {
            newRoute.unique_stops.insert(stop);
        }
    }
    void addStop(const PostStopRequest::Stop& stop)
    {
        auto& newStop = stops[stop.name];

        newStop.coords.first = toRad(stop.coords.first);
        newStop.coords.second = toRad(stop.coords.second);

        for (const auto& [nearbyStopName, distance] : stop.nearbyStops)
        {
            stopsToNearbyDistances[stop.name][nearbyStopName] = distance;
            if(!stopsToNearbyDistances[nearbyStopName].count(stop.name))
            {   
                stopsToNearbyDistances[nearbyStopName][stop.name] = distance;
            }
        }
    }

    double toRad(double n)
    {
        double res = n * P / 180.0;
        return res;
    }

    std::string getBusData(BusNumber busNumber) const
    {
        auto bus = routes.find(busNumber);
        std::string response = "Bus " + std::to_string(busNumber) + ": ";

        if (bus == routes.end())
        {
            response += "not found";
        }
        else
        {   
            auto& route = bus->second;
            std::stringstream ss;

            ss << setprecision(7);
            ss << response;
            ss << route.stops.size()
                    << " stops on route, "
                    << route.unique_stops.size()
                    << " unique stops, "
                    <<  route.LengthRoad
                    <<  " route length, "
                    << route.Curvature
                    << " curvature";
            response = ss.str();
        }
        return response;
    }

    std::string getStopData(const std::string& stopName) const
    {
        auto stop = stops.find(std::string(stopName));
        std::string response = "Stop " + std::string(stopName) + ": ";

        if (stop == stops.end())
        {
            response += "not found";
        }
        else
        {
            auto& busses = stop->second.buses;
            
            if (busses.empty())
            {
                response += "no buses";
            }
            else
            {
                response += "buses";
                for (const auto& bus : busses)
                {
                    response += " " + std::to_string(bus);
                }
            }
        }

        return response;
    }
private:
    std::unordered_map<Stop, StopData> stops;
    std::unordered_map<Stop, std::unordered_map<Stop, size_t>> stopsToNearbyDistances;
    std::unordered_map<BusNumber, Route> routes;

    double getDistanceBetweenStopsGeo(const Stop& lhs, const Stop& rhs)
    {
        auto& lhsCoords = stops[lhs].coords;
        auto& rhscoords = stops[rhs].coords;
        double res = acos(sin(lhsCoords.first) * sin(rhscoords.first)
                          + cos(lhsCoords.first) * cos(rhscoords.first)
                          * cos(abs(lhsCoords.second - rhscoords.second)))
                     * EarthR * 1000;
        return res;
    }
    double getDistanceBetweenStopsRoad(const Stop& lhs, const Stop& rhs)
    {   
        auto& nearbyStops = stopsToNearbyDistances[lhs];
        
        if(auto it = nearbyStops.find(rhs); it != nearbyStops.end())
        {
            return it->second;
        }
        std::cout << lhs << "->" << rhs << std::endl;
        return getDistanceBetweenStopsGeo(lhs, rhs);
    }

    void updateRoutes()
    {
        for (auto& bus : routes)
        {
            auto& route = bus.second;
            auto& busNumber = bus.first;

            Stop previous = "";
            for (const auto& stop : route.stops)
            {
                stops[stop].buses.insert(busNumber);
                if (!previous.empty()) 
                {
                    route.LengthGeo += getDistanceBetweenStopsGeo(previous, stop);
                    route.LengthRoad += getDistanceBetweenStopsRoad(previous, stop);
                }
                previous = stop;
            }
            route.Curvature = route.LengthRoad / route.LengthGeo;
        }
    }
};

void PostBusRequest::Process(DB& db) const
{
    db.addBus(route);
}

void PostStopRequest::Process(DB& db) const
{
    db.addStop(stop);
}

std::string GetBusRequest::Process(const DB& db) const
{
    return db.getBusData(route.bus_number);
}

std::string GetStopRequest::Process(const DB& db) const
{
    return db.getStopData(std::string(stop.name));
}

class Input final : public Singleton<Input>
{
    friend class Singleton<Input>;
public:
    ~Input() = default;
    std::vector<RequestHolder> readPostRequests(std::istream& in_stream = std::cin)
    {
        return readRequests(Request::Type::POST, in_stream);
    }

    std::vector<RequestHolder> readGetRequests(std::istream& in_stream = std::cin)
    {
        return readRequests(Request::Type::GET, in_stream);
    }
private:
    std::vector<RequestHolder> readRequests(Request::Type type, std::istream& in_stream = std::cin)
    {
        const size_t request_count = readNumberOnLine<size_t>(in_stream);

        std::vector<RequestHolder> requests;
        requests.reserve(request_count);

        for (size_t i = 0; i < request_count; ++i)
        {
            std::string request_str;

            getline(in_stream, request_str);
            if (auto request = parseRequest(request_str, type))
            {
                requests.push_back(std::move(request));
            }
        }

        return requests;
    }

    RequestHolder parseRequest(std::string_view request_str, Request::Type type)
    {
        const auto request_option = convertRequestOptionFromString(Reader::readToken(request_str));
        if (!request_option)
        {
            return nullptr;
        }
        RequestHolder request = Request::Create(type, *request_option);
        if (request)
        {
            request->ParseFrom(request_str);
        };

        return request;
    }

    std::optional<Request::Option> convertRequestOptionFromString(std::string_view option_str)
    {
        if (const auto it = STR_TO_REQUEST_OPTION.find(option_str);
            it != STR_TO_REQUEST_OPTION.end())
        {
            return it->second;
        }
        else
        {
            return std::nullopt;
        }
    }

    template <typename Number>
    Number readNumberOnLine(std::istream& stream)
    {
        Number number;
        std::string dummy;

        stream >> number;
        getline(stream, dummy);

        return number;
    }
};

class Output final : public Singleton<Output>
{
    friend class Singleton<Output>;
public:
    ~Output() = default;
    void printResponses(std::vector<std::string>& responses)
    {
        for (const auto& response : responses)
        {
            printLine(response);
        }
    }
    template <typename Data>
    void printLine(Data data, std::ostream& out = std::cout)
    {
        out << data << std::endl;
    }
};

void testParsingPostBus()
{
    PostBusRequest pb;
    std::string_view str;
    std::vector<std::string> stops;

    //-- 1
    str = "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye";
    Reader::readToken(str);
    pb.ParseFrom(str);
    stops = {"Biryulyovo Zapadnoye", "Biryusinka", "Universam", "Biryulyovo Tovarnaya", "Biryulyovo Passazhirskaya", "Biryulyovo Zapadnoye"};
    

    AssertEqual(pb.route.bus_number, 256, std::string(str));
    AssertEqual(pb.route.stops, stops, std::string(str));
    //-- 2
    str = "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka";
    Reader::readToken(str);
    pb.ParseFrom(str);
    stops = {"Tolstopaltsevo", "Marushkino", "Rasskazovka", "Marushkino", "Tolstopaltsevo"};
    

    AssertEqual(pb.route.bus_number, 750, std::string(str));
    AssertEqual(pb.route.stops, stops, std::string(str));
    //--
}

void testParsingPostStop()
{
    PostStopRequest ps;
    std::string_view str;
    std::pair<double, double> coords;

    //-- 1
    str = "Stop Tolstopaltsevo: 55.611087, 37.20829";
    Reader::readToken(str);
    ps.ParseFrom(str);
    coords =  {55.611087, 37.20829};

    AssertEqual(ps.stop.name, "Tolstopaltsevo", std::string(str));
    AssertEqual(ps.stop.coords, coords, std::string(str));
    //--
}

void testParsingGetBus()
{
    GetBusRequest gs;
    std::string_view str;
    //-- 1
    str = "Bus 750";
    Reader::readToken(str);
    gs.ParseFrom(str);

    AssertEqual(gs.route.bus_number, 750, std::string(str));
    //--
}

void testA()
{
    std::stringstream ss1, ss2;

    DB db;
    std::vector<RequestHolder> requests;
    std::vector<std::string> responses;

    ss1 << "10\n"
        "Stop Tolstopaltsevo: 55.611087, 37.20829\n"
        "Stop Marushkino: 55.595884, 37.209755\n"
        "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
        "Stop Rasskazovka: 55.632761, 37.333324\n"
        "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
        "Stop Biryusinka: 55.581065, 37.64839\n"
        "Stop Universam: 55.587655, 37.645687\n"
        "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
        "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164";
    requests = Input::get()->readPostRequests(ss1);
    db.processPostRequests(requests);
    ss2 << "3\n"
        "Bus 750\n"
        "Bus 256\n"
        "Bus 751";
    requests = Input::get()->readGetRequests(ss2);
    db.processGetRequests(requests, responses);

    std::vector<Response> correct_responses =
    {
        "Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length",
        "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length",
        "Bus 751: not found"
    };

    AssertEqual(responses.size(), correct_responses.size(), "responses count");
    for(int i = 0; i < responses.size(); i++)
    {
        AssertEqual(responses[i], correct_responses[i]);
    }
}

void testB()
{
    std::stringstream ss1, ss2;

    DB db;
    std::vector<RequestHolder> requests;
    std::vector<std::string> responses;

    ss1 << "13\n"
            "Stop Tolstopaltsevo: 55.611087, 37.20829\n"
            "Stop Marushkino: 55.595884, 37.209755\n"
            "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
            "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
            "Stop Rasskazovka: 55.632761, 37.333324\n"
            "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
            "Stop Biryusinka: 55.581065, 37.64839\n"
            "Stop Universam: 55.587655, 37.645687\n"
            "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
            "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164\n"
            "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
            "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
            "Stop Prazhskaya: 55.611678, 37.603831";
    requests = Input::get()->readPostRequests(ss1);
    db.processPostRequests(requests);
    ss2 << "6\n"
            "Bus 256\n"
            "Bus 750\n"
            "Bus 751\n"
            "Stop Samara\n"
            "Stop Prazhskaya\n"
            "Stop Biryulyovo Zapadnoye";
    requests = Input::get()->readGetRequests(ss2);
    db.processGetRequests(requests, responses);

    std::vector<Response> correct_responses =
    {
        "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length",
        "Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length",
        "Bus 751: not found",
        "Stop Samara: not found",
        "Stop Prazhskaya: no buses",
        "Stop Biryulyovo Zapadnoye: buses 256 828"
    };

    AssertEqual(responses.size(), correct_responses.size(), "responses count");
    for(int i = 0; i < responses.size(); i++)
    {
        AssertEqual(responses[i], correct_responses[i], correct_responses[i]);
    }
}

void testC()
{
    std::stringstream ss1, ss2;

    DB db;
    std::vector<RequestHolder> requests;
    std::vector<std::string> responses;

    ss1 << "13\n"
            "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino\n"
            "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka\n"
            "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
            "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
            "Stop Rasskazovka: 55.632761, 37.333324\n"
            "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n"
            "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam\n"
            "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n"
            "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n"
            "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n"
            "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
            "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
            "Stop Prazhskaya: 55.611678, 37.603831";
    requests = Input::get()->readPostRequests(ss1);
    db.processPostRequests(requests);
    ss2 <<  "6\n"
            "Bus 256\n"
            "Bus 750\n"
            "Bus 751\n"
            "Stop Samara\n"
            "Stop Prazhskaya\n"
            "Stop Biryulyovo Zapadnoye";
    requests = Input::get()->readGetRequests(ss2);
    db.processGetRequests(requests, responses);

    std::vector<Response> correct_responses =
    {
        "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.361239 curvature",
        "Bus 750: 5 stops on route, 3 unique stops, 27600 route length, 1.318084 curvature",
        "Bus 751: not found",
        "Stop Samara: not found",
        "Stop Prazhskaya: no buses",
        "Stop Biryulyovo Zapadnoye: buses 256 828"
    };

    AssertEqual(responses.size(), correct_responses.size(), "responses count");
    for(int i = 0; i < responses.size(); i++)
    {
        AssertEqual(responses[i], correct_responses[i]);
    }
}

int main()
{   
    // Testing
    TestRunner tr;

    RUN_TEST(tr, testParsingPostBus);
    RUN_TEST(tr, testParsingPostStop);
    RUN_TEST(tr, testParsingGetBus);
    // RUN_TEST(tr, testA);
    // RUN_TEST(tr, testB);
    RUN_TEST(tr, testC);
    //

    // DB db;
    // std::vector<RequestHolder> requests;
    // std::vector<std::string> responses;

    // requests = Input::get()->readPostRequests();
    // db.processPostRequests(requests);
    // requests = Input::get()->readGetRequests();
    // db.processGetRequests(requests, responses);
    // Output::get()->printResponses(responses);

    return 0;
}
