/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "hud.h"
#include "cl_util.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "weapons/CEagle.h"
#include "weapons/CPipewrench.h"
#include "weapons/CM249.h"
#include "weapons/CDisplacer.h"
#include "weapons/CShockRifle.h"
#include "weapons/CSporeLauncher.h"
#include "weapons/CSniperRifle.h"
#include "weapons/CKnife.h"
#include "weapons/CPenguin.h"

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[32];

#include "pm_shared.h"

void V_PunchAxis(int axis, float punch);
void VectorAngles(const float* forward, float* angles);

extern cvar_t* cl_lw;

//RENDERERS START
#include "r_studioint.h"
#include "com_model.h"
#include "bsprenderer.h"
#include "particle_engine.h"
#include "com_weapons.h"

#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

extern CGameStudioModelRenderer g_StudioRenderer;
extern engine_studio_api_t IEngineStudio;
//RENDERERS END

//water splash
void EV_HLDM_WaterSplash(float x, float y, float z)
{
	int  iWaterSplash = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/effects/splash1.spr");
	TEMPENTITY* pTemp = gEngfuncs.pEfxAPI->R_TempSprite(Vector(x, y, z + 50),
		Vector(0, 0, 0),
		0.5, iWaterSplash, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL);

	if (pTemp)
	{
		pTemp->fadeSpeed = 90.0;
		pTemp->entity.curstate.framerate = 100.0;
		pTemp->entity.curstate.renderamt = 155;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
	}

	iWaterSplash = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/effects/splash2.spr");
	pTemp = gEngfuncs.pEfxAPI->R_TempSprite(Vector(x, y, z),
		Vector(0, 0, 0.2),
		0.7, iWaterSplash, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL);

	if (pTemp)
	{
		pTemp->fadeSpeed = 60.0;
		pTemp->entity.curstate.framerate = 40.0;
		pTemp->entity.curstate.renderamt = 100;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
		pTemp->entity.angles = Vector(90, 0, 0);
	}

	//gEngfuncs.pEventAPI->EV_PlaySound(0, Vector(x, y, z), 0, "ambience/splash_1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM);

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0: gEngfuncs.pEventAPI->EV_PlaySound(0, Vector(x, y, z), 0, "ambience/splash_1.wav", 10, ATTN_NORM, 0, PITCH_NORM); break;
	case 1: gEngfuncs.pEventAPI->EV_PlaySound(0, Vector(x, y, z), 0, "ambience/splash_2.wav", 10, ATTN_NORM, 0, PITCH_NORM); break;
	}
}

