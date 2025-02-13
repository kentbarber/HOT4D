CONTAINER OceanDescription
{
	GROUP OD_OCEAN_DESCRIPTION
	{
		NAME OceanDescription;
		DEFAULT 1;
		LONG OD_OCEAN_RESOLUTION
		{ 
			CYCLE
			{
				OD_RESO_4;
				OD_RESO_5;
				OD_RESO_6;
				OD_RESO_7;
				OD_RESO_8;
				OD_RESO_9;
				OD_RESO_10;
				OD_RESO_11;
			}
		}
		
		REAL OD_OCEAN_SIZE { MIN 1; MAX 100000000;STEP 1;}
		REAL OD_WIND_SPEED { MIN 0; MAX 100000; STEP 1; MINEX;}
		REAL OD_WIND_DIRECTION { MIN 0; MAX 360;STEP 1; CUSTOMGUI REALSLIDER;}
		REAL OD_SHRT_WAVELENGHT { MIN 0.01; MAX 100; STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL OD_WAVE_HEIGHT { MIN 0.01; MAX 100000;STEP 1;}
		LONG OD_SEED { MIN 0; MAX 1000000; STEP 1;}
		
		SEPARATOR { LINE; }
		
		BOOL OD_DO_CHOPYNESS {}
		REAL OD_CHOPAMOUNT {MINEX; MIN 0; MAX 5; STEP 0.1; CUSTOMGUI REALSLIDER;}
		
		SEPARATOR { LINE; }
		
		REAL OD_DAMP_REFLECT { MIN 0; MAX 1; STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL OD_WIND_ALIGNMENT { MIN 0; MAX 6;STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL OD_OCEAN_DEPTH { MIN 0; MAX 10000;STEP 1;}
		
		SEPARATOR { LINE; }
		
		REAL OD_TIMESCALE {MIN 0 ; MAX 10000000000000; STEP 0.1;}
		LONG OD_TIMELOOP {MIN 0; MAX 1000000; STEP 1;}
		BOOL OD_AUTO_ANIM_TIME {}
		REAL OD_CURRENTTIME { MIN 0; MAX 10000000000000; STEP 1;}
		
		SEPARATOR { LINE; }

		BOOL OD_DO_CATMU_INTER { }
		
		// BOOL OD_DO_NORMALS {  }
		
		SEPARATOR { LINE; }
		
		BOOL OD_DO_JACOBIAN { }
		BUTTON OD_CREATE_FOAM_TAGS { }
		LINK OD_JACOBMAP {ACCEPT { 431000045;} }
		REAL OD_JACOB_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		LINK OD_FOAMMAP  {ACCEPT { 431000045;} }
		REAL OD_FOAM_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		BOOL OD_PRE_RUN_FOAM { HIDDEN;};
		
		SEPARATOR { LINE; }
		
		LINK OD_PSEL_PARTICLES {ACCEPT  {Tpointselection; }}
		REAL OD_PSEL_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		BOOL OD_ACTIVE_DEFORM { HIDDEN;}
	}
}
