#pragma once
//  
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//
//  Modified by Kent Barber on 29 /07/22.
//  Copyright (c) 2022 GameLogicDesign Limited.All rights reserved.
//

class OceanSimulationEffector : public EffectorData
{
	INSTANCEOF(OceanSimulationEffector, EffectorData);
public:

	virtual Bool InitEffector(GeListNode* node);
	virtual void InitPoints(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, MoData* md, BaseThread* thread);
	virtual void CalcPointValue(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight);
	virtual Vector CalcPointColor(BaseObject* op, BaseObject* gen, BaseDocument* doc, EffectorDataStruct* data, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight);
	virtual Bool AddToExecution(BaseObject *op, PriorityList *list);

	virtual EXECUTIONRESULT Execute(BaseObject *op, BaseDocument *doc, BaseThread *bt, Int32 priority, EXECUTIONFLAGS flags);	
	virtual Bool GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);

	static NodeData* Alloc() { return NewObjClear(OceanSimulationEffector); }

private:
	OceanSimulation::OceanRef  _oceanSimulationRef; ///< the reference to the ocean simulation
	maxon::Float _currentTime; ///< store the current time of the animation used in check dirty

	//----------------------------------------------------------------------------------------
	/// Evaluate the result of the simulation and return the vectors for displacement, normals and jacobian
	/// @param[in]  op	: the effector send for speed.
	/// @param[in]  p  :  vector of the point to evaluate (only the x and z will be taken into account
	/// @param[out]  displacement  : reference to store the displacement result 
	/// @return		maxon::OK on success
	//----------------------------------------------------------------------------------------
	maxon::Result<void> EvaluatePoint(BaseObject* op, const maxon::Vector p, maxon::Vector &displacement) const;
};