//RENDERERS START
char* EV_HLDM_HDDecal(pmtrace_t* ptr, physent_t* pe, float* vecSrc, float* vecEnd)
{
	if (gEngfuncs.PM_PointContents(ptr->endpos, NULL) == CONTENT_SKY)
		return 0;

	// hit the world, try to play sound based on texture material type
	char chTextureType = 0;
	int entity;
	char* pStart;
	char* pTextureName;
	char texname[64];
	char szbuffer[64];
	static char decalname[32];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);

	if (pe && pe->solid == SOLID_BSP)
	{
		// Nothing
		if (vecSrc == 0 && vecEnd == 0)
		{
			// hit body
			chTextureType = 0;
		}
		else
		{

			// get texture from entity or world (world is ent(0))
			pTextureName = (char*)gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);
			pStart = pTextureName;

			if (pTextureName && strcmp("black", pTextureName))
			{
				strcpy(texname, pTextureName);
				pTextureName = texname;

				// strip leading '-0' or '+0~' or '{' or '!'
				if (*pTextureName == '-' || *pTextureName == '+')
				{
					pTextureName += 2;
				}

				if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
				{
					pTextureName++;
				}

				// '}}'
				strcpy(szbuffer, pTextureName);
				szbuffer[CBTEXTURENAMEMAX - 1] = 0;

				// get texture type
				chTextureType = PM_FindTextureType(szbuffer);
			}
			else
			{
				return FALSE;
			}
		}
	}

	if (pStart[0] == '{')
		return 0;

	cl_entity_t* pHit = gEngfuncs.GetEntityByIndex(gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr));

	if (pHit->curstate.rendermode == kRenderTransColor && pHit->curstate.renderamt == 0)
	{
		if (chTextureType == CHAR_TEX_CONCRETE)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_METAL)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_GRATE)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_DIRT)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("dirt_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_VENT)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_TILE)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_WOOD)
		{
			sprintf(decalname, "shot_wood");
			gParticleEngine.CreateCluster("wood_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_COMPUTER)
		{
			sprintf(decalname, "shot");
		}
		else if (chTextureType == CHAR_TEX_GLASS)
		{
			sprintf(decalname, "shot_glass");
			gParticleEngine.CreateCluster("glass_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}

		g_StudioRenderer.StudioDecalExternal(ptr->endpos, ptr->plane.normal, decalname);
		return FALSE;
	}

	if (pe->classnumber == 1 && pHit->curstate.renderamt)
	{
		sprintf(decalname, "shot_glass");
		gParticleEngine.CreateCluster("glass_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
	}
	else if (pe->rendermode != kRenderNormal && pHit->curstate.renderamt)
	{
		sprintf(decalname, "shot_glass");
		gParticleEngine.CreateCluster("glass_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
	}
	else
	{
		if (chTextureType == CHAR_TEX_CONCRETE)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_METAL)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_GRATE)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_DIRT)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("dirt_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_VENT)
		{
			sprintf(decalname, "shot_metal");
		}
		else if (chTextureType == CHAR_TEX_TILE)
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_WOOD)
		{
			sprintf(decalname, "shot_wood");
			gParticleEngine.CreateCluster("wood_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else if (chTextureType == CHAR_TEX_COMPUTER)
		{
			sprintf(decalname, "shot");
		}
		else if (chTextureType == CHAR_TEX_GLASS)
		{
			sprintf(decalname, "shot_glass");
			gParticleEngine.CreateCluster("glass_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
		else
		{
			sprintf(decalname, "shot");
			gParticleEngine.CreateCluster("concrete_impact_cluster.txt", ptr->endpos, ptr->plane.normal, 0);
		}
	}
	return decalname;
}
//RENDERERS END


// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound(int idx, pmtrace_t* ptr, float* vecSrc, float* vecEnd, int iBulletType)
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char* pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if (entity >= 1 && entity <= gEngfuncs.GetMaxClients())
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char*)gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

		if (pTextureName)
		{
			strcpy(texname, pTextureName);
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			// get texture type
			chTextureType = PM_FindTextureType(szbuffer);
		}
	}

	if (gEngfuncs.PM_PointContents(ptr->endpos, NULL) == CONTENT_SKY)
		return FALSE;

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE: fvol = 0.9;	fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL: fvol = 0.9; fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:	fvol = 0.9; fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:	fvol = 0.5; fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE: fvol = 0.9; fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:	fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH: fvol = 0.9; fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD: fvol = 0.9; fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;	fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0, 0xf));
	return fvolbar;
}

void EV_HLDM_MuzzleFlash(Vector pos, float amount, int iR = 255, int iG = 255, int iB = 128)

{
	dlight_t * dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	
	dl->origin = pos;
	dl->color.r = iR; // red
	dl->color.g = iG; // green
	dl->color.b = iB; // blue
	dl->radius = amount * 100;
	dl->die = gEngfuncs.GetClientTime() + 0.15;
}


char* EV_HLDM_DamageDecal(physent_t* pe)
{
	static char decalname[32];
	int idx;

	if (pe->classnumber == 1)
	{
		idx = gEngfuncs.pfnRandomLong(0, 2);
		sprintf(decalname, "{break%i", idx + 1);
	}
	else if (pe->rendermode != kRenderNormal)
	{
		sprintf(decalname, "{bproof1");
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong(0, 4);
		sprintf(decalname, "{shot%i", idx + 1);
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	int iRand;
	physent_t* pe;


	//RENDERERS START
	if (gParticleEngine.m_pCvarDrawParticles->value <= 0)
		gEngfuncs.pEfxAPI->R_BulletImpactParticles(pTrace->endpos);
	//RENDERERS END

	iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2))// not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0:	gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 1:	gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 2:	gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 3:	gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		case 4:	gEngfuncs.pEventAPI->EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM); break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	//RENDERERS START
		// Only decal brush models such as the world etc.
	if (decalName && decalName[0] && pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
	{
		if (pTrace->allsolid || pTrace->fraction == 1.0)
			return;

		gBSPRenderer.CreateDecal(pTrace->endpos, pTrace->plane.normal, decalName, FALSE);
	}
	//RENDERERS END
}

void EV_HLDM_DecalGunshot(pmtrace_t* pTrace, int iBulletType, float* vecSrc, float* vecEnd)
{
	physent_t* pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	if (pe && pe->solid == SOLID_BSP)
	{
		switch (iBulletType)
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		case BULLET_PLAYER_556:
		case BULLET_PLAYER_762:
		case BULLET_PLAYER_EAGLE:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace(pTrace, EV_HLDM_HDDecal(pTrace, pe, vecSrc, vecEnd));
			break;
		}
	}
}

int EV_HLDM_CheckTracer(int idx, float* vecSrc, float* end, float* forward, float* right, int iBulletType, int iTracerFreq, int* tracerCount)
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if (iTracerFreq != 0 && ((*tracerCount)++ % iTracerFreq) == 0)
	{
		Vector vecTracerSrc;

		if (player)
		{
			Vector offset(0, 0, -4);

			// adjust tracer position for player
			for (i = 0; i < 3; i++)
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			VectorCopy(vecSrc, vecTracerSrc);
		}

		if (iTracerFreq != 1)		// guns that always trace also always decal
			tracer = 1;

		switch (iBulletType)
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_12MM:
		case BULLET_PLAYER_556:
		case BULLET_PLAYER_762:
		case BULLET_PLAYER_EAGLE:
		case BULLET_PLAYER_357:
		case BULLET_PLAYER_BUCKSHOT:
		default:
			EV_CreateTracer(vecTracerSrc, end);
			break;
		}
	}

	return tracer;
}


