//  
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//
//  Modified by Kent Barber on 29 /07/22.
//  Copyright (c) 2022 GameLogicDesign Limited.All rights reserved.
//

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_baseeffectordata.h"
#include "OceanDescription.h"
#include "OceanSimulation_decl.h"
#include "OceanSimulationEffector.h"
#include "main.h"

#define GLD_ID_OCEAN_SIMULATION_EFFECTOR 1057480

Bool OceanSimulationEffector::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
	BaseObject* op = (BaseObject*)node;
	BaseContainer* bc = op->GetDataInstance();
	if (id[0].id == CURRENTTIME)
	{
		return !bc->GetBool(AUTO_ANIM_TIME);
	}

	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);
}

Bool OceanSimulationEffector::AddToExecution(BaseObject *op, PriorityList *list)
{
	list->Add(op, EXECUTIONPRIORITY_EXPRESSION, EXECUTIONFLAGS::NONE);
	return true;
}

EXECUTIONRESULT OceanSimulationEffector::Execute(BaseObject *op, BaseDocument *doc, BaseThread *bt, Int32 priority, EXECUTIONFLAGS flags)
{
	if (priority != EXECUTIONPRIORITY_EXPRESSION)
		return EXECUTIONRESULT::OK;

	BaseContainer* bc = op->GetDataInstance();
	maxon::Bool doAutoTime = bc->GetBool(AUTO_ANIM_TIME);
	if (doAutoTime)
	{
		BaseTime btCurrentTime;
		maxon::Float    currentFrame;
		btCurrentTime = doc->GetTime();
		currentFrame = (maxon::Float)btCurrentTime.GetFrame(doc->GetFps());
		if (currentTime_ != currentFrame)
		{
			currentTime_ = currentFrame;
			op->SetDirty(DIRTYFLAGS::DATA);
		}
	}

	return EXECUTIONRESULT::OK;
}

Bool OceanSimulationEffector::InitEffector(GeListNode* node)
{
	BaseObject *op = (BaseObject*)node;
	if (!op)
		return false;

	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;

	bc->SetInt32(OCEAN_RESOLUTION, 7);
	bc->SetInt32(SEED, 12345);
	bc->SetFloat(OCEAN_SIZE, 400.0);
	bc->SetFloat(WIND_SPEED, 20.0);
	bc->SetFloat(WIND_DIRECTION, 120.0);
	bc->SetFloat(SHRT_WAVELENGHT, 0.01);
	bc->SetFloat(WAVE_HEIGHT, 30.0);
	bc->SetFloat(CHOPAMOUNT, 0.5);
	bc->SetFloat(DAMP_REFLECT, 1.0);
	bc->SetFloat(WIND_ALIGNMENT, 1.0);
	bc->SetFloat(OCEAN_DEPTH, 200.0);
	bc->SetFloat(CURRENTTIME, 0.0);
	bc->SetInt32(TIMELOOP, 90);
	bc->SetFloat(TIMESCALE, 0.5);
	bc->SetBool(AUTO_ANIM_TIME, true);
	bc->SetBool(PRE_RUN_FOAM, false);
	bc->SetBool(DO_CATMU_INTER, false);
	bc->SetBool(DO_JACOBIAN, false);
	bc->SetBool(DO_CHOPYNESS, true);
	bc->SetFloat(PSEL_THRES, 0.1);
	bc->SetFloat(JACOB_THRES, 0.5);
	bc->SetFloat(FOAM_THRES, 0.03);

	bc->SetFloat(ID_MG_BASEEFFECTOR_MINSTRENGTH, -1.0);
	bc->SetBool(ID_MG_BASEEFFECTOR_POSITION_ACTIVE, true);
	bc->SetVector(ID_MG_BASEEFFECTOR_POSITION, Vector(50.0));
	
	iferr_scope_handler {
		err.DiagOutput();
		return false;
	};

	if (oceanSimulationRef_ == nullptr)
	{
		oceanSimulationRef_ = OceanSimulation::Ocean().Create() iferr_return;
	}

	return true;
}

