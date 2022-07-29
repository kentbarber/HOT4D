CONTAINER OceanDescription
{
	GROUP ID_OCEAN_DESCRIPTION
	{
		NAME OceanDescription;
		DEFAULT 1;
		LONG OCEAN_RESOLUTION
		{ 
			CYCLE
			{
				RESO_4;
				RESO_5;
				RESO_6;
				RESO_7;
				RESO_8;
				RESO_9;
				RESO_10;
				RESO_11;
			}
		}
		
		REAL OCEAN_SIZE { MIN 1; MAX 100000000;STEP 1;}
		REAL WIND_SPEED { MIN 0; MAX 100000; STEP 1; MINEX;}
		REAL WIND_DIRECTION { MIN 0; MAX 360;STEP 1; CUSTOMGUI REALSLIDER;}
		REAL SHRT_WAVELENGHT { MIN 0.01; MAX 100; STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL WAVE_HEIGHT { MIN 0.01; MAX 100000;STEP 1;}
		LONG SEED { MIN 0; MAX 1000000; STEP 1;}
		
		SEPARATOR { LINE; }
		
		BOOL DO_CHOPYNESS {}
		REAL CHOPAMOUNT {MINEX; MIN 0; MAX 5; STEP 0.1; CUSTOMGUI REALSLIDER;}
		
		SEPARATOR { LINE; }
		
		REAL DAMP_REFLECT { MIN 0; MAX 1; STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL WIND_ALIGNMENT { MIN 0; MAX 6;STEP 0.1; CUSTOMGUI REALSLIDER;}
		REAL OCEAN_DEPTH { MIN 0; MAX 10000;STEP 1;}
		
		SEPARATOR { LINE; }
		
		REAL TIMESCALE {MIN 0 ; MAX 10000000000000; STEP 0.1;}
		LONG TIMELOOP {MIN 0; MAX 1000000; STEP 1;}
		BOOL AUTO_ANIM_TIME {}
		REAL CURRENTTIME { MIN 0; MAX 10000000000000; STEP 1;}
		
		SEPARATOR { LINE; }

		BOOL DO_CATMU_INTER { }
		
		// BOOL DO_NORMALS {  }
		
		SEPARATOR { LINE; }
		
		BOOL DO_JACOBIAN { }
		BUTTON CREATE_FOAM_TAGS { }
		LINK JACOBMAP {ACCEPT { 431000045;} }
		REAL JACOB_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		LINK FOAMMAP  {ACCEPT { 431000045;} }
		REAL FOAM_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		BOOL PRE_RUN_FOAM { HIDDEN;};
		
		SEPARATOR { LINE; }
		
		LINK PSEL_PARTICLES {ACCEPT  {Tpointselection; }}
		REAL PSEL_THRES {MIN 0; MAX 1; STEP 0.01; CUSTOMGUI REALSLIDER; }
		BOOL ACTIVE_DEFORM { HIDDEN;}
	}
}
