#include "TasTools.hpp"

#include <cmath>
#include <cstring>

#include "Features/OffsetFinder.hpp"

#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

TasTools* tasTools;

TasTools::TasTools()
    : propName("m_hGroundEntity")
    , propType(PropType::Handle)
<<<<<<< HEAD
    , acceleration({ 0, 0, 0 })
    , prevVelocity({ 0, 0, 0 })
    , prevTick(0)
=======
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
    , data()
{
    for (auto i = 0; i < Offsets::MAX_SPLITSCREEN_PLAYERS; ++i) {
        this->data.push_back(new TasPlayerData());
    }

    if (sar.game->Is(SourceGame_Portal | SourceGame_Portal2Engine)) {
        std::strncpy(this->className, "CPortal_Player", sizeof(this->className));
    } else if (sar.game->Is(SourceGame_HalfLife2)) {
        std::strncpy(this->className, "CHL2_Player", sizeof(this->className));
    } else {
        std::strncpy(this->className, "CBasePlayer", sizeof(this->className));
    }

    offsetFinder->ServerSide(this->className, this->propName, &this->propOffset);

    this->hasLoaded = true;
}
<<<<<<< HEAD
=======
TasTools::~TasTools()
{
    for (auto& data : this->data) {
        delete data;
    }
    this->data.clear();
}
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
void TasTools::AimAtPoint(void* player, float x, float y, float z, int doSlerp)
{
    Vector target = { y, x, z };
    Vector campos = server->GetAbsOrigin(player) + server->GetViewOffset(player);
    campos = { campos.y, campos.x, campos.z };

    // Axis in the game, need to know it to fix up:
    //              : G - D ; Av - Ar ; H - B
    // Rotation Axis:   x        z        y
    // Translation  :   y        x        z

    float xdis = target.x - campos.x;
    float ydis = target.z - campos.z;
    float zdis = target.y - campos.y;
    float xzdis = sqrtf(xdis * xdis + zdis * zdis);

    QAngle angles = { RAD2DEG(-atan2f(ydis, xzdis)), RAD2DEG(-(atan2f(-xdis, zdis))), 0 };

<<<<<<< HEAD
    if (!doSlerp) {
        engine->SetAngles(angles);
        return;
    }

    auto slot = server->GetSplitScreenPlayerSlot(player);
    auto slotData = &this->data[slot];
    slotData->currentAngles = { engine->GetAngles().x, engine->GetAngles().y, 0 };
=======
    auto slot = server->GetSplitScreenPlayerSlot(player);
    if (!doSlerp) {
        engine->SetAngles(slot, angles);
        return;
    }

    auto slotData = this->data[slot];
    auto curAngles = engine->GetAngles(slot);
    slotData->currentAngles = { curAngles.x, curAngles.y, 0 };
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
    slotData->targetAngles = angles;
}
void* TasTools::GetPlayerInfo(void* player)
{
    return (player) ? reinterpret_cast<void*>((uintptr_t)player + this->propOffset) : nullptr;
}
Vector TasTools::GetVelocityAngles(void* player)
{
    auto velocityAngles = server->GetLocalVelocity(player);
    if (velocityAngles.Length() == 0) {
        return { 0, 0, 0 };
    }

    Math::VectorNormalize(velocityAngles);

    float yaw = atan2f(velocityAngles.y, velocityAngles.x);
    float pitch = atan2f(velocityAngles.z, sqrtf(velocityAngles.y * velocityAngles.y + velocityAngles.x * velocityAngles.x));

    return { RAD2DEG(yaw), RAD2DEG(pitch), 0 };
}
Vector TasTools::GetAcceleration(void* player)
{
    auto slot = server->GetSplitScreenPlayerSlot(player);
    auto slotData = this->data[slot];

    auto curTick = engine->GetSessionTick();
    if (slotData->prevTick != curTick) {
        auto curVelocity = server->GetLocalVelocity(player);

        // z used to represent the combined x/y acceleration axis value
        slotData->acceleration.z = curVelocity.Length2D() - slotData->prevVelocity.Length2D();
        slotData->acceleration.x = std::abs(curVelocity.x) - std::abs(slotData->prevVelocity.x);
        slotData->acceleration.y = std::abs(curVelocity.y) - std::abs(slotData->prevVelocity.y);

        slotData->prevVelocity = curVelocity;
        slotData->prevTick = curTick;
    }

    return slotData->acceleration;
}
void TasTools::SetAngles(void* player)
{
    auto slot = server->GetSplitScreenPlayerSlot(player);
    auto slotData = this->data[slot];

    if (slotData->speedInterpolation == 0) {
        return;
    }

    auto angles = engine->GetAngles(slot);
    slotData->currentAngles = { angles.x, angles.y, 0 };
    QAngle m = this->Slerp(slotData->currentAngles, slotData->targetAngles, slotData->speedInterpolation);

    engine->SetAngles(slot, m);

    if (m.x == slotData->targetAngles.x && m.y == slotData->targetAngles.y) {
        slotData->speedInterpolation = 0;
    }
}
QAngle TasTools::Slerp(QAngle a0, QAngle a1, float speedInterpolation)
{
    if (std::abs(a1.y - a0.y) > std::abs(a1.y + 360 - a0.y)) {
        a1.y = a1.y + 360;
    }
    if (std::abs(a1.y - a0.y) > std::abs(a1.y - 360 - a0.y)) {
        a1.y = a1.y - 360;
    }

    Vector vec{ a1.x - a0.x, a1.y - a0.y, 0 };
    Math::VectorNormalize(vec);

    QAngle m;
    m.x = a0.x + vec.x * speedInterpolation;
    m.y = a0.y + vec.y * speedInterpolation;
    m.z = 0;

    if (std::abs(a0.x - a1.x) <= speedInterpolation) {
        m.x = a1.x;
    }
    if (std::abs(a1.y - a0.y) <= speedInterpolation) {
        m.y = a1.y;
    }

    return m;
}
void TasTools::SetAngles(void* pPlayer)
{
    auto slot = server->GetSplitScreenPlayerSlot(pPlayer);
    auto slotData = &this->data[slot];

    if (slotData->speedInterpolation == 0) {
        return;
    }

    slotData->currentAngles = { engine->GetAngles(slot).x, engine->GetAngles(slot).y, 0 };
    QAngle m = tasTools->Slerp(slotData->currentAngles, slotData->targetAngles, slotData->speedInterpolation);

    engine->SetAngles(slot, m);

    if (m.x == slotData->targetAngles.x && m.y == slotData->targetAngles.y)
        slotData->speedInterpolation = 0;
}
QAngle TasTools::Slerp(QAngle a0, QAngle a1, float speedInterpolation)
{
    if (abs(a1.y - a0.y) > abs(a1.y + 360 - a0.y))
        a1.y = a1.y + 360;
    if (abs(a1.y - a0.y) > abs(a1.y - 360 - a0.y))
        a1.y = a1.y - 360;

    Vector vec{ a1.x - a0.x, a1.y - a0.y, 0 };
    Math::VectorNormalize(vec);

    QAngle m;
    m.x = a0.x + vec.x * speedInterpolation;
    m.y = a0.y + vec.y * speedInterpolation;
    m.z = 0;

    if (abs(a0.x - a1.x) <= speedInterpolation) {
        m.x = a1.x;
    }
    if (abs(a1.y - a0.y) <= speedInterpolation) {
        m.y = a1.y;
    }

    return m;
}