void OceanSimulationEffector::InitPoints(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, MoData* md, BaseThread* thread)
{
	BaseContainer* bc = op->GetDataInstance();
	if (!bc)
		return;

	iferr_scope_handler {
		err.DiagOutput();
		return;
	};

	if (oceanSimulationRef_ == nullptr)
	{
		oceanSimulationRef_ = OceanSimulation::Ocean().Create() iferr_return;
	}

	Int32 oceanResolution = 1 << bc->GetInt32(OCEAN_RESOLUTION);
	Float oceanSize = bc->GetFloat(OCEAN_SIZE);
	Float shrtWaveLenght = bc->GetFloat(SHRT_WAVELENGHT);
	Float waveHeight =  bc->GetFloat(WAVE_HEIGHT);
	Float windSpeed = bc->GetFloat(WIND_SPEED);
	Float windDirection = DegToRad(bc->GetFloat(WIND_DIRECTION));
	Float windAlign = bc->GetFloat(WIND_ALIGNMENT);
	Float dampReflection = bc->GetFloat(DAMP_REFLECT);
	Int32 seed = bc->GetInt32(SEED);
	Float oceanDepth = bc->GetFloat(OCEAN_DEPTH);
	Float chopAmount = bc->GetFloat(CHOPAMOUNT);
	Int32 timeLoop = bc->GetInt32(TIMELOOP);
	Float timeScale = bc->GetFloat(TIMESCALE);
	Bool doAutoTime = bc->GetBool(AUTO_ANIM_TIME);

	if (!doAutoTime)
	{
		currentTime_ = bc->GetFloat(CURRENTTIME);
	}

	Bool doChopyness = bc->GetBool(DO_CHOPYNESS);

	if (oceanSimulationRef_.NeedUpdate(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed))
	{
		oceanSimulationRef_.Init(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed) iferr_return;
	}

	oceanSimulationRef_.Animate(currentTime_, timeLoop, timeScale, oceanDepth, chopAmount, true, doChopyness, false, false) iferr_return;
}

maxon::Result<void> OceanSimulationEffector::EvaluatePoint(BaseObject* op, const maxon::Vector p, maxon::Vector &displacement) const
{
	iferr_scope;

	BaseContainer* bc = op->GetDataInstance();
	maxon::Float waveHeight = bc->GetFloat(WAVE_HEIGHT);
	maxon::Bool doCatmuInter = bc->GetBool(DO_CATMU_INTER);

	OceanSimulation::INTERTYPE interType = OceanSimulation::INTERTYPE::LINEAR;
	if (doCatmuInter)
	{
		interType = OceanSimulation::INTERTYPE::CATMULLROM;
	}

	maxon::Vector normal;
	maxon::Float jMinus;
	oceanSimulationRef_.EvaluatePoint(interType, p, displacement, normal, jMinus) iferr_return;
	displacement /= waveHeight; // scale down the result by the wavelegnth so the result should be beetween -1 and 1
	return  maxon::OK;
}


void OceanSimulationEffector::CalcPointValue(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight)
{
	iferr_scope_handler
	{
		err.DbgStop();
		return;
	};
	maxon::Vector disp;

	EvaluatePoint(op, globalpos, disp) iferr_return;

	EffectorStrengths* es = (EffectorStrengths*)data->strengths;
	es->pos = disp;
	es->rot = disp;
	es->scale = disp;
}

Vector OceanSimulationEffector::CalcPointColor(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight)
{
	iferr_scope_handler
	{
		err.DbgStop();
		return Vector(0);
	};

	maxon::Vector disp;
	EvaluatePoint(op, globalpos, disp) iferr_return;

	return disp;
}

Bool RegisterOceanSimulationEffector()
{
	return RegisterEffectorPlugin(GLD_ID_OCEAN_SIMULATION_EFFECTOR, "Ocean Simulation Effector"_s, OBJECT_CALL_ADDEXECUTION, OceanSimulationEffector::Alloc, "OOceanEffector"_s, AutoBitmap("hot4D_eff.tif"_s), 0);
}

