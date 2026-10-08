#pragma once

typedef struct
{
    qboolean (*CL_PrecacheResources)();
} private_funcs_t;

typedef enum
{
    t_sound = 0,
    t_skin,
    t_model,
    t_decal,
    t_generic,
    t_eventscript,
    t_world, // Fake type for world, is really t_model
} resourcetype_t;

typedef struct resource_s
{
    char           szFileName[64]; // File name to download/precache.
    resourcetype_t type;           // t_sound, t_skin, t_model, t_decal.
    int            nIndex;         // For t_decals
    int            nDownloadSize;  // Size in Bytes if this must be downloaded.
    unsigned char  ucFlags;

    // For handling client to client resource propagation
    unsigned char rgucMD5_hash[16]; // To determine if we already have it.
    unsigned char playernum;        // Which player index this resource is associated with, if it's a custom resource.

    unsigned char      rguc_reserved[32]; // For future expansion
    struct resource_s* pNext;             // Next in chain.
    struct resource_s* pPrev;
} resource_t;

extern resource_t* cl_resourcesonhand;

//Resolve a gamedata symbol for a module. A miss is fatal: it reports the symbol,
//its owning module, the engine buildnum, the module CRC64 and the host's reason.
//moduleName only labels the diagnostic; the lookup itself uses moduleBase.
PVOID GamedataResolvePtr(PVOID moduleBase, const char* moduleName, const char* symbolName, mh_gamesymbol_kind_t kind);

void Engine_FillAddress_CL_ResourceOnHand(void);
void Engine_FillAddress(void);
