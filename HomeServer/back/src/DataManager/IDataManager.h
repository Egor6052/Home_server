#pragma once

#include <cstdint>
#include <string>


class IDataManager {

public:
    virtual ~IDataManager() = default;
    virtual bool startDB() = 0;

};