// Commands

CON_COMMAND(sar_tas_aim_at_point, "sar_tas_aim_at_point <x> <y> <z> [speed] : Aims at point {x, y, z} specified.\n"
<<<<<<< HEAD
                                  "Setting the [speed] parameter will make an time interpolation between current player angles and the targeted point.\n")
=======
                                  "Setting the [speed] parameter will make a time interpolation between current player angles and the targeted point.\n")
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
{
    if (!sv_cheats.GetBool()) {
        return console->Print("Cannot use sar_tas_aim_at_point without sv_cheats set to 1.\n");
    }

    if (args.ArgC() < 4) {
        return console->Print("Missing arguments: sar_tas_aim_at_point <x> <y> <z> [speed].\n");
    }

<<<<<<< HEAD
    auto player = server->GetPlayer();
    auto nSlot = GET_SLOT();
    if (player && args[4] == 0 || args.ArgC() == 4) {
        tasTools->data[nSlot].speedInterpolation = 0;
        tasTools->AimAtPoint(player, static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])), 0);
    }
    if (player && args.ArgC() > 4) {
        tasTools->data[nSlot].speedInterpolation = static_cast<float>(std::atof(args[4]));
=======
    auto nSlot = GET_SLOT();
    auto player = server->GetPlayer(nSlot + 1);
    if (player && args[4] == 0 || args.ArgC() == 4) {
        tasTools->data[nSlot]->speedInterpolation = 0;
        tasTools->AimAtPoint(player, static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])), 0);
    }
    if (player && args.ArgC() > 4) {
        tasTools->data[nSlot]->speedInterpolation = static_cast<float>(std::atof(args[4]));
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
        tasTools->AimAtPoint(player, static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])), 1);
    }
}
CON_COMMAND(sar_tas_set_prop, "sar_tas_set_prop <prop_name> : Sets value for sar_hud_player_info.\n")
{
    if (args.ArgC() < 2) {
        return console->Print(sar_tas_set_prop.ThisPtr()->m_pszHelpString);
    }

    auto offset = 0;
    offsetFinder->ServerSide(tasTools->className, args[1], &offset);

    if (!offset) {
        console->Print("Unknown prop of %s!\n", tasTools->className);
    } else {
        std::strncpy(tasTools->propName, args[1], sizeof(tasTools->propName));
        tasTools->propOffset = offset;

        if (Utils::StartsWith(tasTools->propName, "m_b")) {
            tasTools->propType = PropType::Boolean;
        } else if (Utils::StartsWith(tasTools->propName, "m_f")) {
            tasTools->propType = PropType::Float;
        } else if (Utils::StartsWith(tasTools->propName, "m_vec") || Utils::StartsWith(tasTools->propName, "m_ang") || Utils::StartsWith(tasTools->propName, "m_q")) {
            tasTools->propType = PropType::Vector;
        } else if (Utils::StartsWith(tasTools->propName, "m_h") || Utils::StartsWith(tasTools->propName, "m_p")) {
            tasTools->propType = PropType::Handle;
        } else if (Utils::StartsWith(tasTools->propName, "m_sz") || Utils::StartsWith(tasTools->propName, "m_isz")) {
            tasTools->propType = PropType::String;
        } else if (Utils::StartsWith(tasTools->propName, "m_ch")) {
            tasTools->propType = PropType::Char;
        } else {
            tasTools->propType = PropType::Integer;
        }
    }

    console->Print("Current prop: %s::%s\n", tasTools->className, tasTools->propName);
}
CON_COMMAND(sar_tas_addang, "sar_tas_addang <x> <y> [z] : Adds {x, y, z} degrees to {x, y, z} view axis.\n")
{
    if (!sv_cheats.GetBool()) {
        return console->Print("Cannot use sar_tas_addang without sv_cheats set to 1.\n");
    }

    if (args.ArgC() < 3) {
        return console->Print("Missing arguments : sar_tas_addang <x> <y> [z].\n");
    }

    auto nSlot = GET_SLOT();
    auto angles = engine->GetAngles(nSlot);

    angles.x += static_cast<float>(std::atof(args[1]));
    angles.y += static_cast<float>(std::atof(args[2])); // Player orientation

    if (args.ArgC() == 4)
        angles.z += static_cast<float>(std::atof(args[3]));

    engine->SetAngles(nSlot, angles);
}
CON_COMMAND(sar_tas_setang, "sar_tas_setang <x> <y> [z] [speed] : Sets {x, y, z} degrees to view axis.\n"
<<<<<<< HEAD
                            "Setting the [speed] parameter will make an time interpolation between current player angles and the targeted angles.\n")