/*
================
FireBullets
Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx, float* forward, float* right, float* up, int cShots, float* vecSrc, float* vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int* tracerCount, float flSpreadX, float flSpreadY)
{
	int i;
	pmtrace_t tr;
	int iShot;
	int tracer;

	for (iShot = 1; iShot <= cShots; iShot++)
	{
		Vector vecDir, vecEnd;

		float x, y, z;
		//We randomize for the Shotgun.
		if (iBulletType == BULLET_PLAYER_BUCKSHOT)
		{
			do {
				x = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				z = x * x + y * y;
			} while (z > 1);

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

		tracer = EV_HLDM_CheckTracer(idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount);

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
			switch (iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END

				break;
			case BULLET_PLAYER_MP5:

				if (!tracer)
				{
					EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
					//RENDERERS START
					EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
					//RENDERERS END
				}
				break;
			case BULLET_PLAYER_BUCKSHOT:

				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END

				break;
			case BULLET_PLAYER_357:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END

				break;

			case BULLET_PLAYER_EAGLE:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END
				break;

			case BULLET_PLAYER_762:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END
				break;

			case BULLET_PLAYER_556:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				//RENDERERS START
				EV_HLDM_DecalGunshot(&tr, iBulletType, vecSrc, vecEnd);
				//RENDERERS END
				break;
			}


		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//drop mag
void EV_HLDM_DropMag(float x, float y, float z, int body)
{
	int gibz = gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_item_gibs.mdl");// brass shell
	Vector endpos;
	VectorClear(endpos);
	endpos[1] = RANDOM_LONG(0, 255);
	TEMPENTITY* a = gEngfuncs.pEfxAPI->R_TempModel(Vector(x, y, z), Vector(0, 0, 0), endpos, 5, gibz, TE_BOUNCE_SHELL);
	a->entity.curstate.body = body;
}


//======================

//PARTICLES START

//======================

void EV_HLDM_Particles(vec_t Pos_X, vec_t Pos_Y, vec_t Pos_Z, float PosNorm_X, float PosNorm_Y, float PosNorm_Z, int DoPuff, int Material)

{
	return;
	pmtrace_t tr;
	
	pmtrace_t * pTrace = &tr;
	
	
	
	pTrace->endpos.x = Pos_X;
	
	pTrace->endpos.y = Pos_Y;
	
	pTrace->endpos.z = Pos_Z;
	
	
	
	pTrace->plane.normal.x = PosNorm_X;
	
	pTrace->plane.normal.y = PosNorm_Y;
	
	pTrace->plane.normal.z = PosNorm_Z;
	
	
	Vector angles, forward, right, up;
	
	
	
	VectorAngles(pTrace->plane.normal, angles);
	
	AngleVectors(angles, forward, up, right);
	
	forward.z = -forward.z;
	
	
	
	bool fDoPuffs = false;
	
	bool fDoSparks = false;
	
	bool fDoMuzzle = false;
	
	int a, r, g, b;

		float scale;
		if (Material == 0)//concrete, tile
		{
			fDoSparks = (gEngfuncs.pfnRandomLong(1, 2) == 1);
			
			fDoPuffs = true;
			
			fDoMuzzle = false;
			
			a = 96;
			
			r = 128;
			
			g = 128;
			
			b = 128;
			
			scale = 0.03;
			
		}
		if (Material == 1)//metal, vent, grate
		{
			
			fDoSparks = (gEngfuncs.pfnRandomLong(1, 2) == 1);
			
			fDoPuffs = false;
			
			fDoMuzzle = true;
			
			scale = 0.03;
			
		}
		if (Material == 2)//wood
			
		{
			fDoPuffs = true;
			
			fDoSparks = false;
			
			fDoMuzzle = false;
			
			a = 128;
			
			r = 97;
			
			g = 86;
			
			b = 53;
			
			scale = 0.06;
			
		}
		if (Material == 3)//dirt
			
		{
			
			fDoPuffs = true;
			
			fDoSparks = false;
			
			fDoMuzzle = false;
			
			a = 64;
			
			r = 128;
			
			g = 128;
			
			b = 128;
			
			scale = 0.03;
			
		}
		if (Material == 4)//glass
			
		{
			
			fDoPuffs = false;
			
			fDoSparks = false;
			
			fDoMuzzle = false;
			
			scale = 0.03;
			
		}
		if (Material == 5)//computer
		{
			fDoPuffs = false;
			
			fDoSparks = (gEngfuncs.pfnRandomLong(1, 2) == 1);
			
			fDoMuzzle = false;
			
			scale = 0.03;
			
		}
		if (DoPuff != 0)
			
		{
			
				if (fDoPuffs)
				{ // get sprite index
						int  iWallsmoke = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/debris/smokepuff.spr");
						// create sprite
						TEMPENTITY * pTemp = gEngfuncs.pEfxAPI->R_TempSprite(pTrace->endpos,
							
							forward * gEngfuncs.pfnRandomFloat(10, 30) + right * gEngfuncs.pfnRandomFloat(-6, 6) + up * gEngfuncs.pfnRandomFloat(0, 6),
							
							0.7, iWallsmoke, kRenderTransAdd, kRenderFxNone, 1.0, 0.3, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL);
						if (pTemp)
						{// sprite created successfully, adjust some things
							
							pTemp->fadeSpeed = 3.0;
							
							pTemp->entity.curstate.framerate = 25.0;
							
							pTemp->entity.curstate.renderamt = a;
							
							pTemp->entity.curstate.rendercolor.r = r;
							
							pTemp->entity.curstate.rendercolor.g = g;
							
							pTemp->entity.curstate.rendercolor.b = b;
							
						}
				}
		}
		if (fDoSparks)
		{ // make some sparks
				gEngfuncs.pEfxAPI->R_SparkShower(pTrace->endpos);
		}
		if (fDoMuzzle)
			
		{
			
			dlight_t * dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
			
			dl->origin = pTrace->endpos;
			
			dl->color.r = 255; // red
			
			dl->color.g = 255; // green
			
			dl->color.b = 128; // blue
			
			dl->radius = 100;
			
			dl->die = gEngfuncs.GetClientTime() + 0.01;
			

			gEngfuncs.pEfxAPI->R_MuzzleFlash(pTrace->endpos, 11);
			
		}
}
//=======================================
//Particles end
//=======================================



//======================
//	    GLOCK START
//======================
void EV_FireGlock1(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	int empty;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	empty = args->bparam1;
	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 2);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	EV_GetGunPosition(args, vecSrc, origin);

	EV_HLDM_MuzzleFlash(vecSrc, 1.5 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}

void EV_FireGlock2(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector vecSpread;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(GLOCK_SHOOT, 2);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	EV_GetGunPosition(args, vecSrc, origin);

	EV_HLDM_MuzzleFlash(vecSrc, 1.5 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx - 1], args->fparam1, args->fparam2);

}
//======================
//	   GLOCK END
//======================

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	int j;
	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector vecSpread;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOTGUN_FIRE2, 2);
		V_PunchAxis(0, -10.0);
	}

	for (j = 0; j < 2; j++)
	{
		EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);

		EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);

	EV_HLDM_MuzzleFlash(vecSrc, 2.5 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	VectorCopy(forward, vecAiming);

	if (gEngfuncs.GetMaxClients() > 1)
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx - 1], 0.17365, 0.04362);
	}
	else
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx - 1], 0.08716, 0.08716);
	}
}

void EV_FireShotGunSingle(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector vecSpread;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOTGUN_FIRE, 2);

		V_PunchAxis(0, -5.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);
	EV_HLDM_MuzzleFlash(vecSrc, 2.0 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));
	VectorCopy(forward, vecAiming);

	if (gEngfuncs.GetMaxClients() > 1)
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx - 1], 0.08716, 0.04362);
	}
	else
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx - 1], 0.08716, 0.08716);
	}
}
//======================
//	   SHOTGUN END
//======================

//======================
//	    MP5 START
//======================
void EV_FireMP5(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 2);

		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);
	EV_HLDM_MuzzleFlash(vecSrc, 2 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx - 1], args->fparam1, args->fparam2);
	
}

// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP52(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5_LAUNCH, 2);
		V_PunchAxis(0, -10);
	}
	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}



}
//======================
//		 MP5 END
//======================

//======================
//		 GL START
//======================
// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_GLFire(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	gEngfuncs.pEventAPI->EV_WeaponAnimation(GL_FIRE1, 2);
	EV_HLDM_MuzzleFlash(origin, 2.0 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));
	V_PunchAxis(0, -10);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/gl_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));


	//gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/m79_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
}
//======================
//		 GL END
//======================

//======================
//	    AR START
//======================
void EV_ar1(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");// brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(AR_FIRE1 + gEngfuncs.pfnRandomLong(0, 1), 2);

		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);


	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/556ar_shoot.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));


	EV_GetGunPosition(args, vecSrc, origin);
	EV_HLDM_MuzzleFlash(vecSrc, 2.1 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_556, 1, &tracerCount[idx - 1], args->fparam1, args->fparam2);

}

void EV_ar2(event_args_t* args)
{
	// roflmao
}
//======================
//		 AR END
//======================

//======================
//	   PHYTON START 
//	     ( .357 )
//======================
void EV_FirePython(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector vecSrc, vecAiming;
	Vector up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		// Python uses different body in multiplayer versus single player
		int multiplayer = gEngfuncs.GetMaxClients() == 1 ? 0 : 1;

		const auto body = multiplayer ? 1 : 0;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(PYTHON_FIRE1, body);

		V_PunchAxis(0, -10.0);

		SetLocalBody(WEAPON_PYTHON, body);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);

	EV_HLDM_MuzzleFlash(vecSrc, 2.3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 1, &tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//	    PHYTON END 
//	     ( .357 )
//======================

//======================
//	   GAUSS START 
//======================
void EV_SpinGauss(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	int iSoundState = 0;

	int pitch;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	EV_HLDM_MuzzleFlash(args->origin, 1.8 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 50, 255, 255);

	pitch = args->iparam1;

	iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch);
}

/*
==============================
EV_StopPreviousGauss
==============================
*/
void EV_StopPreviousGauss(int idx)
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents(idx, "events/gaussspin.sc");
	gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_WEAPON, "ambience/pulsemachine.wav");
}

