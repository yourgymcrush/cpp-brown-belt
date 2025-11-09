#include "ini.h"
#include <iostream>

namespace Ini {

Section& Document::AddSection(std::string name) {
    return sections[name];
}

const Section& Document::GetSection(const std::string& name) const {
    auto it = sections.find(name);

    if(it == sections.end())
        throw std::out_of_range("Unknown section");
    return it->second;
}

size_t Document::SectionCount() const {
    return sections.size();
}

Document Load(std::istream& input) {
    Document document;
    Section* section;
    std::string line;
    
    while (getline(input, line)) {
        if (line.empty())
            continue;
        if (line.length() >= 3 && line.front() == '[' && line.back() == ']') {
            std::string sectionName = line.substr(1, line.length() - 2);
            section = &document.AddSection(sectionName);
        }
        else {
            auto pos = line.find('=');
            section->insert({line.substr(0, pos), line.substr(pos + 1)});
        }
    }

    return document;
}
}
