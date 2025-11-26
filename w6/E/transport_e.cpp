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
#include <fstream> 

#include "../test_runner.h"
#include "json.h"
#include "router.h"
#include "graph.h"

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

struct Settings
{
    size_t bus_wait_time;
    double bus_velocity;
};

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
        ROUTE
    };

    Request(Type type, Option option) : type(type), option(option)
    {
        //
    }
    static RequestHolder Create(Type type, Option option);
    virtual void ParseFromJson(const Json::Node& node) = 0;
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
    virtual Json::Node Process(const DB& db) const = 0;

    virtual void GetId(const Json::Node& node)
    {   
        auto data = node.AsMap();
        id = data.at("id").AsInt();
    }
    size_t id {0};
};

struct RouteRequest
{
    std::string from = "";
    std::string to = "";
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
    virtual void ParseFromJson(const Json::Node& node) override
    {
        auto data = node.AsMap();
        route.bus_number = std::stoi(data.at("name").AsString());
        route.type = data.at("is_roundtrip").AsBool() ? Route::Type::CIRCLE : Route::Type::LINEAR;

        auto stops = data.at("stops").AsArray();

        for (auto& stop : stops)
        {
            route.stops.push_back(stop.AsString());
        }
        if (route.type == Route::Type::LINEAR)
        {
            AddBackRoute();
        }
    }

    void AddBackRoute()
    {
        std::vector<std::string> stops = route.stops;
        
        stops.reserve(route.stops.size() * 2 - 1);
        for (auto it = route.stops.rbegin() + 1; it != route.stops.rend(); it++)
        {
            stops.push_back(*it);
        }
        route.stops = std::move(stops);
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

    virtual void ParseFromJson(const Json::Node& node) override
    {
        auto data = node.AsMap();
        stop.name = data.at("name").AsString();
        stop.coords.first = data.at("longitude").AsDouble();
        stop.coords.second = data.at("latitude").AsDouble();

        auto nearbyStopData = data.at("road_distances").AsMap();
        for (const auto& [stopName, distance] : nearbyStopData)
        {
            stop.nearbyStops[stopName] = distance.AsInt();
        }
    }

    virtual void Process(DB& db) const override;

    Stop stop;
};

struct GetBusRequest : GetRequest, BusRequest
{
    GetBusRequest() : GetRequest(Option::BUS) {}

    virtual void ParseFromJson(const Json::Node& node) override
    {   
        GetId(node);
        auto data = node.AsMap();
        route.bus_number = std::stoi(data.at("name").AsString());
    }

    virtual Json::Node Process(const DB& db) const override;
};

struct GetStopRequest : GetRequest, StopRequest
{
    GetStopRequest() : GetRequest(Option::BUS) {}
    
    virtual void ParseFromJson(const Json::Node& node) override
    {
        GetId(node);
        auto data = node.AsMap();
        stop.name = data.at("name").AsString();
    }