extern float g_flApplyVel;

void EV_FireGauss(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	float flDamage = args->fparam1;
	int primaryfire = args->bparam1;

	int m_fPrimaryFire = args->bparam1;
	int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	Vector vecSrc;
	Vector vecDest;
	edict_t* pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;
	physent_t* pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	Vector up, right, forward;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	if (args->bparam2)
	{
		EV_StopPreviousGauss(idx);
		return;
	}


	//	Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition(args, vecSrc, origin);

	EV_HLDM_MuzzleFlash(vecSrc, 3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 255, 160, 0);

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr");
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr");

	AngleVectors(angles, forward, right, up);

	VectorMA(vecSrc, 8192, forward, vecDest);

	if (EV_IsLocal(idx))
	{
		V_PunchAxis(0, -2.0);
		gEngfuncs.pEventAPI->EV_WeaponAnimation(GAUSS_FIRE2, 2);

		if (m_fPrimaryFire == false)
			g_flApplyVel = flDamage;

	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong(0, 0x1f));

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr);

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if (tr.allsolid)
			break;

		if (fFirstBeam)
		{
			if (EV_IsLocal(idx))
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint(
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				(m_fPrimaryFire ? 128.0 : flDamage) / 255.0,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0,
				(m_fPrimaryFire ? 128 : 255) / 255.0,
				(m_fPrimaryFire ? 0 : 255) / 255.0
			);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints(vecSrc,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				(m_fPrimaryFire ? 128.0 : flDamage) / 255.0,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0,
				(m_fPrimaryFire ? 128 : 255) / 255.0,
				(m_fPrimaryFire ? 0 : 255) / 255.0
			);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
		if (pEntity == NULL)
			break;

		if (pEntity->solid == SOLID_BSP)
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct(tr.plane.normal, forward);

			if (n < 0.5) // 60 degrees	
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;

				VectorMA(forward, 2.0 * n, tr.plane.normal, r);

				flMaxFrac = flMaxFrac - tr.fraction;

				VectorCopy(r, forward);

				VectorMA(tr.endpos, 8.0, forward, vecSrc);
				VectorMA(vecSrc, 8192.0, forward, vecDest);

				gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT);

				Vector fwd;
				VectorAdd(tr.endpos, tr.plane.normal, fwd);

				gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
					255, 100);

				// lose energy
				if (n == 0)
				{
					n = 0.1;
				}

				flDamage = flDamage * (1 - n);

			}
			else
			{
				// tunnel
				EV_HLDM_DecalGunshot(&tr, BULLET_MONSTER_12MM, vecSrc, vecDest);

				gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT);

				// limit it to one hole punch
				if (fHasPunched)
				{
					break;
				}
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if (!m_fPrimaryFire)
				{
					Vector start;

					VectorMA(tr.endpos, 8.0, forward, start);

					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();

					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);
					gEngfuncs.pEventAPI->EV_PlayerTrace(start, vecDest, PM_STUDIO_BOX, -1, &beam_tr);

					if (!beam_tr.allsolid)
					{
						Vector delta;
						float n;

						// trace backwards to find exit point

						gEngfuncs.pEventAPI->EV_PlayerTrace(beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr);

						VectorSubtract(beam_tr.endpos, tr.endpos, delta);

						n = Length(delta);

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// absorption balls
							{
								Vector fwd;
								VectorSubtract(tr.endpos, forward, fwd);
								gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
									255, 100);
							}

							//////////////////////////////////// WHAT TO DO HERE
													// CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							EV_HLDM_DecalGunshot(&beam_tr, BULLET_MONSTER_12MM, vecSrc, vecDest);

							gEngfuncs.pEfxAPI->R_TempSprite(beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT);

							// balls
							{
								Vector fwd;
								VectorSubtract(beam_tr.endpos, forward, fwd);
								gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 200,
									255, 40);
							}

							VectorAdd(beam_tr.endpos, forward, vecSrc);
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if (m_fPrimaryFire)
					{
						// slug doesn't punch through ever with primary 
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT);

						{
							Vector fwd;
							VectorAdd(tr.endpos, tr.plane.normal, fwd);
							gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
								255, 200);
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			VectorAdd(tr.endpos, forward, vecSrc);
		}
	}
}
//======================
//	   GAUSS END 
//======================

