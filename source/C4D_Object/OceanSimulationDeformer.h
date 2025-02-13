#pragma once
//  
//  Created by Manuel MAGALHAES on 14/01/13.
//  Copyright (c) 2013 Valkaari. All rights reserved.
//
//  Modified by Kent Barber on 29 /07/22.
//  Copyright (c) 2022 GameLogicDesign Limited.All rights reserved.

#include "c4d_falloffdata.h"

class OceanSimulationDeformer : public ObjectData
{
	INSTANCEOF(OceanSimulationDeformer, ObjectData);
	
private:
	// This is where all the wave action takes place
	maxon::Float						_currentTime; ///< store the current time of the animation used in check dirty
	OceanSimulation::OceanRef			_oceanSimulationRef; ///< ocean reference

	// manage falloff
	AutoAlloc<C4D_Falloff>		_falloff; ///< the falloff object to be compatible with fields.
	maxon::Int32				_falloffDirtyCheck; ///< store the checkdirty to see if the fields have changed.
	
	maxon::Float				MapRange(maxon::Float value, const maxon::Float min_input, const maxon::Float max_input, const maxon::Float min_output, const maxon::Float max_output) const;

public:
	virtual Int32				GetHandleCount(BaseObject *op);
	virtual void				GetHandle(BaseObject *op, Int32 i, HandleInfo &info);
	virtual void				SetHandle(BaseObject *op, Int32 i, Vector p, const HandleInfo &info);
	virtual Bool				CopyTo(NodeData *dest, GeListNode *snode, GeListNode *dnode, COPYFLAGS flags, AliasTrans *trn);
	virtual DRAWRESULT			Draw(BaseObject *op, DRAWPASS drawpass, BaseDraw *bd, BaseDrawHelp *bh);
	
	virtual Bool				GetDDescription(GeListNode *node, Description *description, DESCFLAGS_DESC &flags);

	virtual void				CheckDirty(BaseObject* op, BaseDocument* doc);
	virtual Bool				GetDEnabling(GeListNode *node, const DescID &id, const GeData &t_data, DESCFLAGS_ENABLE flags, const BaseContainer *itemdesc);

	// end of function for managing falloff.

	virtual Bool                Init(GeListNode *node);
	virtual Bool                Message(GeListNode *node, Int32 type, void *t_data);
	virtual Bool                ModifyObject(BaseObject *mod, BaseDocument *doc, BaseObject *op, const Matrix &op_mg, const Matrix &mod_mg, Float lod, Int32 flags, BaseThread *thread);

	static NodeData *Alloc() { return NewObjClear(OceanSimulationDeformer); }
};