    virtual Json::Node Process(const DB& db) const override;
};

struct GetRouteRequest : GetRequest, RouteRequest
{
    GetRouteRequest() : GetRequest(Option::BUS) {}
    virtual void ParseFromJson(const Json::Node& node) override
    {
        GetId(node);
        auto data = node.AsMap();
        from = data.at("from").AsString();
        to = data.at("to").AsString();
    }
    virtual Json::Node Process(const DB& db) const override;
};


const std::unordered_map<std::string_view, Request::Option> STR_TO_REQUEST_OPTION =
{
    {"Bus", Request::Option::BUS},
    {"Stop", Request::Option::STOP},
    {"Route", Request::Option::ROUTE}
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
                case Option::ROUTE:
                {
                    return std::make_unique<GetRouteRequest>();
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
    using Id = Graph::VertexId;

    struct StopData
    {   
        std::set<BusNumber> buses;
        StopCoords coords;
        Id id;
    };

    struct Route
    {
        enum class Type
        {
            LINEAR,
            CIRCLE
        };
        std::vector<Stop> stops;
        std::unordered_set<Stop> unique_stops;
        double LengthGeo = 0;
        double LengthRoad = 0;
        double Curvature = 1;
        Type type;

    };
public:
    DB() = default;
    ~DB() = default;

    void setSettings(Settings&& settings)
    {
        routingSettings = settings;
    }
    
    void processGetRequests(const std::vector<RequestHolder>& requests,
                            std::optional<std::reference_wrapper<Json::Node>> responses = std::nullopt)
    {
        processRequests(requests, responses);
    }

    void processPostRequests(const std::vector<RequestHolder>& requests)
    {
        processRequests(requests);
        updateRoutes();
        buildGraph();
    }

    void processRequests(const std::vector<RequestHolder>& requests,
                        std::optional<std::reference_wrapper<Json::Node>> responses = std::nullopt)
    {
        for (const auto& requestHolder : requests)
        {
            if (requestHolder->type == Request::Type::GET)
            {
                const auto& request = static_cast<const GetRequest&>(*requestHolder);
                auto response = request.Process(*this);

                if (responses)
                {
                    responses->get().AsArray().push_back(response);
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

        newRoute.type = static_cast<Route::Type> (route.type);
        newRoute.stops = route.stops;
        for (const auto& stop : newRoute.stops)
        {
            newRoute.unique_stops.insert(stop);
        }
    }
    void addStop(const PostStopRequest::Stop& stop)
    {
        auto& newStop = stops[stop.name];

        newStop.id = nextId;
        stopNames[nextId] = stop.name;
        nextId += 2;
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

    Json::Node getBusData(BusNumber busNumber) const
    {
        auto bus = routes.find(busNumber);
        std::map<std::string, Json::Node> response;

        if (bus == routes.end())
        {
            response["error_message"] = "not found";
        }
        else
        {   
            auto& route = bus->second;

            response["curvature"] =  route.Curvature;
            response["route_length"] = route.LengthRoad;
            response["stop_count"] = static_cast<int>(route.stops.size());
            response["unique_stop_count"] = static_cast<int>(route.unique_stops.size());
        }
        return response;
    }

    Json::Node getStopData(const std::string& stopName) const
    {
        auto stop = stops.find(std::string(stopName));
        std::map<std::string, Json::Node> response;

        if (stop == stops.end())
        {
            response["error_message"] = "not found";
        }
        else
        {
            std::vector<Json::Node> buses;
            for (const auto& bus : stop->second.buses)
            {
                buses.push_back(std::to_string(bus));
            }
            response["buses"] = buses;
        }

        return response;
    }

    Json::Node getRoute(const std::string& stopNameFrom, const std::string& stopNameTo) const
    {
        std::map<std::string, Json::Node> response;
        std::vector<Json::Node> items;
        double totalTime = 0;
        auto info = router->BuildRoute(stops.at(stopNameFrom).id, stops.at(stopNameTo).id);
        
        if (!info)
        {
            response["error_message"] = "not found";
        }
        else
        {
            totalTime = info->weight;

            for (size_t i = 0; i < info->edge_count; i++)
            {
                std::map<std::string, Json::Node> item;
                auto edgeId = router->GetRouteEdge(info->id, i);
                auto edge = graph.GetEdge(edgeId);
                auto start = edge.from;
                auto finish = edge.to;
                auto time = edge.weight;

                if (start % 2 == 0)
                {
                    item["type"] = std::string("Wait");
                    item["stop_name"] = stopNames.at(start);
                }
                else
                {
                    Stop first = stopNames.at(start - 1);
                    Stop second = stopNames.at(finish);

                    for (const auto& [bus, routeInfo] : routes)
                    {
                        auto firstIt = std::find(routeInfo.stops.begin(), routeInfo.stops.end(), first);
                        auto secondIt = std::find(routeInfo.stops.begin(), routeInfo.stops.end(), second);

                        if (firstIt != routeInfo.stops.end() && secondIt != routeInfo.stops.end())
                        {
                            int span_coutn = routeInfo.stops.size();
                            auto& stops = routeInfo.stops;

                            auto firstIt = std::find(stops.begin(), stops.end(), first);
                            auto secondIt = std::find(firstIt, stops.end(), second);
                            
                            span_coutn = std::min(span_coutn, static_cast<int>(secondIt - firstIt));

                            item["type"] = std::string("Bus");
                            item["span_count"] = span_coutn;
                            item["bus"] = std::to_string(bus);
                        }
                    }
                }
                item["time"] = time;
                items.push_back(item);
            }

            response["total_time"] = totalTime;
            response["items"] = items;
            router->ReleaseRoute(info->id);
        }

        return response;
    }
private:
    Settings routingSettings;
    std::unordered_map<Stop, StopData> stops;
    std::unordered_map<Id, Stop> stopNames;
    Id nextId {0};
    std::unordered_map<Stop, std::unordered_map<Stop, size_t>> stopsToNearbyDistances;
    std::unordered_map<BusNumber, Route> routes;

    Graph::DirectedWeightedGraph<double> graph {0};
    std::unique_ptr<Graph::Router<double>> router {nullptr};

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

    void buildGraph()
    {
        Graph::DirectedWeightedGraph<double> newGraph (stops.size() * 2);

        for(const auto& [stop, info] : stops)
        {
            newGraph.AddEdge({.from = info.id,
                            .to = info.id + 1,
                            .weight = routingSettings.bus_wait_time * 1.0});
        }

        for(const auto& [bus, route] : routes)
        {
            buildRouteInGraph(route.stops.begin(), route.stops.end(), newGraph);
            if (route.type == Route::Type::CIRCLE)
            {
                buildCircleRouteInGraph(route.stops.rbegin(), route.stops.rend(), newGraph);
            }
        }

        graph = newGraph;
        router = std::make_unique<Graph::Router<double>>(graph);
    }

    template<typename Iterator>
    void buildRouteInGraph(Iterator start, Iterator end, Graph::DirectedWeightedGraph<double> &graph)
    {
        while (start != end)
        {
            double weight = 0;

            for (auto next = start + 1; next != end; next++)
            {
                weight += (stopsToNearbyDistances[*(next - 1)][*next] / 1000.0 / routingSettings.bus_velocity) * 60.0;
                graph.AddEdge({.from = stops[*start].id + 1,
                               .to = stops[*next].id,
                               .weight = weight});
            }
            start++;
        }
    }

    template<typename Iterator>
    void buildCircleRouteInGraph(Iterator start, Iterator end, Graph::DirectedWeightedGraph<double> &graph)
    {
        double weight = (stopsToNearbyDistances[*start][*(end - 1)] / 1000.0 / routingSettings.bus_velocity) * 60.0;

        while (start != end)
        {
            auto next = start + 1;

            if(next != end)
            {
                graph.AddEdge({.from = stops[*start].id + 1,
                               .to = stops[*(end - 1)].id,
                               .weight = weight});
                weight += (stopsToNearbyDistances[*next][*start] / 1000.0 / routingSettings.bus_velocity) * 60.0;
            }
            start++;
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

Json::Node GetBusRequest::Process(const DB& db) const
{
    auto response = db.getBusData(route.bus_number);

    response.AsMap()["request_id"] = static_cast<int>(id);

    return response;
}

Json::Node GetStopRequest::Process(const DB& db) const
{
    auto response =  db.getStopData(stop.name);

    response.AsMap()["request_id"] = static_cast<int>(id);

    return response;
}

Json::Node GetRouteRequest::Process(const DB& db) const
{
    auto response =  db.getRoute(from, to);

    response.AsMap()["request_id"] = static_cast<int>(id);

    return response;
}


class Input final : public Singleton<Input>
{
    friend class Singleton<Input>;
public:
    ~Input() = default;
    std::tuple<Settings, std::vector<RequestHolder>, std::vector<RequestHolder>> readRequests(const Json::Document& json)
    {
        std::vector<RequestHolder> postRequests;
        std::vector<RequestHolder> getRequests;
        Settings settings;

        auto routingSettings = json.GetRoot().AsMap().at("routing_settings");
        auto baseRequests = json.GetRoot().AsMap().at("base_requests");
        auto statRequests = json.GetRoot().AsMap().at("stat_requests");

        {
            settings.bus_velocity = routingSettings.AsMap().at("bus_velocity").AsInt();
            settings.bus_velocity = routingSettings.AsMap().at("bus_velocity").AsInt();
        }
        for (const auto &requestJson : baseRequests.AsArray())
        {
            auto request = parseRequestJson(requestJson, Request::Type::POST);
            postRequests.push_back(std::move(request));
        }
        for (const auto &requestJson : statRequests.AsArray())
        {
            auto request = parseRequestJson(requestJson, Request::Type::GET);
            getRequests.push_back(std::move(request));
        }

        return std::make_tuple(std::move(settings), std::move(postRequests), std::move(getRequests));
    }
private:
    RequestHolder parseRequestJson(const Json::Node& node, Request::Type type)
    {
        const auto request_option = convertRequestOptionFromString(node.AsMap().at("type").AsString());
        if (!request_option)
        {
            return nullptr;
        }
        RequestHolder request = Request::Create(type, *request_option);
        if (request)
        {
            request->ParseFromJson(node);
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

class FileReader {
public:
    enum class Option
    {
        OPEN,
        TRUNCATE
    };

    FileReader(const std::string& fileName, std::optional<Option> option = Option::OPEN) : fileName(fileName)
    {
       inputFile.open(fileName, option == Option::TRUNCATE ? std::ios::out | std::ios::trunc : std::ios::in);

        if (!inputFile.is_open())
        {
            throw(std::invalid_argument("could not open file " + fileName));
        }
    }

    FileReader(char* fileName) : FileReader(std::string(fileName)) {}

    ~FileReader()
    {
        if (inputFile.is_open())
        {
            inputFile.close();
        }
    }

    void clear()
    {
        inputFile.close();
        inputFile.open(fileName, std::ios::trunc);
    }

    Json::Document Load()
    {
        return Json::Load(inputFile);
    }

    void Write(const Json::Document& doc)
    {
        printNode(doc.GetRoot());
    }

    void printArray(const Json::Node &node) {
        inputFile << '[';
        bool first = true;
        for (auto &item : node.AsArray()) {
            if (first) {
                first = false;
            } else {
                inputFile << ",";
            }
            printNode(item);
        }
        inputFile << ']';
    }

    void printInt(const Json::Node &node) {
        inputFile << node.AsInt();
    }

    void printString(const Json::Node &node) {
        inputFile << "\"" << node.AsString() << "\"";
    }

    void printDouble(const Json::Node &node) {
        inputFile << node.AsDouble();
    }

    void printMap(const Json::Node &node) {
        inputFile << '{';
        bool first = true;
        for (auto &[key, value] : node.AsMap()) {
            if (first) {
                first = false;
            } else {
                inputFile << ",";
            }
            inputFile << "\"" << key << "\"" << ": ";
            printNode(value);
        }
        inputFile << '}';
    }

    void printNode(const Json::Node &node) {
        switch (node.getType()) {
        case Json::Node::Type::ARRAY:
            printArray(node);
            break;
        case Json::Node::Type::MAP:
            printMap(node);
            break;
        case Json::Node::Type::INT:
            printInt(node);
            break;
        case Json::Node::Type::STRING:
            printString(node);
            break;
        case Json::Node::Type::DOUBLE:
            printDouble(node);
            break;
        default:
            break;
        }
    }
private:
    std::string fileName;
    std::fstream inputFile;
};

void testE()
{
    FileReader request_file("requests.txt");
    FileReader correct_response_file("correct_responses.txt");
    FileReader response_file("responses.txt", FileReader::Option::TRUNCATE);

    auto requests = request_file.Load();
    auto correct_responses = correct_response_file.Load();
    auto responses = Json::Node();

    DB db;
    auto [routing_settings, postRequests, getRequests] = Input::get()->readRequests(requests);

    db.setSettings(std::move(routing_settings));
    db.processPostRequests(postRequests);
    db.processGetRequests(getRequests, responses);

    response_file.Write(Json::Document(responses));
}


int main()
{   
    // Testing
    TestRunner tr;

    RUN_TEST(tr, testE);
    //

    return 0;
}
