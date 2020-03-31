#include "ClassDumper.hpp"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

#ifdef _WIN32
#include <functional>
#endif

#include "Modules/Client.hpp"
#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

#include "Features/EntityList.hpp"

#include "Hud/Hud.hpp"

#include "Utils/SDK.hpp"

#include "SAR.hpp"

ClassDumper* classDumper;

ClassDumper::ClassDumper()
    : sendPropSize(sar.game->Is(SourceGame_Portal2Engine) ? sizeof(SendProp2) : sizeof(SendProp))
    , serverClassesFile("server_classes.json")
    , clientClassesFile("client_classes.json")
{
    this->hasLoaded = true;
}
void ClassDumper::Dump(bool dumpServer)
{
    auto source = (dumpServer) ? &this->serverClassesFile : &this->clientClassesFile;

    std::ofstream file(*source, std::ios::out | std::ios::trunc);
    if (!file.good()) {
        console->Warning("Failed to create file!\n");
        return file.close();
    }

    file << "{\"data\":[";

    if (dumpServer) {
        for (auto sclass = server->GetAllServerClasses(); sclass; sclass = sclass->m_pNext) {
            file << "{\"class\":\"" << sclass->m_pNetworkName << "\",\"table\":";
            this->DumpSendTable(file, sclass->m_pTable);
            file << "},";
        }
    } else {
        for (auto cclass = client->GetAllClasses(); cclass; cclass = cclass->m_pNext) {
            file << "{\"class\":\"" << cclass->m_pNetworkName << "\",\"table\":";
            this->DumpRecvTable(file, cclass->m_pRecvTable);
            file << "},";
        }
    }

    file.seekp(-1, std::ios_base::_Seekcur);
    file << "]}";
    file.close();

    console->Print("Created %s file.\n", source->c_str());
}
void ClassDumper::DumpSendTable(std::ofstream& file, SendTable* table)
{
    file << "{\"name\":\"" << table->m_pNetTableName << "\",\"props\":[";

    for (auto i = 0; i < table->m_nProps; ++i) {
        auto prop = *reinterpret_cast<SendProp*>((uintptr_t)table->m_pProps + this->sendPropSize * i);

        auto name = prop.m_pVarName;
        auto offset = prop.m_Offset;
        auto type = prop.m_Type;
        auto nextTable = prop.m_pDataTable;

        if (sar.game->Is(SourceGame_Portal2Engine)) {
            auto temp = *reinterpret_cast<SendProp2*>(&prop);
            name = temp.m_pVarName;
            offset = temp.m_Offset;
            type = temp.m_Type;
            nextTable = temp.m_pDataTable;
        }

        auto sanitized = std::string("");
        auto c = name;
        while (*c != '\0') {
            if (*c == '"') {
                sanitized += "\\";
            }
            sanitized += *c;
            ++c;
        }

        file << "{\"name\":\"" << sanitized.c_str() << "\",\"offset\":" << (int16_t)offset;

        if (type != SendPropType::DPT_DataTable) {
            file << ",\"type\":" << type << "},";
            continue;
        }

        file << ",\"table\":";

        this->DumpSendTable(file, nextTable);

        file << "},";
    }

    if (table->m_nProps != 0) {
        file.seekp(-1, std::ios_base::_Seekcur);
    }
    file << "]}";
}
void ClassDumper::DumpRecvTable(std::ofstream& file, RecvTable* table)
{
    file << "{\"name\":\"" << table->m_pNetTableName << "\",\"props\":[";

    for (auto i = 0; i < table->m_nProps; ++i) {
        auto prop = table->m_pProps[i];

        auto name = prop.m_pVarName;
        auto offset = prop.m_Offset;
        auto type = prop.m_RecvType;
        auto nextTable = prop.m_pDataTable;

        auto sanitized = std::string("");
        auto c = name;
        while (*c != '\0') {
            if (*c == '"') {
                sanitized += "\\";
            }
            sanitized += *c;
            ++c;
        }

        file << "{\"name\":\"" << sanitized.c_str() << "\",\"offset\":" << (int16_t)offset;

        if (type != SendPropType::DPT_DataTable) {
            file << ",\"type\":" << type << "},";
            continue;
        }

        file << ",\"table\":";

        this->DumpRecvTable(file, nextTable);

        file << "},";
    }

    if (table->m_nProps != 0) {
        file.seekp(-1, std::ios_base::_Seekcur);
    }
    file << "]}";
}

