#pragma once
#include "Feature.hpp"

#include "Command.hpp"
#include "Utils.hpp"

class EntityList : public Feature {
public:
    EntityList();
    CEntInfo* GetEntityInfoByIndex(int index);
    CEntInfo* GetEntityInfoByName(const char* name);
    CEntInfo* GetEntityInfoByClassName(const char* name);
    std::vector<int> GetEntitiesIndexByClassName(const char* name);
    std::vector<CEntInfo*> GetEntitiesInfoByClassName(const char* name);
    IHandleEntity* LookupEntity(const CBaseHandle& handle);
    int GetEntityIndex(const CBaseHandle& handle);
};

extern EntityList* entityList;

extern Command sar_list_ents;
extern Command sar_find_ent;
extern Command sar_find_ents;
