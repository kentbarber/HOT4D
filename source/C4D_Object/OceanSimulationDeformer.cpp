/*
 *
 *  Created by Manuel MAGALHAES on 23/12/10.
 *  Copyright 2010 Valkaari. All rights reserved.
 * 
 *  Modified by Kent Barber on 29/07/22.
 *  Copyright 2022 GameLogicDesign Limited. All rights reserved.
 *
 */

#include "c4d.h"
#include "c4d_symbols.h"
#include "ge_prepass.h"

#include "maxon/lib_math.h"

#include "maxon/parallelfor.h"

#include "OceanSimulation/OceanSimulation_decl.h"

#include "OceanSimulationDeformer.h"
#include "OOceanDeformer.h"
#include "description/OceanDescription.h"

#include "main.h"

maxon::Float OceanSimulationDeformer::MapRange(maxon::Float value, const maxon::Float min_input, const maxon::Float max_input, const maxon::Float min_output, const maxon::Float max_output) const
{
	Float inrange = max_input - min_input;

	if (CompareFloatTolerant(value, 0.0))
		value = 0.0;  // Prevent DivByZero error
	else
		value = (value - min_input) / inrange;    // Map input range to [0.0 ... 1.0]

	if (value > max_output)
		return max_output;

	if (value < min_output)
		return min_output;

	return  min_output + (max_output - min_output) * value; // Map to output range and return result
}

Bool OceanSimulationDeformer::Message(GeListNode *node, Int32 type, void *t_data)
{
	BaseObject* op = (BaseObject*)node;
	if (!op)
		return false;

	switch (type)
	{
	case MSG_MENUPREPARE:
	{
		((BaseObject*)node)->SetDeformMode(true);
		break;
	}
	case MSG_DESCRIPTION_COMMAND:
	{
		DescriptionCommand* dc = (DescriptionCommand*)t_data;
		switch (dc->_descId[0].id)
		{
		case OD_CREATE_FOAM_TAGS:
		{
			BaseObject* pParent = op->GetUp();
			if (pParent && pParent->GetType() == Opolygon)
			{
				Int32 pcnt = ToPoint(pParent)->GetPointCount();

				BaseDocument* doc = op->GetDocument();
				if (doc)
				{
					doc->StartUndo();

					VertexColorTag* pJacobTag = VertexColorTag::Alloc(pcnt);
					if (pJacobTag)
					{
						pJacobTag->SetName("Jacob"_s);
						pJacobTag->SetPerPointMode(true);

						doc->AddUndo(UNDOTYPE::NEWOBJ, pJacobTag);
						pParent->InsertTag(pJacobTag);

						doc->AddUndo(UNDOTYPE::CHANGE, op);
						op->SetParameter(OD_JACOBMAP, pJacobTag, DESCFLAGS_SET::FORCESET);
					}

					VertexColorTag* pFoamTag = VertexColorTag::Alloc(pcnt);
					if (pFoamTag)
					{
						pFoamTag->SetName("Foam"_s);
						pFoamTag->SetPerPointMode(true);

						doc->AddUndo(UNDOTYPE::NEWOBJ, pFoamTag);
						pParent->InsertTag(pFoamTag);

						doc->AddUndo(UNDOTYPE::CHANGE, op);
						op->SetParameter(OD_FOAMMAP, pFoamTag, DESCFLAGS_SET::FORCESET);
					}

					doc->SetActiveTag(pFoamTag);

					doc->EndUndo();
					EventAdd();

					pParent->Message(MSG_UPDATE);
					op->Message(MSG_UPDATE);
				}
			}
			else
			{
				MessageDialog("To create the maps please make sure the parent object is editable"_s);
			}
		}
		break;
		}
	}
	default:
		break;
	}

	return true;
}

Bool OceanSimulationDeformer::Init(GeListNode *node)
{
	iferr_scope_handler
	{
		DiagnosticOutput("Error: @", err);
		return false;
	};

	BaseObject		*op = (BaseObject*)node;
	BaseContainer *bc = op->GetDataInstance();

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
	bc->SetBool(OD_ACTIVE_DEFORM, true);

	if (_falloff)
		if (!_falloff->InitFalloff(bc, NULL, op))
			return false;

	return true;
}