// Commands

CON_COMMAND(sar_dump_server_classes, "Dumps all server classes to a file.\n")
{
    classDumper->Dump();
}
CON_COMMAND(sar_dump_client_classes, "Dumps all client classes to a file.\n")
{
    classDumper->Dump(false);
}
CON_COMMAND(sar_list_server_classes, "Lists all server classes.\n")
{
    for (auto sclass = server->GetAllServerClasses(); sclass; sclass = sclass->m_pNext) {
        console->Print("%s\n", sclass->m_pNetworkName);
    }
}
CON_COMMAND(sar_list_client_classes, "Lists all client classes.\n")
{
    for (auto cclass = client->GetAllClasses(); cclass; cclass = cclass->m_pNext) {
        console->Print("%s\n", cclass->m_pNetworkName);
    }
}
CON_COMMAND(sar_find_server_class, "Finds specific server class tables and props with their offset.\n"
                                   "Usage: sar_find_serverclass <class_name>\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_find_server_class.ThisPtr()->m_pszHelpString);
    }

    std::function<void(SendTable * table, int& level)> DumpTable;
    DumpTable = [&DumpTable](SendTable* table, int& level) {
        console->Print("%*s%s\n", level * 4, "", table->m_pNetTableName);
        for (auto i = 0; i < table->m_nProps; ++i) {
            auto prop = *reinterpret_cast<SendProp*>((uintptr_t)table->m_pProps + classDumper->sendPropSize * i);

            auto name = prop.m_pVarName;
            auto offset = prop.m_Offset;
            auto type = prop.m_Type;
            auto nextTable = prop.m_pDataTable;

            if (sar.game->Is(SourceGame_Portal2Engine)) {
                auto temp = *reinterpret_cast<SendProp2*>(&prop);
                name = temp.m_pVarName;
                offset = temp.m_Offset;
                type = temp.m_Type;
                nextTable = temp.m_pDataTable;
            }

            console->Msg("%*s%s -> %d\n", level * 4, "", name, (int16_t)offset);

            if (type != SendPropType::DPT_DataTable) {
                continue;
            }

            ++level;
            DumpTable(nextTable, level);
        }
        --level;
    };

    for (auto sclass = server->GetAllServerClasses(); sclass; sclass = sclass->m_pNext) {
        if (!std::strcmp(args[1], sclass->m_pNetworkName)) {
            console->Print("%s\n", sclass->m_pNetworkName);
            auto level = 1;
            DumpTable(sclass->m_pTable, level);
            break;
        }
    }
}
CON_COMMAND(sar_find_client_class, "Finds specific client class tables and props with their offset.\n"
                                   "Usage: sar_find_clientclass <class_name>\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_find_client_class.ThisPtr()->m_pszHelpString);
    }

    std::function<void(RecvTable * table, int& level)> DumpTable;
    DumpTable = [&DumpTable](RecvTable* table, int& level) {
        console->Print("%*s%s\n", level * 4, "", table->m_pNetTableName);
        for (auto i = 0; i < table->m_nProps; ++i) {
            auto prop = table->m_pProps[i];

            auto name = prop.m_pVarName;
            auto offset = prop.m_Offset;
            auto type = prop.m_RecvType;
            auto nextTable = prop.m_pDataTable;

            console->Msg("%*s%s -> %d\n", level * 4, "", name, (int16_t)offset);

            if (type != SendPropType::DPT_DataTable) {
                continue;
            }

            ++level;
            DumpTable(nextTable, level);
        }
        --level;
    };

    for (auto cclass = client->GetAllClasses(); cclass; cclass = cclass->m_pNext) {
        if (!std::strcmp(args[1], cclass->m_pNetworkName)) {
            console->Print("%s\n", cclass->m_pNetworkName);
            auto level = 1;
            DumpTable(cclass->m_pRecvTable, level);
            break;
        }
    }
}
CON_COMMAND(sar_show_entity, "sar_show_entity <entity classname OR entity index> <network_classname> [filter1] [filter2] ... Hide the elements specified when using sar_hud_entity.\n")
{
    if (args.ArgC() < 3) {
        return console->Print(sar_show_entity.ThisPtr()->m_pszHelpString);
    }

    if (std::isdigit(args[1][0])) { //Search by Index
        CEntInfo* entity = entityList->GetEntityInfoByIndex(std::atoi(args[1]));
        if (entity == nullptr || entity->m_pEntity == nullptr) {
            return console->Print("Failed to find an entity with the '%d' index. Are you sure you gave the right index ?\n", std::atoi(args[1]));
        } else {
            console->Print("Found 1 entity with this index : classname -> %s\n", server->GetEntityClassName(entity->m_pEntity));
            classDumper->entity = entity;
        }
    } else { //Search by class name
        auto entities = entityList->GetEntitiesInfoByClassName(args[1]);
        if (entities.size() > 1) {
            console->Print("Found multiple entities with this classname. Use sar_show_entity with the index corresponding to the entity you're looking for.\n");
            console->Print("[index] address | m_iClassName | m_iName\n");
            auto v = entityList->GetEntitiesIndexByClassName(args[1]);
            for (int i = 0; i < v.size(); ++i) {
                auto entity = entities[i];
                if (entity && entity->m_pEntity) {
                    console->Print("[%i] ", i);
                    console->Msg("%d", v[i]);
                    console->Print(" | ");
                    console->Msg("%s", server->GetEntityClassName(entity->m_pEntity));
                    console->Print(" | ");
                    console->Msg("%s\n", server->GetEntityName(entity->m_pEntity));
                }
            }
            return;
        } else if (entities.size() == 1) {
            CEntInfo* entity = entities[0];
            if (entity == nullptr || entity->m_pEntity == nullptr) {
                return console->Print("Failed to find an entity with the '%s' classname. Are you sure you gave the right classname ?\n", args[1]);
            } else {
                int found = -1;
                for (auto index = 0; index < Offsets::NUM_ENT_ENTRIES; ++index) {
                    auto info = entityList->GetEntityInfoByIndex(index);
                    if (info->m_pEntity == nullptr) {
                        continue;
                    }

                    auto match = server->GetEntityClassName(info->m_pEntity);
                    if (info->m_pEntity != entity->m_pEntity) {
                        continue;
                    }

                    found = index;
                    continue;
                }

                if (found == -1) {
                    return console->Print("Failed to find an entity with the '%s' classname. Are you sure you gave the right classname ?\n", args[1]);
                }

                console->Print("Found 1 entity with this entity class name : index -> %d\n", found);
                classDumper->entity = entity;
            }
        } else if (entities.empty()) {
            return console->Print("Failed to find an entity with the '%s' classname. Are you sure you gave the right classname ?\n", args[1]);
        }
    }

    classDumper->network_classname = args[2];

    classDumper->filters.clear();
    if (args.ArgC() > 3) {
        classDumper->filters.reserve(args.ArgC());
        for (int i = 3; i < args.ArgC(); ++i) {
            classDumper->filters.push_back(args[i]);
        }
    }
}

