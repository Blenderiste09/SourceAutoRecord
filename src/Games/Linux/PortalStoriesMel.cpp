#include "PortalStoriesMel.hpp"

#include "Game.hpp"

PortalStoriesMel::PortalStoriesMel()
{
    this->version = SourceGame_PortalStoriesMel;
}
const char* PortalStoriesMel::Version()
{
    return "Portal Stories: Mel (7054)";
}
const char* PortalStoriesMel::ModDir()
{
    return "portal_stories";
}
