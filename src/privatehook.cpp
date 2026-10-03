#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

static_assert(METAHOOK_API_VERSION >= 109, "PrecacheManager resolves the engine cl_resourcesonhand global from gamedata and requires MetaHook API 109 (ResolveGameSymbol)");

private_funcs_t gPrivateFuncs = {0};

// A gamedata miss is fatal, so print the diagnostics needed to triage it: the
// symbol and its owning module, the engine buildnum, the consuming module's
// CRC64 (the engine identity even for builds the catalog does not recognize)
// and the host's reason for the failure.
static void ReportSymbolFailure(PVOID moduleBase, const char* moduleName, const char* symbolName, mh_gamesymbol_status_t status)
{
	uint64_t crc64 = 0;
	mh_gamesymbol_status_t crcSt = g_pMetaHookAPI->GetModuleCRC64(moduleBase, &crc64);

	if (crcSt == MH_GAMESYMBOL_OK)
	{
		Sys_Error("Failed to resolve \"%s\" (module %s)\nEngine buildnum: %d\nCRC64: %016llx\nReason: %s",
			symbolName, moduleName, g_dwEngineBuildnum, (unsigned long long)crc64, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
	else
	{
		Sys_Error("Failed to resolve \"%s\" (module %s)\nEngine buildnum: %d\nReason: %s",
			symbolName, moduleName, g_dwEngineBuildnum, g_pMetaHookAPI->GetGameSymbolStatusString(status));
	}
}

PVOID GamedataResolvePtr(PVOID moduleBase, const char* moduleName, const char* symbolName, mh_gamesymbol_kind_t kind)
{
	PVOID address = NULL;
	mh_gamesymbol_status_t status = g_pMetaHookAPI->ResolveGameSymbol(moduleBase, symbolName, kind, &address);

	if (status != MH_GAMESYMBOL_OK)
		ReportSymbolFailure(moduleBase, moduleName, symbolName, status);

	return address;
}

void Engine_FillAddress_CL_ResourceOnHand(void)
{
	// The gamedata global is the list sentinel node itself, so the resolved address
	// is exactly what FS_Dump_Precaches walks: start at ->pNext and stop when the
	// walk returns here. No extra dereference.
	cl_resourcesonhand = (decltype(cl_resourcesonhand))
		GamedataResolvePtr(g_EngineDLLInfo.ImageBase, "engine", "cl_resourcesonhand", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress(void)
{
	Engine_FillAddress_CL_ResourceOnHand();
}