HUD_ELEMENT(entity, "", "sar_hud_entity <entity_name> <classname> [filters list]: Display every properties of an entity.\n", HudType_InGame)
{
    if (classDumper->entity != nullptr) {
        std::function<void(SendTable * table, int& level)> DumpTable;
        DumpTable = [&DumpTable, &ctx](SendTable* table, int& level) {
            for (auto i = 0; i < table->m_nProps; ++i) {
                auto prop = *reinterpret_cast<SendProp*>((uintptr_t)table->m_pProps + classDumper->sendPropSize * i);

                auto name = prop.m_pVarName;
                auto offset = prop.m_Offset;
                auto type = prop.m_Type;
                auto nextTable = prop.m_pDataTable;

                if (sar.game->Is(SourceGame_Portal2Engine)) {
                    auto temp = *reinterpret_cast<SendProp2*>(&prop);
                    name = temp.m_pVarName;
                    offset = temp.m_Offset;
                    type = temp.m_Type;
                    nextTable = temp.m_pDataTable;
                }

                bool stop = false;
                for (auto& filter : classDumper->filters) {
                    if (std::string(name).find(filter) != std::string::npos) {
                        stop = true;
                        continue;
                    }
                }
                if (stop) {
                    continue;
                }

                char out[512];
                std::sprintf(out, "%*s%s -> ", level * 4, "", name);
                //std::string out(str1);

#define GET_VALUE(varType) *reinterpret_cast<varType*>((uintptr_t)classDumper->entity->m_pEntity + offset)
                if (Utils::StartsWith(name, "m_b")) {
                    ctx->DrawElement("%s%s\n", out, GET_VALUE(bool) ? "True" : "False");
                } else if (Utils::StartsWith(name, "m_f")) {
                    ctx->DrawElement("%s%.3f\n", out, GET_VALUE(float));
                } else if (Utils::StartsWith(name, "m_vec") || Utils::StartsWith(name, "m_ang") || Utils::StartsWith(name, "m_q")) {
                    Vector v = GET_VALUE(Vector);
                    ctx->DrawElement("%s%.3f %.3f %.3f\n", out, v.x, v.y, v.z);
                } else if (Utils::StartsWith(name, "m_h") || Utils::StartsWith(name, "m_p")) {
                    ctx->DrawElement("%s%p\n", out, GET_VALUE(void*));
                    /*char childClassName[256];
                    auto handle = *reinterpret_cast<CBaseHandle*>((uintptr_t)classDumper->entity->m_pEntity + offset);
                    auto childIndex = entityList->GetEntityIndex(handle);
                    CEntInfo* child = entityList->GetEntityInfoByIndex(childIndex);
                    if (child->m_pEntity != nullptr) {
                        const char* lol = server->GetEntityClassName(child->m_pEntity);
                        if (lol != nullptr && !lol[0]) {
                            std::string s(lol ? lol : "");
                            ctx->DrawElement("%s%s\n", out, s.c_str());
                        } else {
                            ctx->DrawElement("%s(null)\n", out);
                        }
                    } else {
                        ctx->DrawElement("%s(null)\n", out);
                    }*/
                } else if (Utils::StartsWith(name, "m_sz") || Utils::StartsWith(name, "m_isz")) {
                    ctx->DrawElement("%s%s\n", out, GET_VALUE(char*));
                } else if (Utils::StartsWith(name, "m_ch")) {
                    ctx->DrawElement("%s%c\n", out, GET_VALUE(char));
                } else {
                    ctx->DrawElement("%s%d\n", out, GET_VALUE(int));
                }

                if (type != SendPropType::DPT_DataTable) {
                    continue;
                }

                ++level;
                DumpTable(nextTable, level);
            }
            --level;
        };

        for (auto sclass = server->GetAllServerClasses(); sclass; sclass = sclass->m_pNext) {
            if (!std::strcmp(classDumper->network_classname.c_str(), sclass->m_pNetworkName)) {
                auto level = 1;
                DumpTable(sclass->m_pTable, level);
                break;
            }
        }
    }
}

//sar_hud_entity 1; sar_show_entity player CPortal_Player m_fl m_hMyWeapons m_chArea m_AnimOverlay m_iAmmo m_EntityPortal m_audio m_skybox