//======================
//	   CROWBAR START
//======================
int g_iSwing;

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Crowbar(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK1MISS, 1);

		switch ((g_iSwing++) % 3)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK1MISS, 1); break;
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK2MISS, 1); break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK3MISS, 1); break;
		}
	}
}
//======================
//	   CROWBAR END 
//======================

//======================
//	  CROSSBOW START
//======================
//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles 
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback(struct tempent_s* ent, float frametime, float currenttime)
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2(event_args_t* args)
{
	Vector vecSrc, vecEnd;
	Vector up, right, forward;
	pmtrace_t tr;

	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);

	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	EV_GetGunPosition(args, vecSrc, origin);

	VectorMA(vecSrc, 8192, forward, vecEnd);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	if (EV_IsLocal(idx))
	{
		if (args->iparam1)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE1, 1);
		else if (args->iparam2)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE3, 1);
	}

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

	//We hit something
	if (tr.fraction < 1.0)
	{
		physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);

		//Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if (pe->solid != SOLID_BSP)
		{
			switch (gEngfuncs.pfnRandomLong(0, 1))
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}
		//Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if (pe->rendermode == kRenderNormal)
		{
			gEngfuncs.pEventAPI->EV_PlaySound(0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM);

			//Not underwater, do some sparks...
			if (gEngfuncs.PM_PointContents(tr.endpos, NULL) != CONTENTS_WATER)
				gEngfuncs.pEfxAPI->R_SparkShower(tr.endpos);

			Vector vBoltAngles;
			int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("models/crossbow_bolt.mdl");

			VectorAngles(forward, vBoltAngles);

			TEMPENTITY* bolt = gEngfuncs.pEfxAPI->R_TempModel(tr.endpos - forward * 10, Vector(0, 0, 0), vBoltAngles, 5, iModelIndex, TE_BOUNCE_NULL);

			if (bolt)
			{
				bolt->flags |= (FTENT_CLIENTCUSTOM); //So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles; //Look forward!
				bolt->callback = EV_BoltCallback; //So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

//TODO: Fully predict the fliying bolt.
void EV_FireCrossbow(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	//Only play the weapon anims if I shot it. 
	if (EV_IsLocal(idx))
	{
		if (args->iparam1)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE1, 1);
		else if (args->iparam2)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE3, 1);

		V_PunchAxis(0, -2.0);
	}
}
//======================
//	   CROSSBOW END 
//======================

//======================
//	    RPG START 
//======================
void EV_FireRpg(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);

	EV_HLDM_MuzzleFlash(origin, 3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	//Only play the weapon anims if I shot it. 
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(RPG_FIRE2, 1);

		V_PunchAxis(0, -5.0);
	}
}
//======================
//	     RPG END 
//======================

//======================
//	    EGON END 
//======================
int g_fireAnims1[] = { EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4 };
int g_fireAnims2[] = { EGON_ALTFIRECYCLE };

BEAM* pBeam;
BEAM* pBeam2;
TEMPENTITY* pFlare;	// Vit_amiN: egon's beam flare

void EV_EgonFlareCallback(struct tempent_s* ent, float frametime, float currenttime)
{
	float delta = currenttime - ent->tentOffset.z;	// time past since the last scale
	if (delta >= ent->tentOffset.y)
	{
		ent->entity.curstate.scale += ent->tentOffset.x * delta;
		ent->tentOffset.z = currenttime;
	}

}

