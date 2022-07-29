/*
 *  main.cpp
 *
 *  Created by Manuel MAGALHAES on 23/12/10.
 *  Copyright 2010 Valkaari. All rights reserved.
 * 
 *  Modified by Kent Barber on 29/7/2022.
 *  Copyright 2022 GameLogicDesign Limited. All rights reserved.
 *
 */

#include "c4d.h"
#include "main.h"

Bool PluginStart()
{
	if (!RegisterOceanSimulationDescription())
		return false;
	if (!RegisterOceanSimulationDeformer())
		return false;
	if (!RegisterOceanSimulationEffector())
		return false;

	return true;
}

void PluginEnd()
{

}

Bool PluginMessage(Int32 id, void *data)
{
	// use the following lines to set a plugin priority
	//
	switch (id)
	{
	case C4DPL_INIT_SYS:
		if (!g_resource.Init())
			return false; // don't start plugin without resource
		return true;

	case C4DMSG_PRIORITY:
		return true;
	}

	return false;
}
