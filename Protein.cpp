/*
 * Protein.cpp
 *
 * Copyright (C) 2009 by VISUS (Universitaet Stuttgart)
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"

#include "Protein.h"
#include "api/MegaMolCore.std.h"

#include "ProteinRendererCartoon.h"
#include "ProteinRenderer.h"
#include "ProteinRendererSES.h"
#include "ProteinRendererBDP.h"
#include "ProteinVolumeRenderer.h"
#include "ProteinMovementRenderer.h"
#include "ProteinRendererCBOpenCL.h"
#include "SolventRenderer.h"
#include "SimpleMoleculeRenderer.h"
#include "SphereRenderer.h"
#include "MoleculeSESRenderer.h"
#include "MoleculeCartoonRenderer.h"

#include "ProteinData.h"
#include "PDBLoader.h"
#include "NetCDFData.h"
#include "ProteinMovementData.h"
#include "CCP4VolumeData.h"
#include "CoarseGrainDataLoader.h"

#include "CallProteinData.h"
#include "CallFrame.h"
#include "CallProteinMovementData.h"
#include "CallVolumeData.h"
#include "MolecularDataCall.h"
#include "SphereDataCall.h"

#include "CallAutoDescription.h"
#include "ModuleAutoDescription.h"
#include "vislib/vislibversion.h"

#include "vislib/Log.h"
#include "vislib/ThreadSafeStackTrace.h"

#include "SolPathDataCall.h"
#include "SolPathDataSource.h"
#include "SolPathRenderer.h"


/*
 * mmplgPluginAPIVersion
 */
PROTEIN_API int mmplgPluginAPIVersion(void) {
    return 100;
}


/*
 * mmplgPluginName
 */
PROTEIN_API const char * mmplgPluginName(void) {
    return "Protein";
}


/*
 * mmplgPluginDescription
 */
PROTEIN_API const char * mmplgPluginDescription(void) {
    return "Plugin for protein rendering (SFB716 D4)";
}


/*
 * mmplgCoreCompatibilityValue
 */
PROTEIN_API const void * mmplgCoreCompatibilityValue(void) {
    static const mmplgCompatibilityValues compRev = {
        sizeof(mmplgCompatibilityValues),
        MEGAMOL_CORE_COMP_REV,
        VISLIB_VERSION_REVISION
    };
    return &compRev;
}


/*
 * mmplgModuleCount
 */
PROTEIN_API int mmplgModuleCount(void) {
	int moduleCount = 18;
#if (defined(WITH_NETCDF) && (WITH_NETCDF))
    moduleCount++;
#endif /* (defined(WITH_NETCDF) && (WITH_NETCDF)) */
#if (defined(WITH_OPENCL) && (WITH_OPENCL))
    moduleCount++;
#endif /* (defined(WITH_OPENCL) && (WITH_OPENCL)) */
    return moduleCount;
}


/*
 * mmplgModuleDescription
 */
PROTEIN_API void* mmplgModuleDescription(int idx) {
    switch (idx) {
        case 0: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinData>();
        case 1: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinRenderer>();
        case 2: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinRendererCartoon>();
        case 3: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinRendererSES>();
        case 4: return new megamol::core::ModuleAutoDescription<megamol::protein::SolPathDataSource>();
        case 5: return new megamol::core::ModuleAutoDescription<megamol::protein::SolPathRenderer>();
        case 6: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinRendererBDP>();
		case 7: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinVolumeRenderer>();
        case 8: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinMovementData>();
		case 9: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinMovementRenderer>();
        case 10: return new megamol::core::ModuleAutoDescription<megamol::protein::CCP4VolumeData>();
        case 11: return new megamol::core::ModuleAutoDescription<megamol::protein::SolventRenderer>();
        case 12: return new megamol::core::ModuleAutoDescription<megamol::protein::PDBLoader>();
        case 13: return new megamol::core::ModuleAutoDescription<megamol::protein::SimpleMoleculeRenderer>();
        case 14: return new megamol::core::ModuleAutoDescription<megamol::protein::CoarseGrainDataLoader>();
        case 15: return new megamol::core::ModuleAutoDescription<megamol::protein::SphereRenderer>();
        case 16: return new megamol::core::ModuleAutoDescription<megamol::protein::MoleculeSESRenderer>();
        case 17: return new megamol::core::ModuleAutoDescription<megamol::protein::MoleculeCartoonRenderer>();
#if (defined(WITH_NETCDF) && (WITH_NETCDF))
        case 18: return new megamol::core::ModuleAutoDescription<megamol::protein::NetCDFData>();
		#define NETCDF_OFFSET 1
#else
		#define NETCDF_OFFSET 0
#endif /* (defined(WITH_NETCDF) && (WITH_NETCDF)) */
#if (defined(WITH_OPENCL) && (WITH_OPENCL))
		case 18 + NETCDF_OFFSET: return new megamol::core::ModuleAutoDescription<megamol::protein::ProteinRendererCBOpenCL>();
		#define OPENCL_OFFSET 1
#else
		#define OPENCL_OFFSET 0
#endif /* (defined(WITH_OPENCL) && (WITH_OPENCL)) */
        default: return NULL;
    }
    return NULL;
}


/*
 * mmplgCallCount
 */
PROTEIN_API int mmplgCallCount(void) {
    return 7;
}


/*
 * mmplgCallDescription
 */
PROTEIN_API void* mmplgCallDescription(int idx) {
    switch (idx) {
        case 0: return new megamol::core::CallAutoDescription<megamol::protein::CallProteinData>();
        case 1: return new megamol::core::CallAutoDescription<megamol::protein::CallFrame>();
        case 2: return new megamol::core::CallAutoDescription<megamol::protein::SolPathDataCall>();
        case 3: return new megamol::core::CallAutoDescription<megamol::protein::CallProteinMovementData>();
        case 4: return new megamol::core::CallAutoDescription<megamol::protein::CallVolumeData>();
        case 5: return new megamol::core::CallAutoDescription<megamol::protein::MolecularDataCall>();
        case 6: return new megamol::core::CallAutoDescription<megamol::protein::SphereDataCall>();
        default: return NULL;
    }
    return NULL;
}


/*
 * mmplgConnectStatics
 */
PROTEIN_API bool mmplgConnectStatics(int which, void* value) {
    static vislib::sys::Log::EchoTargetRedirect etr(NULL);
    switch (which) {

        case 1: // vislib::log
            etr.SetTarget(static_cast<vislib::sys::Log*>(value));
            vislib::sys::Log::DefaultLog.SetLogFileName(static_cast<const char*>(NULL), false);
            vislib::sys::Log::DefaultLog.SetLevel(vislib::sys::Log::LEVEL_NONE);
            vislib::sys::Log::DefaultLog.SetEchoOutTarget(&etr);
            vislib::sys::Log::DefaultLog.SetEchoLevel(vislib::sys::Log::LEVEL_ALL);
            vislib::sys::Log::DefaultLog.EchoOfflineMessages(true);
            return true;

        case 2: // vislib::stacktrace
            return vislib::sys::ThreadSafeStackTrace::Initialise(
                *static_cast<const vislib::SmartPtr<vislib::StackTrace>*>(value), true);

    }
    return false;
}