void EV_EgonFire(event_args_t* args)
{
	int idx, iFireState, iFireMode;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	iFireState = args->iparam1;
	iFireMode = args->iparam2;
	int iStartup = args->bparam1;


	if (iStartup)
	{
		if (iFireMode == FIRE_WIDE)
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125);
		else
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100);
	}
	else
	{
		//If there is any sound playing already, kill it.
		//This is necessary because multiple sounds can play on the same channel at the same time.
		//In some cases, more than 1 run sound plays when the egon stops firing, in which case only the earliest entry in the list is stopped.
		//This ensures no more than 1 of those is ever active at the same time.
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, EGON_SOUND_RUN);

		if (iFireMode == FIRE_WIDE)
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125);
		else
			gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100);
	}

	//Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(g_fireAnims1[gEngfuncs.pfnRandomLong(0, 3)], 1);

	if (iStartup == 1 && EV_IsLocal(idx) && !pBeam && !pBeam2 && !pFlare && cl_lw->value) //Adrian: Added the cl_lw check for those lital people that hate weapon prediction.
	{
		Vector vecSrc, vecEnd, origin, angles, forward, right, up;
		pmtrace_t tr;

		cl_entity_t* pl = gEngfuncs.GetEntityByIndex(idx);

		if (pl)
		{
			VectorCopy(gHUD.m_vecAngles, angles);

			AngleVectors(angles, forward, right, up);

			EV_GetGunPosition(args, vecSrc, pl->origin);
			EV_HLDM_MuzzleFlash(vecSrc, 2.3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 50, 50, 125);

			VectorMA(vecSrc, 2048, forward, vecEnd);

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);

			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();

			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex(EGON_BEAM_SPRITE);

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			//if ( IEngineStudio.IsHardware() )
			{
				r /= 255.0f;
				g /= 255.0f;
				b /= 255.0f;
			}


			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint(idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b);

			if (pBeam)
				pBeam->flags |= (FBEAM_SINENOISE);

			pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint(idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b);

			// Vit_amiN: egon beam flare
			pFlare = gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 1.0,
				gEngfuncs.pEventAPI->EV_FindModelIndex(EGON_FLARE_SPRITE),
				kRenderGlow, kRenderFxNoDissipation, 1.0, 99999, FTENT_SPRCYCLE | FTENT_PERSIST);
		}
	}
	

	if (pFlare)	// Vit_amiN: store the last mode for EV_EgonStop()
	{
		pFlare->tentOffset.x = (iFireMode == FIRE_WIDE) ? 1.0f : 0.0f;
	}
}

void EV_EgonStop(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy(args->origin, origin);

	gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, EGON_SOUND_RUN);

	if (args->iparam1)
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100);

	if (EV_IsLocal(idx))
	{
		if (pBeam)
		{
			pBeam->die = 0.0;
			pBeam = NULL;
		}


		if (pBeam2)
		{
			pBeam2->die = 0.0;
			pBeam2 = NULL;
		}

		if (pFlare)	// Vit_amiN: egon beam flare
		{
			pFlare->die = gEngfuncs.GetClientTime();

			if (gEngfuncs.GetMaxClients() == 1 || !(pFlare->flags & FTENT_NOMODEL))
			{
				if (pFlare->tentOffset.x != 0.0f)	// true for iFireMode == FIRE_WIDE
				{
					pFlare->callback = &EV_EgonFlareCallback;
					pFlare->fadeSpeed = 2.0;			// fade out will take 0.5 sec
					pFlare->tentOffset.x = 10.0;		// scaling speed per second
					pFlare->tentOffset.y = 0.1;			// min time between two scales
					pFlare->tentOffset.z = pFlare->die;	// the last callback run time
					pFlare->flags = FTENT_FADEOUT | FTENT_CLIENTCUSTOM;
				}
			}

			pFlare = NULL;
		}
	}
}
//======================
//	    EGON END 
//======================

//======================
//	   HORNET START
//======================
void EV_HornetGunFire(event_args_t* args)
{
	int idx, iFireMode;
	Vector origin, angles, vecSrc, forward, right, up;

	idx = args->entindex;
	VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	iFireMode = args->iparam1;

	//Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
	{
		V_PunchAxis(0, gEngfuncs.pfnRandomLong(0, 2));
		gEngfuncs.pEventAPI->EV_WeaponAnimation(HGUN_SHOOT, 1);
	}

	switch (gEngfuncs.pfnRandomLong(0, 2))
	{
	case 0:	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire1.wav", 1, ATTN_NORM, 0, 100);	break;
	case 1:	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire2.wav", 1, ATTN_NORM, 0, 100);	break;
	case 2:	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire3.wav", 1, ATTN_NORM, 0, 100);	break;
	}
}
//======================
//	   HORNET END
//======================

//======================
//	   TRIPMINE START
//======================
//We only check if it's possible to put a trip mine
//and if it is, then we play the animation. Server still places it.
void EV_TripmineFire(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy(args->origin, vecSrc);
	VectorCopy(args->angles, angles);

	AngleVectors(angles, forward, NULL, NULL);

	if (!EV_IsLocal(idx))
		return;

	// Grab predicted result for local player
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);

	vecSrc = vecSrc + view_ofs;

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecSrc + forward * 128, PM_NORMAL, -1, &tr);

	//Hit something solid
	if (tr.fraction < 1.0)
		gEngfuncs.pEventAPI->EV_WeaponAnimation(TRIPMINE_DRAW, 0);

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   TRIPMINE END
//======================

//======================
//	   SQUEAK START
//======================
void EV_SnarkFire(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy(args->origin, vecSrc);
	VectorCopy(args->angles, angles);

	AngleVectors(angles, forward, NULL, NULL);

	if (!EV_IsLocal(idx))
		return;

	if (args->ducking)
		vecSrc = vecSrc - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr);

	//Find space to drop the thing.
	if (tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25)
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SQUEAK_THROW, 0);

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