Bool OceanSimulationDeformer::GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags)
{
	BaseObject *op = (BaseObject*)node;
	if (!op)
		return false;
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return false;

	if (!description->LoadDescription(op->GetType()))
		return false;

	//---------------------------------
	// Add the falloff interface
	if (_falloff)
	{
		if (!_falloff->SetMode(FIELDS, bc))
			return false; // The falloff parameters have to have been setup before it can be added to the description, this like makes sure of that
		if (!_falloff->AddFalloffToDescription(description, bc, DESCFLAGS_DESC::NONE))
			return false;
	}

	flags |= DESCFLAGS_DESC::LOADED;

	return true;
}

Bool OceanSimulationDeformer::CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn)
{
	OceanSimulationDeformer *df = (OceanSimulationDeformer*)dest;
	if (!df)
		return false;
	if (_falloff && df->_falloff)
		if (!_falloff->CopyTo(df->_falloff))
			return false;
	return ObjectData::CopyTo(dest, snode, dnode, flags, trn);
}

DRAWRESULT OceanSimulationDeformer::Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh)
{
	if (!op->GetDeformMode())
		return DRAWRESULT::SKIP;
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return DRAWRESULT::FAILURE;
	if (_falloff)
		_falloff->Draw(bd, bh, drawpass, bc);
	return ObjectData::Draw(op, drawpass, bd, bh);
}

Bool OceanSimulationDeformer::GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc)
{
	BaseObject* op = (BaseObject*)node;
	if (!op)
		return false;
	BaseContainer* bc = op->GetDataInstance();
	if (!bc)
		return false;

	Bool parentIsPolygon = false;
	BaseObject* pParent = op->GetUp();
	if (pParent && pParent->GetType() == Opolygon)
	{
		parentIsPolygon = true;
	}

	Bool doJacobian = bc->GetBool(OD_DO_JACOBIAN);
	Bool autoAnimTime = bc->GetBool(OD_AUTO_ANIM_TIME);
	Bool chopyness = bc->GetBool(OD_DO_CHOPYNESS);

	switch (id[0].id)
	{
	case OD_CURRENTTIME:
		return !autoAnimTime;
	case OD_JACOBMAP:
	case OD_JACOB_THRES:
	case OD_FOAMMAP:
	case OD_FOAM_THRES:
		return doJacobian && parentIsPolygon;
	case OD_CREATE_FOAM_TAGS:
		return doJacobian;
	case OD_PRE_RUN_FOAM:
		return doJacobian && parentIsPolygon && autoAnimTime;
	case OD_CHOPAMOUNT:
		return chopyness;
	}

	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);
}

Int32 OceanSimulationDeformer::GetHandleCount(BaseObject *op)
{
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return 0;
	if (_falloff)
		return _falloff->GetHandleCount(bc);
	return 0;
}

void OceanSimulationDeformer::GetHandle(BaseObject *op, Int32 i, HandleInfo &info)
{
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return;
	if (_falloff)
		_falloff->GetHandle(i, bc, info);
}

void OceanSimulationDeformer::SetHandle(BaseObject *op, Int32 i, Vector p, const HandleInfo &info)
{
	BaseContainer *bc = op->GetDataInstance();
	if (!bc)
		return;
	if (_falloff)
		_falloff->SetHandle(i, p, bc, info);
}

void OceanSimulationDeformer::CheckDirty(BaseObject* op, BaseDocument* doc)
{
	// fields
	if (_falloff)
	{
		BaseContainer *data = op->GetDataInstance();
		Int32 dirty = _falloff->GetDirty(doc, data);
		if (dirty != _falloffDirtyCheck)
		{
			op->SetDirty(DIRTYFLAGS::DATA);
			_falloffDirtyCheck = dirty;
		}
	}

	// auto time
	maxon::Bool						doAutoTime;
	GeData							data;

	op->GetParameter(DescID(OD_AUTO_ANIM_TIME), data, DESCFLAGS_GET::NONE);
	doAutoTime = data.GetBool();

	if (doAutoTime)
	{
		BaseTime		btCurrentTime = doc->GetTime();
		maxon::Float    currentFrame = btCurrentTime.GetFrame(doc->GetFps());
		if (_currentTime != currentFrame)
		{
			_currentTime = currentFrame;
			op->SetDirty(DIRTYFLAGS::DATA);
		}
	}
}

