#include <stdio.h>
#include "as_admcmd.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"

AdminCMD g_AdminCMD;
PLUGIN_EXPOSE(AdminCMD, g_AdminCMD);

IVEngineServer2*   engine              = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem*     g_pEntitySystem     = nullptr;
CGlobalVars*       gpGlobals           = nullptr;

IUtilsApi*   g_pUtils      = nullptr;
IMenusApi*   g_pMenus      = nullptr;
IPlayersApi* g_pPlayersApi = nullptr;
IAdminApi*   g_pAdminApi   = nullptr;

#define MAX_PLAYERS  64

bool g_bESP[65]        = {};
bool g_bInfAmmo[65]    = {};
bool g_bPlayerESP[65]  = {};
bool g_bPlayerAmmo[65] = {};

int g_iTeleportVictim[65] = {};

CGameEntitySystem* GameEntitySystem()
{
	return g_pUtils->GetCGameEntitySystem();
}

void StartupServer()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem     = g_pUtils->GetCEntitySystem();
	gpGlobals           = g_pUtils->GetCGlobalVars();
}

static bool IsPlayerValid(int slot)
{
	if (slot < 0 || slot >= MAX_PLAYERS) return false;
	CCSPlayerController* p = CCSPlayerController::FromSlot(slot);
	if (!p || p->IsBot() || !p->IsConnected()) return false;
	if (!engine->IsClientFullyAuthenticated(slot)) return false;
	return true;
}

static bool IsAlive(int slot)
{
	CCSPlayerController* p = CCSPlayerController::FromSlot(slot);
	if (!p) return false;
	CCSPlayerPawn* pawn = p->GetPlayerPawn();
	return pawn && pawn->IsAlive();
}

static const char* AdminName(int iSlot)
{
	return iSlot == -1 ? "Console" : g_pPlayersApi->GetPlayerName(iSlot);
}

static void ToggleESP(int iSlot)
{
	char szCmd[64];
	g_SMAPI->Format(szCmd, sizeof(szCmd), "css_adminwh_toggle %i", iSlot);
	engine->ServerCommand(szCmd);
}

void ShowMainMenu(int iSlot);
void ShowRespawnMenu(int iSlot);
void ShowGiveESPMenu(int iSlot);
void ShowGiveAmmoMenu(int iSlot);
void ShowTeleportStep1(int iSlot);
void ShowTeleportStep2(int iSlot, int iVictim);
void ShowGiveMoneyMenu(int iSlot);
void ShowTakeMoneyMenu(int iSlot);
void ShowMoneyAmountMenu(int iSlot, int iTarget, bool bGive);
void ShowGiveWeaponMenu(int iSlot);
void ShowWeaponListMenu(int iSlot, int iTarget);
void ShowResetScoreMenu(int iSlot);

void ShowMainMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Category_AdminCommands"));

	char szESP[64];
	g_SMAPI->Format(szESP, sizeof(szESP), "Admin ESP: [%s]",
		g_bESP[iSlot] ? g_pAdminApi->GetTranslation("Status_On") : g_pAdminApi->GetTranslation("Status_Off"));
	g_pMenus->AddItemMenu(hMenu, "esp", szESP);

	g_pMenus->AddItemMenu(hMenu, "respawn",    g_pAdminApi->GetTranslation("Item_RespawnPlayer"));
	g_pMenus->AddItemMenu(hMenu, "teleport",   g_pAdminApi->GetTranslation("Item_TeleportPlayer"));
	g_pMenus->AddItemMenu(hMenu, "givemoney",  g_pAdminApi->GetTranslation("Item_GiveMoney"));
	g_pMenus->AddItemMenu(hMenu, "takemoney",  g_pAdminApi->GetTranslation("Item_TakeMoney"));

	char szAmmo[64];
	g_SMAPI->Format(szAmmo, sizeof(szAmmo), "%s: [%s]",
		g_pAdminApi->GetTranslation("Item_InfAmmo"),
		g_bInfAmmo[iSlot] ? g_pAdminApi->GetTranslation("Status_On") : g_pAdminApi->GetTranslation("Status_Off"));
	g_pMenus->AddItemMenu(hMenu, "infammo", szAmmo);

	g_pMenus->AddItemMenu(hMenu, "giveweapon", g_pAdminApi->GetTranslation("Item_GiveWeapon"));
	g_pMenus->AddItemMenu(hMenu, "resetscore", g_pAdminApi->GetTranslation("Item_ResetScore"));
	g_pMenus->AddItemMenu(hMenu, "giveesp",    g_pAdminApi->GetTranslation("Item_GiveESP"));
	g_pMenus->AddItemMenu(hMenu, "giveammo",   g_pAdminApi->GetTranslation("Item_GiveAmmo"));

	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { g_pAdminApi->ShowAdminLastCategoryMenu(iSlot); return; }
		if (iItem > 7) return;

		if (!strcmp(szBack, "esp"))
		{
			g_bESP[iSlot] = !g_bESP[iSlot];
			ToggleESP(iSlot);
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				if (!IsPlayerValid(i) || i == iSlot) continue;
				if (!g_pAdminApi->HasPermission(i, "@admin/root")) continue;
				char szMsg[256];
				g_SMAPI->Format(szMsg, sizeof(szMsg),
					g_bESP[iSlot]
						? g_pAdminApi->GetTranslation("ESP_Enabled_Root")
						: g_pAdminApi->GetTranslation("ESP_Disabled_Root"),
					AdminName(iSlot));
				g_pUtils->PrintToChat(i, szMsg);
			}
			g_pAdminApi->SendAction(iSlot, "esp", g_bESP[iSlot] ? "1" : "0");
			ShowMainMenu(iSlot);
			return;
		}

		if (!strcmp(szBack, "infammo"))
		{
			g_bInfAmmo[iSlot] = !g_bInfAmmo[iSlot];
			if (g_bInfAmmo[iSlot])
				g_pUtils->PrintToChat(iSlot, g_pAdminApi->GetTranslation("InfAmmo_Enabled"));
			else
				g_pUtils->PrintToChat(iSlot, g_pAdminApi->GetTranslation("InfAmmo_Disabled"));
			g_pAdminApi->SendAction(iSlot, "infammo", g_bInfAmmo[iSlot] ? "1" : "0");
			ShowMainMenu(iSlot);
			return;
		}

		if (!strcmp(szBack, "respawn"))    { ShowRespawnMenu(iSlot);   return; }
		if (!strcmp(szBack, "teleport"))   { ShowTeleportStep1(iSlot); return; }
		if (!strcmp(szBack, "givemoney"))  { ShowGiveMoneyMenu(iSlot); return; }
		if (!strcmp(szBack, "takemoney"))  { ShowTakeMoneyMenu(iSlot); return; }
		if (!strcmp(szBack, "giveweapon")) { ShowGiveWeaponMenu(iSlot); return; }
		if (!strcmp(szBack, "resetscore")) { ShowResetScoreMenu(iSlot); return; }
		if (!strcmp(szBack, "giveesp"))    { ShowGiveESPMenu(iSlot);   return; }
		if (!strcmp(szBack, "giveammo"))   { ShowGiveAmmoMenu(iSlot);  return; }
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowRespawnMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_RespawnPlayer"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i)) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		g_pPlayersApi->Respawn(iTarget);
		if (g_pAdminApi->GetMessageType() == 2)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminRespawnMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
		else if (g_pAdminApi->GetMessageType() == 1)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("RespawnMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		g_pAdminApi->SendAction(iSlot, "respawn", std::to_string(iTarget).c_str());
		ShowRespawnMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowTeleportStep1(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Teleport_SelectTarget"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i) || !IsAlive(i)) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iVictim = std::atoi(szBack);
		if (!IsPlayerValid(iVictim)) return;
		g_iTeleportVictim[iSlot] = iVictim;
		ShowTeleportStep2(iSlot, iVictim);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowTeleportStep2(int iSlot, int iVictim)
{
	Menu hMenu;
	char szTitle[128];
	g_SMAPI->Format(szTitle, sizeof(szTitle), "%s: %s",
		g_pAdminApi->GetTranslation("Teleport_SelectDest"),
		g_pPlayersApi->GetPlayerName(iVictim));
	g_pMenus->SetTitleMenu(hMenu, szTitle);
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i) || !IsAlive(i) || i == iVictim) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [iVictim](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowTeleportStep1(iSlot); return; }
		if (iItem > 7) return;
		int iDest = std::atoi(szBack);
		if (!IsPlayerValid(iDest) || !IsPlayerValid(iVictim)) return;
		CCSPlayerController* pV = CCSPlayerController::FromSlot(iVictim);
		CCSPlayerController* pD = CCSPlayerController::FromSlot(iDest);
		if (pV && pD)
		{
			CCSPlayerPawn* pawnV = pV->GetPlayerPawn();
			CCSPlayerPawn* pawnD = pD->GetPlayerPawn();
			if (pawnV && pawnD)
			{
				Vector vPos = pawnD->GetAbsOrigin();
				vPos.z += 70.0f;
				QAngle vAng = pawnD->GetAbsRotation();
				Vector vVel = vec3_origin;
				g_pUtils->TeleportEntity(pawnV, &vPos, &vAng, &vVel);
			}
		}
		if (g_pAdminApi->GetMessageType() == 2)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminTeleportMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iVictim), g_pPlayersApi->GetPlayerName(iDest));
		else if (g_pAdminApi->GetMessageType() == 1)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("TeleportMessage"), g_pPlayersApi->GetPlayerName(iVictim), g_pPlayersApi->GetPlayerName(iDest));
		char szAction[64];
		g_SMAPI->Format(szAction, sizeof(szAction), "%i %i", iVictim, iDest);
		g_pAdminApi->SendAction(iSlot, "teleport", szAction);
		ShowMainMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowMoneyAmountMenu(int iSlot, int iTarget, bool bGive)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Money_SelectAmount"));
	g_pMenus->AddItemMenu(hMenu, "2000",  "$ 2 000");
	g_pMenus->AddItemMenu(hMenu, "4000",  "$ 4 000");
	g_pMenus->AddItemMenu(hMenu, "6000",  "$ 6 000");
	g_pMenus->AddItemMenu(hMenu, "8000",  "$ 8 000");
	g_pMenus->AddItemMenu(hMenu, "10000", "$ 10 000");
	g_pMenus->AddItemMenu(hMenu, "12000", "$ 12 000");
	g_pMenus->AddItemMenu(hMenu, "14000", "$ 14 000");
	g_pMenus->AddItemMenu(hMenu, "16000", "$ 16 000 (MAX)");
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [iTarget, bGive](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iAmount = std::atoi(szBack);
		CCSPlayerController* p = CCSPlayerController::FromSlot(iTarget);
		if (p && p->m_pInGameMoneyServices())
		{
			int iCur = p->m_pInGameMoneyServices()->m_iAccount();
			int iNew = bGive ? std::min(iCur + iAmount, 16000) : std::max(iCur - iAmount, 0);
			p->m_pInGameMoneyServices()->m_iAccount = iNew;
			g_pUtils->SetStateChanged(p, "CCSPlayerController_InGameMoneyServices", "m_iAccount");
		}
		if (bGive)
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminGiveMoneyMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget), iAmount);
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("GiveMoneyMessage"), g_pPlayersApi->GetPlayerName(iTarget), iAmount);
		}
		else
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminTakeMoneyMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget), iAmount);
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("TakeMoneyMessage"), g_pPlayersApi->GetPlayerName(iTarget), iAmount);
		}
		char szAction[64];
		g_SMAPI->Format(szAction, sizeof(szAction), "%i %s %i", iTarget, bGive ? "give" : "take", iAmount);
		g_pAdminApi->SendAction(iSlot, "money", szAction);
		ShowMainMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowGiveMoneyMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_GiveMoney"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i)) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		ShowMoneyAmountMenu(iSlot, iTarget, true);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowTakeMoneyMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_TakeMoney"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i)) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		ShowMoneyAmountMenu(iSlot, iTarget, false);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowWeaponListMenu(int iSlot, int iTarget)
{
	Menu hMenu;
	char szTitle[128];
	g_SMAPI->Format(szTitle, sizeof(szTitle), "%s: %s",
		g_pAdminApi->GetTranslation("Weapon_SelectWeapon"),
		g_pPlayersApi->GetPlayerName(iTarget));
	g_pMenus->SetTitleMenu(hMenu, szTitle);
	g_pMenus->AddItemMenu(hMenu, "weapon_glock",         "Glock-18");
	g_pMenus->AddItemMenu(hMenu, "weapon_usp_silencer",  "USP-S");
	g_pMenus->AddItemMenu(hMenu, "weapon_p250",          "P250");
	g_pMenus->AddItemMenu(hMenu, "weapon_fiveseven",     "Five-SeveN");
	g_pMenus->AddItemMenu(hMenu, "weapon_tec9",          "Tec-9");
	g_pMenus->AddItemMenu(hMenu, "weapon_cz75a",         "CZ75-Auto");
	g_pMenus->AddItemMenu(hMenu, "weapon_deagle",        "Desert Eagle");
	g_pMenus->AddItemMenu(hMenu, "weapon_revolver",      "R8 Revolver");
	g_pMenus->AddItemMenu(hMenu, "weapon_mac10",         "MAC-10");
	g_pMenus->AddItemMenu(hMenu, "weapon_mp9",           "MP9");
	g_pMenus->AddItemMenu(hMenu, "weapon_mp5sd",         "MP5-SD");
	g_pMenus->AddItemMenu(hMenu, "weapon_ump45",         "UMP-45");
	g_pMenus->AddItemMenu(hMenu, "weapon_p90",           "P90");
	g_pMenus->AddItemMenu(hMenu, "weapon_bizon",         "PP-Bizon");
	g_pMenus->AddItemMenu(hMenu, "weapon_ak47",          "AK-47");
	g_pMenus->AddItemMenu(hMenu, "weapon_m4a1",          "M4A4");
	g_pMenus->AddItemMenu(hMenu, "weapon_m4a1_silencer", "M4A1-S");
	g_pMenus->AddItemMenu(hMenu, "weapon_galil",         "Galil AR");
	g_pMenus->AddItemMenu(hMenu, "weapon_famas",         "FAMAS");
	g_pMenus->AddItemMenu(hMenu, "weapon_sg556",         "SG 553");
	g_pMenus->AddItemMenu(hMenu, "weapon_aug",           "AUG");
	g_pMenus->AddItemMenu(hMenu, "weapon_awp",           "AWP");
	g_pMenus->AddItemMenu(hMenu, "weapon_ssg08",         "SSG 08");
	g_pMenus->AddItemMenu(hMenu, "weapon_scar20",        "SCAR-20");
	g_pMenus->AddItemMenu(hMenu, "weapon_g3sg1",         "G3SG1");
	g_pMenus->AddItemMenu(hMenu, "weapon_nova",          "Nova");
	g_pMenus->AddItemMenu(hMenu, "weapon_xm1014",        "XM1014");
	g_pMenus->AddItemMenu(hMenu, "weapon_sawedoff",      "Sawed-Off");
	g_pMenus->AddItemMenu(hMenu, "weapon_mag7",          "MAG-7");
	g_pMenus->AddItemMenu(hMenu, "weapon_m249",          "M249");
	g_pMenus->AddItemMenu(hMenu, "weapon_negev",         "Negev");
	g_pMenus->AddItemMenu(hMenu, "weapon_hegrenade",     "HE Grenade");
	g_pMenus->AddItemMenu(hMenu, "weapon_flashbang",     "Flashbang");
	g_pMenus->AddItemMenu(hMenu, "weapon_smokegrenade",  "Smoke Grenade");
	g_pMenus->AddItemMenu(hMenu, "weapon_molotov",       "Molotov");
	g_pMenus->AddItemMenu(hMenu, "weapon_incgrenade",    "Incendiary");
	g_pMenus->AddItemMenu(hMenu, "weapon_decoy",         "Decoy");
	g_pMenus->AddItemMenu(hMenu, "item_kevlar",          "Kevlar");
	g_pMenus->AddItemMenu(hMenu, "item_assaultsuit",     "Kevlar + Helmet");
	g_pMenus->AddItemMenu(hMenu, "item_defuser",         "Defuse Kit");
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [iTarget](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowGiveWeaponMenu(iSlot); return; }
		if (iItem > 7) return;
		CCSPlayerController* p = CCSPlayerController::FromSlot(iTarget);
		if (p)
		{
			CCSPlayerPawn* pawn = p->GetPlayerPawn();
			if (pawn && pawn->m_pItemServices())
				pawn->m_pItemServices()->GiveNamedItem(szBack);
		}
		if (g_pAdminApi->GetMessageType() == 2)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminGiveWeaponMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget), szFront);
		else if (g_pAdminApi->GetMessageType() == 1)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("GiveWeaponMessage"), g_pPlayersApi->GetPlayerName(iTarget), szFront);
		char szAction[64];
		g_SMAPI->Format(szAction, sizeof(szAction), "%i %s", iTarget, szBack);
		g_pAdminApi->SendAction(iSlot, "give_weapon", szAction);
		ShowWeaponListMenu(iSlot, iTarget);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowGiveWeaponMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_GiveWeapon"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i) || !IsAlive(i)) continue;
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), g_pPlayersApi->GetPlayerName(i));
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		ShowWeaponListMenu(iSlot, iTarget);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowResetScoreMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_ResetScore"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i)) continue;
		CCSPlayerController* p = CCSPlayerController::FromSlot(i);
		if (!p) continue;
		int iKills  = p->m_pActionTrackingServices() ? p->m_pActionTrackingServices()->m_matchStats().m_iKills()  : 0;
		int iDeaths = p->m_pActionTrackingServices() ? p->m_pActionTrackingServices()->m_matchStats().m_iDeaths() : 0;
		char szBuf[128];
		g_SMAPI->Format(szBuf, sizeof(szBuf), "%s [K:%i D:%i]", g_pPlayersApi->GetPlayerName(i), iKills, iDeaths);
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), szBuf);
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		CCSPlayerController* p = CCSPlayerController::FromSlot(iTarget);
		if (p)
		{
			p->m_iScore = 0;
			p->m_iMVPs  = 0;
			g_pUtils->SetStateChanged(p, "CCSPlayerController", "m_iScore");
			if (p->m_pActionTrackingServices())
			{
				p->m_pActionTrackingServices()->m_matchStats().m_iKills   = 0;
				p->m_pActionTrackingServices()->m_matchStats().m_iDeaths  = 0;
				p->m_pActionTrackingServices()->m_matchStats().m_iAssists = 0;
				g_pUtils->SetStateChanged(p, "CCSPlayerController_ActionTrackingServices", "m_matchStats");
			}
		}
		if (g_pAdminApi->GetMessageType() == 2)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminResetScoreMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
		else if (g_pAdminApi->GetMessageType() == 1)
			g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("ResetScoreMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		g_pAdminApi->SendAction(iSlot, "reset_score", std::to_string(iTarget).c_str());
		ShowResetScoreMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}


void ShowGiveESPMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_GiveESP"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i) || i == iSlot) continue;
		char szName[128];
		g_SMAPI->Format(szName, sizeof(szName), "%s [%s]",
			g_pPlayersApi->GetPlayerName(i),
			g_bPlayerESP[i] ? g_pAdminApi->GetTranslation("Status_On") : g_pAdminApi->GetTranslation("Status_Off"));
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), szName);
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		g_bPlayerESP[iTarget] = !g_bPlayerESP[iTarget];
		char szCmd[64];
		g_SMAPI->Format(szCmd, sizeof(szCmd), "css_adminwh_toggle %i", iTarget);
		engine->ServerCommand(szCmd);
		if (g_bPlayerESP[iTarget])
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminGiveESPMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("GiveESPMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		}
		else
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminTakeESPMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("TakeESPMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		}
		g_pAdminApi->SendAction(iSlot, "give_esp", std::to_string(iTarget).c_str());
		ShowGiveESPMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

void ShowGiveAmmoMenu(int iSlot)
{
	Menu hMenu;
	g_pMenus->SetTitleMenu(hMenu, g_pAdminApi->GetTranslation("Item_GiveAmmo"));
	bool bFound = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerValid(i) || i == iSlot) continue;
		char szName[128];
		g_SMAPI->Format(szName, sizeof(szName), "%s [%s]",
			g_pPlayersApi->GetPlayerName(i),
			g_bPlayerAmmo[i] ? g_pAdminApi->GetTranslation("Status_On") : g_pAdminApi->GetTranslation("Status_Off"));
		g_pMenus->AddItemMenu(hMenu, std::to_string(i).c_str(), szName);
		bFound = true;
	}
	if (!bFound)
		g_pMenus->AddItemMenu(hMenu, "", g_pAdminApi->GetTranslation("NoPlayers"), ITEM_DISABLED);
	g_pMenus->SetBackMenu(hMenu, true);
	g_pMenus->SetExitMenu(hMenu, true);
	g_pMenus->SetCallback(hMenu, [](const char* szBack, const char* szFront, int iItem, int iSlot)
	{
		if (iItem == 7) { ShowMainMenu(iSlot); return; }
		if (iItem > 7) return;
		int iTarget = std::atoi(szBack);
		if (!IsPlayerValid(iTarget)) return;
		g_bPlayerAmmo[iTarget] = !g_bPlayerAmmo[iTarget];
		if (g_bPlayerAmmo[iTarget])
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminGiveAmmoMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("GiveAmmoMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		}
		else
		{
			if (g_pAdminApi->GetMessageType() == 2)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("AdminTakeAmmoMessage"), AdminName(iSlot), g_pPlayersApi->GetPlayerName(iTarget));
			else if (g_pAdminApi->GetMessageType() == 1)
				g_pUtils->PrintToChatAll(g_pAdminApi->GetTranslation("TakeAmmoMessage"), g_pPlayersApi->GetPlayerName(iTarget));
		}
		g_pAdminApi->SendAction(iSlot, "give_ammo", std::to_string(iTarget).c_str());
		ShowGiveAmmoMenu(iSlot);
	});
	g_pMenus->DisplayPlayerMenu(hMenu, iSlot);
}

