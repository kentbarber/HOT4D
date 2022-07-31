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
	if (id[0].id == OD_CURRENTTIME)
	{
		return !bc->GetBool(OD_AUTO_ANIM_TIME);
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
	maxon::Bool doAutoTime = bc->GetBool(OD_AUTO_ANIM_TIME);
	if (doAutoTime)
	{
		BaseTime btCurrentTime;
		maxon::Float    currentFrame;
		btCurrentTime = doc->GetTime();
		currentFrame = (maxon::Float)btCurrentTime.GetFrame(doc->GetFps());
		if (_currentTime != currentFrame)
		{
			_currentTime = currentFrame;
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

	bc->SetInt32(OD_OCEAN_RESOLUTION, 7);
	bc->SetInt32(OD_SEED, 12345);
	bc->SetFloat(OD_OCEAN_SIZE, 400.0);
	bc->SetFloat(OD_WIND_SPEED, 20.0);
	bc->SetFloat(OD_WIND_DIRECTION, 120.0);
	bc->SetFloat(OD_SHRT_WAVELENGHT, 0.01);
	bc->SetFloat(OD_WAVE_HEIGHT, 30.0);
	bc->SetFloat(OD_CHOPAMOUNT, 0.5);
	bc->SetFloat(OD_DAMP_REFLECT, 1.0);
	bc->SetFloat(OD_WIND_ALIGNMENT, 1.0);
	bc->SetFloat(OD_OCEAN_DEPTH, 200.0);
	bc->SetFloat(OD_CURRENTTIME, 0.0);
	bc->SetInt32(OD_TIMELOOP, 90);
	bc->SetFloat(OD_TIMESCALE, 0.5);
	bc->SetBool(OD_AUTO_ANIM_TIME, true);
	bc->SetBool(OD_PRE_RUN_FOAM, false);
	bc->SetBool(OD_DO_CATMU_INTER, false);
	bc->SetBool(OD_DO_JACOBIAN, false);
	bc->SetBool(OD_DO_CHOPYNESS, true);
	bc->SetFloat(OD_PSEL_THRES, 0.1);
	bc->SetFloat(OD_JACOB_THRES, 0.5);
	bc->SetFloat(OD_FOAM_THRES, 0.03);

	bc->SetFloat(ID_MG_BASEEFFECTOR_MINSTRENGTH, -1.0);
	bc->SetBool(ID_MG_BASEEFFECTOR_POSITION_ACTIVE, true);
	bc->SetVector(ID_MG_BASEEFFECTOR_POSITION, Vector(50.0));
	
	iferr_scope_handler {
		err.DiagOutput();
		return false;
	};

	if (_oceanSimulationRef == nullptr)
	{
		_oceanSimulationRef = OceanSimulation::Ocean().Create() iferr_return;
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

	if (_oceanSimulationRef == nullptr)
	{
		_oceanSimulationRef = OceanSimulation::Ocean().Create() iferr_return;
	}

	Int32 oceanResolution = 1 << bc->GetInt32(OD_OCEAN_RESOLUTION);
	Float oceanSize = bc->GetFloat(OD_OCEAN_SIZE);
	Float shrtWaveLenght = bc->GetFloat(OD_SHRT_WAVELENGHT);
	Float waveHeight =  bc->GetFloat(OD_WAVE_HEIGHT);
	Float windSpeed = bc->GetFloat(OD_WIND_SPEED);
	Float windDirection = DegToRad(bc->GetFloat(OD_WIND_DIRECTION));
	Float windAlign = bc->GetFloat(OD_WIND_ALIGNMENT);
	Float dampReflection = bc->GetFloat(OD_DAMP_REFLECT);
	Int32 seed = bc->GetInt32(OD_SEED);
	Float oceanDepth = bc->GetFloat(OD_OCEAN_DEPTH);
	Float chopAmount = bc->GetFloat(OD_CHOPAMOUNT);
	Int32 timeLoop = bc->GetInt32(OD_TIMELOOP);
	Float timeScale = bc->GetFloat(OD_TIMESCALE);
	Bool doAutoTime = bc->GetBool(OD_AUTO_ANIM_TIME);

	if (!doAutoTime)
	{
		_currentTime = bc->GetFloat(OD_CURRENTTIME);
	}

	Bool doChopyness = bc->GetBool(OD_DO_CHOPYNESS);

	if (_oceanSimulationRef.NeedUpdate(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed))
	{
		_oceanSimulationRef.Init(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed) iferr_return;
	}

	_oceanSimulationRef.Animate(_currentTime, timeLoop, timeScale, oceanDepth, chopAmount, true, doChopyness, false, false) iferr_return;
}

maxon::Result<void> OceanSimulationEffector::EvaluatePoint(BaseObject* op, const maxon::Vector p, maxon::Vector &displacement) const
{
	iferr_scope;

	BaseContainer* bc = op->GetDataInstance();
	maxon::Float waveHeight = bc->GetFloat(OD_WAVE_HEIGHT);
	maxon::Bool doCatmuInter = bc->GetBool(OD_DO_CATMU_INTER);

	OceanSimulation::INTERTYPE interType = OceanSimulation::INTERTYPE::LINEAR;
	if (doCatmuInter)
	{
		interType = OceanSimulation::INTERTYPE::CATMULLROM;
	}

	maxon::Vector normal;
	maxon::Float jMinus;
	_oceanSimulationRef.EvaluatePoint(interType, p, displacement, normal, jMinus) iferr_return;
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

