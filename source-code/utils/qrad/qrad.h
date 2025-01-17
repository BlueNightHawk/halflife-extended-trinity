/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/


#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "polylib.h"
#include "threads.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#pragma warning(disable: 4142 4028)
#define filelength IO_filelength
#include <io.h>
#undef filelength
#pragma warning(default: 4142 4028)

#include <fcntl.h>
#include <direct.h>
#include <ctype.h>

typedef enum
{
	emit_surface,
	emit_point,
	emit_spotlight,
	emit_skylight
} emittype_t;



typedef struct directlight_s
{
	struct directlight_s *next;
	emittype_t	type;
    int			style;
	Vector		origin;
	Vector		intensity;
	Vector		normal;		// for surfaces and spotlights
	float		stopdot;		// for spotlights
	float		stopdot2;		// for spotlights
} directlight_t;


#define	TRANSFER_SCALE (1.0f/16384)
#define	INVERSE_TRANSFER_SCALE	16384

typedef struct
{
	unsigned short	patch;
	unsigned short	transfer;
} transfer_t;


#define	MAX_PATCHES	65536

typedef struct patch_s
{
	winding_t	*winding;
	Vector		mins, maxs, face_mins, face_maxs;
	struct patch_s		*next;		// next in face
	int			numtransfers;
	transfer_t	*transfers;
	Vector		origin;
	Vector		normal;

	dplane_t	*plane;

	float		chop;				// smallest acceptable width of patch face
	float		scale[2];			// Scaling of texture in s & t

	qboolean	sky;

	Vector		totallight;			// accumulated by radiosity
									// does NOT include light
									// accounted for by direct lighting
	Vector		baselight;			// emissivity only
	Vector		directlight;		// direct light value
	float		area;

	Vector		reflectivity;		// Average RGB of texture, modified by material type.

	Vector		samplelight;
	int			samples;		// for averaging direct light
	int			faceNumber;
} patch_t;

extern	patch_t		*face_patches[MAX_MAP_FACES];
extern	entity_t	*face_entity[MAX_MAP_FACES];
extern	Vector		face_offset[MAX_MAP_FACES];		// for rotating bmodels
extern  Vector		face_centroids[MAX_MAP_EDGES];
extern	patch_t		patches[MAX_PATCHES];
extern	unsigned	num_patches;

extern	int		leafparents[MAX_MAP_LEAFS];
extern	int		nodeparents[MAX_MAP_NODES];

extern	float	lightscale;
extern	float	dlight_threshold;
extern  float	coring;

void MakeShadowSplits (void);

//==============================================

_int64 getfreespace(char *filepath);
long getfilesize(char *filename);
time_t getfiletime(char *filename);

void BuildVisMatrix (void);
void FreeVisMatrix (void);
qboolean CheckVisBit (int p1, int p2);
void TouchVMFFile (void);

//==============================================

extern  qboolean extra;
extern	Vector ambient;
extern  float maxlight;
extern	unsigned numbounce;
extern	directlight_t	*directlights[MAX_MAP_LEAFS];
extern	byte	nodehit[MAX_MAP_NODES];
extern  float	gamma;
extern	float	indirect_sun;
extern	float	smoothing_threshold;

void MakeTnodes (dmodel_t *bm);
void PairEdges (void);
qboolean IsIncremental(char *filename);
int SaveIncremental(char *filename);
int PartialHead (void);
void BuildFacelights (int facenum);
void PrecompLightmapOffsets();
void FinalLightFace (int facenum);
void PvsForOrigin (Vector org, byte *pvs);
int TestLine_r (int node, Vector start, Vector stop);
void CreateDirectLights (void);
void DeleteDirectLights (void);
int ProgressiveRefinement (void);
vec_t PatchPlaneDist( patch_t *patch );
void GetPhongNormal( int facenum, Vector spot, Vector phongnormal );

dleaf_t		*PointInLeaf (Vector point);