void EV_FireEagle(event_args_t* args)
{
	const bool bEmpty = args->bparam1 != 0;

	int	idx = args->entindex;

	Vector up, right, forward;

	AngleVectors(args->angles, forward, right, up);

	const int iShell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");

	if (EV_IsLocal(args->entindex))
	{
		EV_MuzzleFlash();

		gEngfuncs.pEventAPI->EV_WeaponAnimation(bEmpty ? EAGLE_SHOOT_EMPTY : EAGLE_SHOOT, 0);
		V_PunchAxis(0, -4.0);
	}

	Vector ShellVelocity;
	Vector ShellOrigin;

	EV_GetDefaultShellInfo(
		args,
		args->origin, args->velocity,
		ShellVelocity,
		ShellOrigin,
		forward, right, up,
		-9.0, 14.0, 9.0);

	EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[1], iShell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(
		args->entindex,
		args->origin, CHAN_WEAPON, "weapons/desert_eagle_fire.wav",
		gEngfuncs.pfnRandomFloat(0.92, 1), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	Vector vecSrc;

	EV_GetGunPosition(args, vecSrc, args->origin);
	EV_HLDM_MuzzleFlash(vecSrc, 2.3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	Vector vecAiming = forward;

	EV_HLDM_FireBullets(
		args->entindex,
		forward, right, up,
		1,
		vecSrc, vecAiming,
		8192.0,
		BULLET_PLAYER_EAGLE,
		1, &tracerCount[idx - 1],
		args->fparam1, args->fparam2);
}

//======================
//	PIPE WRENCH START
//======================
int g_iClub;

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Pipewrench(event_args_t* args)
{
	const int idx = args->entindex;
	Vector origin = args->origin;
	const int iBigSwing = args->bparam1;
	const int hitSomething = args->bparam2;

	if (!EV_IsLocal(idx))
	{
		return;
	}

	//Play Swing sound
	if (iBigSwing)
	{
		if (hitSomething)
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_BIG_SWING_HIT, 0);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_BIG_SWING_MISS, 0);
		}

		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, PITCH_NORM);
	}
	else
	{
		if (hitSomething)
		{
			switch (g_iClub % 3)
			{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK1HIT, 0); break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK2HIT, 0); break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK3HIT, 0); break;
			}
		}
		else
		{
			switch (g_iClub % 3)
			{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK1MISS, 0); break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK2MISS, 0); break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK3MISS, 0); break;
			}

			switch (g_iClub % 2)
			{
			case 0: gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
			case 1: gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_miss2.wav", 1, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}

		++g_iClub;
	}
}
//======================
//	 PIPE WRENCH END 
//======================

void EV_FireM249(event_args_t* args)
{
	int iBody = args->iparam1;

	int idx = args->entindex;

	const bool bAlternatingEject = args->bparam1 != 0;

	Vector up, right, forward;

	AngleVectors(args->angles, forward, right, up);

	int iShell =
		bAlternatingEject ?
		gEngfuncs.pEventAPI->EV_FindModelIndex("models/saw_link.mdl") :
		gEngfuncs.pEventAPI->EV_FindModelIndex("models/saw_shell.mdl");

	if (EV_IsLocal(args->entindex))
	{
		SetLocalBody(WEAPON_M249, iBody);
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(0, 2) + M249_SHOOT1, iBody);
		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
		V_PunchAxis(1, gEngfuncs.pfnRandomFloat(-1, 1));
	}

	Vector ShellVelocity;
	Vector ShellOrigin;

	EV_GetDefaultShellInfo(
		args,
		args->origin, args->velocity,
		ShellVelocity,
		ShellOrigin,
		forward, right, up,
		-28.0, 24.0, 4.0);

	EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[1], iShell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(
		args->entindex,
		args->origin, CHAN_WEAPON, "weapons/saw_fire1.wav",
		VOL_NORM, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));

	Vector vecSrc;

	EV_GetGunPosition(args, vecSrc, args->origin);

	EV_HLDM_MuzzleFlash(vecSrc, 2.5 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	Vector vecAiming = forward;

	EV_HLDM_FireBullets(
		args->entindex,
		forward, right, up,
		1,
		vecSrc, vecAiming,
		8192.0,
		BULLET_PLAYER_556,
		1, &tracerCount[idx - 1],
		args->fparam1, args->fparam2);
}

