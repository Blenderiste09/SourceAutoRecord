#pragma once
#include "Feature.hpp"

#include <fstream>
#include <string>

#include "Utils/SDK.hpp"

#include "Command.hpp"

class ClassDumper : public Feature {
public:
    int sendPropSize;
    std::vector<std::string> filters;
    CEntInfo* entity;
    std::string network_classname;
    std::vector<std::string> props;

private:
    std::string serverClassesFile;
    std::string clientClassesFile;

public:
    ClassDumper();
    void Dump(bool dumpServer = true);

private:
    void DumpSendTable(std::ofstream& file, SendTable* table);
    void DumpRecvTable(std::ofstream& file, RecvTable* table);
};

extern ClassDumper* classDumper;

extern Command sar_dump_server_classes;
extern Command sar_dump_client_classes;
extern Command sar_list_server_classes;
extern Command sar_list_client_classes;
extern Command sar_find_server_class;
extern Command sar_find_client_class;
extern Command sar_show_entity;