Bool OceanSimulationDeformer::ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread)
{
	iferr_scope_handler
	{
		return false;
	};

	if (!op->IsInstanceOf(Opoint) || !_falloff)
		return true;

	maxon::Int32                    pcnt;
	GeData							data;
	VertexColorTag					*jacobmaptag = nullptr;
	VertexColorTag					*foammaptag = nullptr;

	VertexColorHandle				jacobpoint = nullptr;
	VertexColorHandle               foampoint = nullptr;

	maxon::Float32                  *weight = nullptr;
	maxon::Float                    pselThres;
	maxon::Float                    jacobThres, foamThres;
	BaseSelect						*bsp = nullptr;
	SelectionTag					*stag = nullptr;
	maxon::Vector					p;
	maxon::Vector					dispvalue;
	maxon::Vector					*padr = nullptr;

	maxon::Float					oceanSize, windSpeed, windDirection, shrtWaveLenght, waveHeight, chopAmount, dampReflection, windAlign, oceanDepth, timeScale;
	maxon::Int32					oceanResolution, seed, timeLoop;
	maxon::Bool						doCatmuInter, doJacobian, doChopyness, doNormals, doAutoTime, preRunFoam;

	padr = ToPoint(op)->GetPointW();
	pcnt = ToPoint(op)->GetPointCount();

	if (!pcnt)
		return true;

	weight = ToPoint(op)->CalcVertexMap(mod);

	finally {
		DeleteMem(weight);
	};

	BaseContainer* bc = mod->GetDataInstance();

	oceanResolution = 1 << bc->GetInt32(OD_OCEAN_RESOLUTION);
	oceanSize = bc->GetFloat(OD_OCEAN_SIZE);
	shrtWaveLenght = bc->GetFloat(OD_SHRT_WAVELENGHT);
	waveHeight = bc->GetFloat(OD_WAVE_HEIGHT);
	windSpeed = bc->GetFloat(OD_WIND_SPEED);
	windDirection = DegToRad(bc->GetFloat(OD_WIND_DIRECTION));
	windAlign = bc->GetFloat(OD_WIND_ALIGNMENT);
	dampReflection = bc->GetFloat(OD_DAMP_REFLECT);
	seed = bc->GetInt32(OD_SEED);
	oceanDepth = bc->GetFloat(OD_OCEAN_DEPTH);
	chopAmount = bc->GetFloat(OD_CHOPAMOUNT);
	timeLoop = bc->GetInt32(OD_TIMELOOP);
	timeScale = bc->GetFloat(OD_TIMESCALE);
	doAutoTime = bc->GetBool(OD_AUTO_ANIM_TIME);

	if (!doAutoTime)
	{
		// currentTime is set in checkDirty
		_currentTime = bc->GetFloat(OD_CURRENTTIME);
	}

	doCatmuInter = bc->GetBool(OD_DO_CATMU_INTER);
	doJacobian = bc->GetBool(OD_DO_JACOBIAN);
	doChopyness = bc->GetBool(OD_DO_CHOPYNESS);
	doNormals = bc->GetBool(OD_DO_NORMALS) && !doChopyness; // why choppyness ???  normals are not used !!
	preRunFoam = bc->GetBool(OD_PRE_RUN_FOAM);
	jacobmaptag = (VertexColorTag*)bc->GetLink(OD_JACOBMAP, doc, Tvertexcolor);
	foammaptag = (VertexColorTag*)bc->GetLink(OD_FOAMMAP, doc, Tvertexcolor);
	stag = (SelectionTag*)bc->GetLink(OD_PSEL_PARTICLES, doc, Tpointselection);
	pselThres = bc->GetFloat(OD_PSEL_THRES);
	jacobThres = bc->GetFloat(OD_JACOB_THRES);
	foamThres = bc->GetFloat(OD_FOAM_THRES);

	maxon::Bool doDeform = bc->GetBool(OD_ACTIVE_DEFORM);

	if (jacobmaptag)
	{

		jacobmaptag->SetPerPointMode(true);


		if (jacobmaptag->GetDataCount() == pcnt)
		{
			jacobpoint = jacobmaptag->GetDataAddressW();
		}
		else
		{
			jacobmaptag = nullptr;
			jacobpoint = nullptr;
		}
	}

	if (foammaptag)
	{
		if (foammaptag->GetDataCount() == pcnt)
		{
			foampoint = foammaptag->GetDataAddressW();
		}
		else
		{
			foammaptag = nullptr;
			foampoint = nullptr;
		}
	}

	if (stag)
	{
		bsp = stag->GetBaseSelect();
		if (bsp)
			bsp->DeselectAll();
	}

	if (_oceanSimulationRef == nullptr)
	{
		_oceanSimulationRef = OceanSimulation::Ocean().Create() iferr_return;
	}

	if (_oceanSimulationRef.NeedUpdate(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed))
	{
		_oceanSimulationRef.Init(oceanResolution, oceanSize, shrtWaveLenght, waveHeight, windSpeed, windDirection, windAlign, dampReflection, seed) iferr_return;
	}

	_oceanSimulationRef.Animate(_currentTime, timeLoop, timeScale, oceanDepth, chopAmount, true, doChopyness, doJacobian, doNormals) iferr_return;

	FieldInput inputs(padr, pcnt, op_mg);
	Bool outputsOK = _falloff->PreSample(doc, mod, inputs, FIELDSAMPLE_FLAG::VALUE);

	OceanSimulation::INTERTYPE interType = OceanSimulation::INTERTYPE::LINEAR;
	if (doCatmuInter)
		interType = OceanSimulation::INTERTYPE::CATMULLROM;

	Float newMin, newMax;
	newMin = newMax = 0.0;

	maxon::BaseArray<maxon::Float> storeJminus;
	storeJminus.Resize(pcnt) iferr_return;

	auto updatePoints = [this, &padr, &interType, &doChopyness, &doJacobian, &outputsOK, &weight, &jacobmaptag, &jacobpoint, &bsp, &pselThres, &storeJminus, &doDeform](maxon::Int32 i)
	{
		iferr_scope_handler
		{
			err.DbgStop();
			return;
		};

		maxon::Vector p = padr[i];

		maxon::Vector disp, normal, dispValue;
		maxon::Float jMinus;

		_oceanSimulationRef.EvaluatePoint(interType, p, disp, normal, jMinus) iferr_return;

		Float fallOffSampleValue(1.0);
		if (outputsOK)
			_falloff->Sample(p, &fallOffSampleValue, true, 0.0, nullptr, i);
		disp *= fallOffSampleValue;

		if (weight)
			disp *= weight[i];

		if (doChopyness)
			p += disp;
		else
			p.y += disp.y;

		if (doJacobian)
		{
			maxon::Float jMinusValue = -jMinus;

			if (weight)
				jMinusValue *= weight[i];

			if (bsp)
				if (jMinusValue > pselThres)
					bsp->Toggle(i);

			storeJminus[i] = jMinusValue;
		}
		else if (jacobmaptag && jacobpoint) // tag are present but not the option,  reset the value
		{
			jacobmaptag->Set(jacobpoint, nullptr, nullptr, i, maxon::ColorA32(0.0));
		}
		if (doDeform)
			padr[i] = p; // finally update the point
	};
	maxon::ParallelFor::Dynamic(0, pcnt, updatePoints);

	if (jacobmaptag && jacobpoint && doJacobian)
	{
		// get the range of jminus value to set the map in the range of 0-1
		for (auto &jvalue : storeJminus)
		{
			if (jvalue > newMax)
				newMax = jvalue;
			if (jvalue < newMin)
				newMin = jvalue;
		}

		auto updateTag = [&jacobmaptag, &storeJminus, &jacobpoint, &newMax, &newMin, this](maxon::Int32 i)
		{
			jacobmaptag->Set(jacobpoint, nullptr, nullptr, i, maxon::ColorA32(maxon::SafeConvert<maxon::Float32>(MapRange(storeJminus[i], newMin, newMax, 0.0, 1.0))));
		};
		maxon::ParallelFor::Dynamic(0, jacobmaptag->GetDataCount(), updateTag);

		jacobmaptag->SetDirty(DIRTYFLAGS::NONE);
	}

	if (foammaptag && foampoint && jacobpoint && jacobmaptag && doJacobian)
	{
		if (preRunFoam && doAutoTime && _currentTime == 0.0)
		{
			// run simulation for xx frame and get the jminus vertex map from here.
			// only available in autoTime mode and frame 0.0  (or time offset if implemented)
			// magic number 15 should be change to a UI variable
			// calculate 15 frame before
			maxon::BaseArray<maxon::Float32> foamAtFrameZero;
			foamAtFrameZero.Resize(pcnt) iferr_return;

			// clear the vertex map
			auto clearFoamTag = [&foammaptag, &foampoint](maxon::Int32 i)
			{
				//foampoint[i] = 0.0;
				foammaptag->Set(foampoint, nullptr, nullptr, i, maxon::ColorA32(0.0));
			};
			maxon::ParallelFor::Dynamic(0, pcnt, clearFoamTag);

			maxon::TimeValue t = maxon::TimeValue::GetTime();
			for (maxon::Int32 j = -90; j <= 0; j++)
			{
				// animate the ocean 
				_oceanSimulationRef.Animate(j, timeLoop, timeScale, oceanDepth, chopAmount, true, doChopyness, doJacobian, doNormals) iferr_return;

				auto getJminus = [this, &padr, &interType, &weight, &storeJminus](maxon::Int32 i)
				{
					iferr_scope_handler
					{
						return;
					};
					maxon::Vector p = padr[i];

					maxon::Vector disp, normal;
					maxon::Float jMinus;

					_oceanSimulationRef.EvaluatePoint(interType, p, disp, normal, jMinus) iferr_return;
					maxon::Float jMinusValue = -jMinus;
					if (weight)
						jMinusValue *= weight[i];

					storeJminus[i] = jMinusValue;
				};
				maxon::ParallelFor::Dynamic(0, pcnt, getJminus);

				newMin = newMax = 0.0;
				for (auto &jvalue : storeJminus)
				{
					if (jvalue > newMax)
						newMax = jvalue;
					if (jvalue < newMin)
						newMin = jvalue;
				}

				auto updateTag = [&storeJminus, &newMax, &newMin, this](maxon::Int32 i)
				{
					storeJminus[i] = maxon::SafeConvert<maxon::Float32>(MapRange(storeJminus[i], newMin, newMax, 0.0, 1.0));
				};
				maxon::ParallelFor::Dynamic(0, pcnt, updateTag);

				auto updateFoam = [&foamAtFrameZero, &storeJminus, &jacobThres, &foamThres, this](maxon::Int32 i) {
					if (storeJminus[i] > jacobThres)
						foamAtFrameZero[i] += maxon::SafeConvert<maxon::Float32>((MapRange(storeJminus[i], jacobThres, 1.0, 0.0, 1.0) - foamThres));
					else
						foamAtFrameZero[i] -= maxon::SafeConvert<maxon::Float32>(foamThres);

					foamAtFrameZero[i] = maxon::Clamp01(foamAtFrameZero[i]);
				};
				maxon::ParallelFor::Dynamic(0, pcnt, updateFoam);
			} // end for foam before

			ApplicationOutput("time to calculate the sequence @ ", t.Stop());

			auto updateFoamTag = [&foammaptag, &foampoint, &foamAtFrameZero](maxon::Int32 i)
			{
				maxon::ColorA32 color = maxon::ColorA32(foamAtFrameZero[i]);
				foammaptag->Set(foampoint, nullptr, nullptr, i, color);
			};

			maxon::ParallelFor::Dynamic(0, pcnt, updateFoamTag);
		}
		else
		{
			// calculate normal foam 
			BaseTime currentTime;
			Int32     currentFrame;
			currentTime = doc->GetTime();
			currentFrame = currentTime.GetFrame(doc->GetFps());

			auto updateTag = [&jacobmaptag, &foammaptag, &foampoint, &jacobpoint, &jacobThres, &foamThres, this](maxon::Int32 i) {

				if (jacobmaptag->Get(jacobpoint, nullptr, nullptr, i).r > jacobThres)
				{
					maxon::ColorA32 color = foammaptag->Get(foampoint, nullptr, nullptr, i);
					color += maxon::ColorA32(maxon::SafeConvert<maxon::Float32>((MapRange(jacobmaptag->Get(jacobpoint, nullptr, nullptr, i).r, jacobThres, 1.0, 0.0, 1.0) - foamThres)));
					foammaptag->Set(foampoint, nullptr, nullptr, i, color);
				}
				else
				{
					maxon::ColorA32 color = foammaptag->Get(foampoint, nullptr, nullptr, i);
					color -= maxon::ColorA32(maxon::SafeConvert<maxon::Float32>(foamThres));
					foammaptag->Set(foampoint, nullptr, nullptr, i, color);
				}
				maxon::ColorA32 color = foammaptag->Get(foampoint, nullptr, nullptr, i);
				foammaptag->Set(foampoint, nullptr, nullptr, i, color.Clamp01());
			};
			maxon::ParallelFor::Dynamic(0, pcnt, updateTag);
		}
	}

	if (stag)
	{
		stag->SetDirty(DIRTYFLAGS::DATA);
		stag->Message(MSG_UPDATE);
	}

	op->Message(MSG_UPDATE);
	return true;
}

#define GLD_ID_OCEAN_SIMULATION_DEFORMER 1057479
Bool RegisterOceanSimulationDeformer()
{
	return RegisterObjectPlugin(GLD_ID_OCEAN_SIMULATION_DEFORMER, "HOT 4D"_s, OBJECT_MODIFIER, OceanSimulationDeformer::Alloc, "OOceanDeformer"_s, AutoBitmap("hot4D.tif"_s), 0);
}