void EV_FireDisplacer(event_args_t* args)
{
	const auto mode = static_cast<DisplacerMode>(args->iparam1);

	switch (mode)
	{
	case DisplacerMode::SPINNING_UP:
	{
		int iAttach = 0;

		int iStartAttach, iEndAttach;

		for (size_t uiIndex = 0; uiIndex < DISPLACER_NUM_BEAMS; ++uiIndex)
		{
			if (iAttach <= 2)
			{
				iStartAttach = iAttach++ + 2;
				iEndAttach = iAttach % 2 + 2;
			}
			else
			{
				iStartAttach = 0;
				iEndAttach = 0;
			}

			gEngfuncs.pEfxAPI->R_BeamEnts(
				args->entindex | (iStartAttach << 12), args->entindex | (iEndAttach << 12),
				gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/lgtning.spr"),
				1,
				1, 60 * 0.01, 190 / 255.0, 30, 0, 10,
				96 / 255.0, 128 / 255.0, 16 / 255.0);
		}

		break;
	}

	case DisplacerMode::FIRED:
	{
		//bparam1 indicates whether it's a primary or secondary attack. - Solokiller
		if (!args->bparam1)
		{
			gEngfuncs.pEventAPI->EV_PlaySound(
				args->entindex, args->origin,
				CHAN_WEAPON, "weapons/displacer_fire.wav",
				gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_PlaySound(
				args->entindex, args->origin,
				CHAN_WEAPON, "weapons/displacer_self.wav",
				gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		}
		
		if (EV_IsLocal(args->entindex))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(DISPLACER_FIRE, 0);
			V_PunchAxis(0, -2);
		}

		EV_HLDM_MuzzleFlash(args->origin, 3.5 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 96, 128, 16);

		break;
	}

	default: break;
	}
}

void EV_FireShockRifle(event_args_t* args)
{
	gEngfuncs.pEventAPI->EV_PlaySound(args->entindex, args->origin, CHAN_WEAPON, "weapons/shock_fire.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(args->entindex))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOCKRIFLE_FIRE, 0);
	EV_HLDM_MuzzleFlash(args->origin, 2.3 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 0, 253, 253);

	for (size_t uiIndex = 0; uiIndex < 3; ++uiIndex)
	{
		gEngfuncs.pEfxAPI->R_BeamEnts(
			args->entindex | 0x1000, args->entindex | ((uiIndex + 2) << 12),
			gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/lgtning.spr"),
			0.08,
			1, 75 * 0.01, 190 / 255.0, 30, 0, 10,
			0, 253 / 255.0, 253 / 255.0);
	}

}

void EV_FireSpore(event_args_t* args)
{
	gEngfuncs.pEventAPI->EV_PlaySound(
		args->entindex, args->origin,
		CHAN_WEAPON, "weapons/splauncher_fire.wav",
		0.9,
		ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(args->entindex))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SPLAUNCHER_FIRE, 0);

		EV_HLDM_MuzzleFlash(args->origin, 2 + gEngfuncs.pfnRandomFloat(-0.2, 0.2), 30, 255, 30);

		V_PunchAxis(0, -3.0);

		if (cl_entity_t* pViewModel = gEngfuncs.GetViewModel())
		{
			Vector vecSrc = pViewModel->attachment[1];

			Vector forward;

			AngleVectors(args->angles, forward, nullptr, nullptr);

			gEngfuncs.pEfxAPI->R_Sprite_Spray(
				vecSrc, forward,
				gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/tinyspit.spr"),
				10, 10, 180);
		}
	}
}

void EV_SniperRifle(event_args_t* args)
{
	const int idx = args->entindex;
	Vector vecOrigin = args->origin;
	Vector vecAngles = args->angles;

	const int iClip = args->iparam1;

	Vector up, right, forward;

	AngleVectors(vecAngles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(iClip <= 0 ? SNIPERRIFLE_FIRELASTROUND : SNIPERRIFLE_FIRE, 0);
		V_PunchAxis(0, -2.0);
	}

	gEngfuncs.pEventAPI->EV_PlaySound(idx, vecOrigin,
		CHAN_WEAPON, "weapons/sniper_fire.wav",
		gEngfuncs.pfnRandomFloat(0.9f, 1.0f), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	Vector vecSrc;
	Vector vecAiming = forward;

	EV_GetGunPosition(args, vecSrc, vecOrigin);

	EV_HLDM_MuzzleFlash(vecSrc, 2 + gEngfuncs.pfnRandomFloat(-0.2, 0.2));

	EV_HLDM_FireBullets(
		idx,
		forward,
		right,
		up,
		1,
		vecSrc,
		vecAiming,
		8192.0,
		BULLET_PLAYER_762,
		1,
		&tracerCount[idx - 1],
		args->fparam1,
		args->fparam2);
}

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Knife(event_args_t* args)
{
	const int idx = args->entindex;
	Vector origin = args->origin;

	const char* pszSwingSound;

	switch (g_iSwing)
	{
	default:
	case 0: pszSwingSound = "weapons/knife1.wav"; break;
	case 1: pszSwingSound = "weapons/knife2.wav"; break;
	case 2: pszSwingSound = "weapons/knife3.wav"; break;
	}

	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, pszSwingSound, 1, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(idx))
	{
		switch ((g_iSwing++) % 3)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK1MISS, 0); break;
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK2, 0); break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK3, 0); break;
		}
	}
}

void EV_PenguinFire(event_args_t* args)
{
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector forward;
	gEngfuncs.pfnAngleVectors(angles, forward, nullptr, nullptr);

	if (EV_IsLocal(args->entindex))
	{
		if (args->ducking)
			origin.z += 18;

		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(args->entindex - 1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);

		Vector start = origin + (forward * 20);
		Vector end = origin + (forward * 64);

		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_NORMAL, -1, &tr);

		if (!tr.allsolid && !tr.startsolid && tr.fraction > 0.25)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PENGUIN_THROW, 0);

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}
//=============================================
// CHUMMY START
//=============================================

void EV_FireChumtoad(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy(args->origin, vecSrc);
	VectorCopy(args->angles, angles);

	AngleVectors(angles, forward, NULL, NULL);

	if (!EV_IsLocal(idx))
		return;

	if (args->ducking)
		vecSrc = vecSrc - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr);

	//Find space to drop the thing.
	if (tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25)
		gEngfuncs.pEventAPI->EV_WeaponAnimation(VCHUB_THROW, 0);

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//=============================================
// CHUMMY END
//=============================================

void EV_TrainPitchAdjust(event_args_t* args)
{
	int idx;
	Vector origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy(args->origin, origin);

	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)(us_params & 0x003f) / 40.0;
	noise = (int)(((us_params) >> 12) & 0x0007);
	pitch = (int)(10.0 * (float)((us_params >> 6) & 0x003f));

	switch (noise)
	{
	case 1: strcpy(sz, "plats/ttrain1.wav"); break;
	case 2: strcpy(sz, "plats/ttrain2.wav"); break;
	case 3: strcpy(sz, "plats/ttrain3.wav"); break;
	case 4: strcpy(sz, "plats/ttrain4.wav"); break;
	case 5: strcpy(sz, "plats/ttrain6.wav"); break;
	case 6: strcpy(sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy(sz, "");
		return;
	}

	if (stop)
	{
		gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, sz);
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch);
	}
}

int EV_TFC_IsAllyTeam(int iTeam1, int iTeam2)
{
	return 0;
}