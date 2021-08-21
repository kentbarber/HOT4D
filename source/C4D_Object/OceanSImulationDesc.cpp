#include "c4d.h"
#include "c4d_symbols.h"
#include "description/OceanDescription.h"
#include "main.h"

#define GLD_ID_OCEAN_DESCRIPTION 1057478

Bool RegisterOceanSimulationDescription()
{
	return RegisterDescription(GLD_ID_OCEAN_DESCRIPTION, "OceanDescription"_s);
}