=======
                            "Setting the [speed] parameter will make a time interpolation between current player angles and the targeted angles.\n")
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
{
    if (!sv_cheats.GetBool()) {
        return console->Print("Cannot use sar_tas_setang without sv_cheats set to 1.\n");
    }

    if (args.ArgC() < 3) {
        return console->Print("Missing arguments : sar_tas_setang <x> <y> [z] [speed].\n");
    }

    auto nSlot = GET_SLOT();

    // Fix the bug when z is not set
    if (args.ArgC() == 3) {
<<<<<<< HEAD
        tasTools->data[nSlot].speedInterpolation = 0;
        engine->SetAngles(QAngle{ static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), 0.0 });
    } else if (args.ArgC() < 5 || args[4] == 0) {
        tasTools->data[nSlot].speedInterpolation = 0;
        engine->SetAngles(QAngle{ static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])) });
    } else {
        tasTools->data[nSlot].speedInterpolation = static_cast<float>(std::atof(args[4]));
        tasTools->data[nSlot].currentAngles = { engine->GetAngles(nSlot).x, engine->GetAngles(nSlot).y, 0 };
        tasTools->data[nSlot].targetAngles = { static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])) };
=======
        tasTools->data[nSlot]->speedInterpolation = 0;
        engine->SetAngles(nSlot, QAngle{ static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), 0.0 });
    } else if (args.ArgC() < 5 || args[4] == 0) {
        tasTools->data[nSlot]->speedInterpolation = 0;
        engine->SetAngles(nSlot, QAngle{ static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])) });
    } else {
        auto angles = engine->GetAngles(nSlot);
        tasTools->data[nSlot]->speedInterpolation = static_cast<float>(std::atof(args[4]));
        tasTools->data[nSlot]->currentAngles = { angles.x, angles.y, 0 };
        tasTools->data[nSlot]->targetAngles = { static_cast<float>(std::atof(args[1])), static_cast<float>(std::atof(args[2])), static_cast<float>(std::atof(args[3])) };
>>>>>>> 1b44ab346acb6d0aa2e8764be26e9196dfc09ec2
    }
}