bool AdminCMD::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	g_SMAPI->AddListener(this, this);
	ConVar_Register(FCVAR_GAMEDLL | FCVAR_SERVER_CAN_EXECUTE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_NOTIFY);
	return true;
}

bool AdminCMD::Unload(char* error, size_t maxlen)
{
	ConVar_Unregister();
	g_pUtils->ClearAllHooks(g_PLID);
	return true;
}

void AdminCMD::AllPluginsLoaded()
{
	char error[64];
	int ret;

	g_pUtils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_SMAPI->Format(error, sizeof(error), "Missing Utils plugin");
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
		return;
	}

	g_pPlayersApi = (IPlayersApi*)g_SMAPI->MetaFactory(Players_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_pUtils->ErrorLog("[%s] Missing Players plugin", GetLogTag());
		engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
		return;
	}

	g_pMenus = (IMenusApi*)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_pUtils->ErrorLog("[%s] Missing Menus plugin", GetLogTag());
		engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
		return;
	}

	g_pAdminApi = (IAdminApi*)g_SMAPI->MetaFactory(Admin_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_pUtils->ErrorLog("[%s] Missing AdminSystem plugin", GetLogTag());
		engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
		return;
	}

	g_pUtils->HookEvent(g_PLID, "player_disconnect", [](const char* szName, IGameEvent* pEvent, bool bBroadcast)
	{
		int slot = pEvent->GetInt("userid");
		if (slot < 0 || slot >= MAX_PLAYERS) return;
		g_bESP[slot]        = false;
		g_bInfAmmo[slot]    = false;
		g_bPlayerESP[slot]  = false;
		g_bPlayerAmmo[slot] = false;
	});

	g_pUtils->HookEvent(g_PLID, "weapon_fire", [](const char* szName, IGameEvent* pEvent, bool bBroadcast)
	{
		int slot = pEvent->GetInt("userid");
		if (slot < 0 || slot >= MAX_PLAYERS || (!g_bInfAmmo[slot] && !g_bPlayerAmmo[slot])) return;
		CCSPlayerController* p = CCSPlayerController::FromSlot(slot);
		if (!p) return;
		CCSPlayerPawn* pawn = p->GetPlayerPawn();
		if (!pawn || !pawn->m_pWeaponServices()) return;
		CBasePlayerWeapon* weapon = pawn->m_pWeaponServices()->m_hActiveWeapon().Get();
		if (!weapon) return;
		weapon->m_iClip1 = 250;
	});

	g_pPlayersApi->HookOnClientAuthorized(g_PLID, [](int iSlot, uint64 iSteamID64)
	{
		g_bESP[iSlot]        = false;
		g_bInfAmmo[iSlot]    = false;
		g_bPlayerESP[iSlot]  = false;
		g_bPlayerAmmo[iSlot] = false;
	});

	g_pUtils->StartupServer(g_PLID, StartupServer);

	g_pAdminApi->OnCoreLoaded(g_PLID, []()
	{
		g_pAdminApi->RegisterCategory("admin_commands", "Category_AdminCommands", nullptr);
		g_pAdminApi->RegisterItem(
			"admin_commands",
			"Category_AdminCommands",
			"admin_commands",
			"@admin/admcmd",
			nullptr,
			[](int iSlot, const char* szCategory, const char* szIdentity, const char* szItem)
			{
				ShowMainMenu(iSlot);
			}
		);
	});
}

const char* AdminCMD::GetLicense()     { return "GPL"; }
const char* AdminCMD::GetVersion()     { return "1.0.0"; }
const char* AdminCMD::GetDate()        { return __DATE__; }
const char* AdminCMD::GetLogTag()      { return "AdminCMD"; }
const char* AdminCMD::GetAuthor()      { return "sq1z"; }
const char* AdminCMD::GetDescription() { return "Admin Commands module for AdminSystem"; }
const char* AdminCMD::GetName()        { return "[AS] Admin Commands"; }
const char* AdminCMD::GetURL()         { return "https://github.com/sq1zzzz"; }
