#include "json.h"

using namespace std;

namespace Json {

Document::Document(Node root) : root(move(root))
{
}

const Node& Document::GetRoot() const
{
    return root;
}

Node LoadNode(istream& input);

Node LoadArray(istream& input)
{
    vector<Node> result;

    for (char c; input >> c && c != ']'; )
    {
        if (c != ',')
        {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    return Node(move(result));
}

Node LoadString(istream& input)
{
    string line;
    getline(input, line, '"');
    return Node(move(line));
}

Node LoadDict(istream& input)
{
    map<string, Node> result;

    for (char c; input >> c && c != '}'; )
    {
        if (c == ',')
        {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
}

Node LoadBool(istream& input)
{
    std::string line;
    while (isalpha(input.peek()))
    {
        line += input.get();
    }
    
    bool res = line == "true" ? true : false;
    return Node(res);
}

Node LoadDouble(istream& input, int integer)
{
    double result = 0.0;
    uint64_t fraction = 0;
    uint64_t order = 1;

    while (isdigit(input.peek()))
    {
        fraction *= 10;
        fraction += input.get() - '0';
        order *= 10;
    }
    result = double(fraction) / double(order);
    result += integer;

    return Node(result);
}

Node LoadInt(istream& input) {
    int sign = 1;
    if (input.peek() == '-')
    {
        input.get();
        sign = -1;
    }
    int result = 0;
    while (isdigit(input.peek()))
    {
        result *= 10;
        result += input.get() - '0';
    }
    result *= sign;
    if (input.peek() == '.')
    {
        input.get();
        return LoadDouble(input, result);
    }
    return Node(result);
}

Node LoadType(istream& input)
{
    if (std::isalpha(input.peek()))
    {
        return LoadBool(input);
    }
    else
    {
        return LoadInt(input);
    }
}

Node LoadNode(istream& input)
{
    char c;
    input >> c;

    if (c == '[')
    {
        return LoadArray(input);
    }
    else if (c == '{')
    {
        return LoadDict(input);
    }
    else if (c == '"')
    {
        return LoadString(input);
    }
    else
    {
        input.putback(c);
        return LoadType(input);
    }
}

Document Load(istream& input)
{
    return Document{LoadNode(input)};
}

}
