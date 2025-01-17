//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "threads.h"


#define DEFAULTLIGHTLEVEL	300

typedef struct entity_s
{
	char	classname[64];
	Vector	origin;
	float	angle;
	Vector	light;
	int		style;
	qboolean	targetent;
	Vector	targetorigin;
} lightentity_t;

extern	lightentity_t	lightentities[MAX_MAP_ENTITIES];
extern	int		numlightentities;

#define	ON_EPSILON	0.1

#define	MAXLIGHTS			1024

void LoadNodes (char *file);
qboolean TestLine (Vector start, Vector stop);

void LightFace (int surfnum);
void LightLeaf (dleaf_t *leaf);

void MakeTnodes (dmodel_t *bm);

extern	float		scaledist;
extern	float		scalecos;
extern	float		rangescale;

extern	int		c_culldistplane, c_proper;

byte *GetFileSpace (int size);
extern	byte		*filebase;

extern	Vector	bsp_origin;
extern	Vector	bsp_xvector;
extern	Vector	bsp_yvector;

void TransformSample (Vector in, Vector out);
void RotateSample (Vector in, Vector out);

extern	qboolean	extrasamples;

extern	float		minlights[MAX_MAP_FACES];
