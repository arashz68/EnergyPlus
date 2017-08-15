// EnergyPlus, Copyright (c) 1996-2016, The Board of Trustees of the University of Illinois and
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy). All rights
// reserved.
//
// If you have questions about your rights to use or distribute this software, please contact
// Berkeley Lab's Innovation & Partnerships Office at IPO@lbl.gov.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without Lawrence Berkeley National Laboratory's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// You are under no obligation whatsoever to provide any bug fixes, patches, or upgrades to the
// features, functionality or performance of the source code ("Enhancements") to anyone; however,
// if you choose to make your Enhancements available either publicly, or directly to Lawrence
// Berkeley National Laboratory, without imposing a separate written license agreement for such
// Enhancements, then you hereby grant the following license: a non-exclusive, royalty-free
// perpetual license to install, use, modify, prepare derivative works, incorporate into other
// computer software, distribute, and sublicense such enhancements or derivative works thereof,
// in binary and source code form.


// C++ Headers
#include <cassert>
#include <cmath>

// ObjexxFCL Headers
#include <ObjexxFCL/Array.functions.hh>
#include <ObjexxFCL/Fmath.hh>

// EnergyPlus Headers
#include <HybridUnitaryAirConditioners.hh>
// EnergyPlus Headers
#include <HybridEvapCoolingModel.hh>

#include <BranchNodeConnections.hh>
#include <CurveManager.hh>
#include <DataAirSystems.hh>
#include <DataContaminantBalance.hh>
#include <DataEnvironment.hh>
#include <DataGlobalConstants.hh>
#include <DataHeatBalance.hh>
#include <DataHeatBalFanSys.hh>
#include <DataHVACGlobals.hh>
#include <DataIPShortCuts.hh>
#include <DataLoopNode.hh>
#include <DataPrecisionGlobals.hh>
#include <DataSizing.hh>
#include <DataWater.hh>
#include <DataZoneEnergyDemands.hh>
#include <EMSManager.hh>
#include <Fans.hh>
#include <General.hh>
#include <GeneralRoutines.hh>
#include <InputProcessor.hh>
#include <NodeInputManager.hh>
#include <OutAirNodeManager.hh>
#include <OutputProcessor.hh>
#include <Psychrometrics.hh>
#include <ReportSizingManager.hh>
#include <ScheduleManager.hh>
#include <UtilityRoutines.hh>
#include <WaterManager.hh>

//#include <HybridModelConfigFile.hh>

#define  TEMP_CURVE 0 
#define  W_CURVE 1
#define  POWER_CURVE 2

namespace EnergyPlus {//***************

	namespace HybridUnitaryAirConditioners {
		// Using/Aliasing
		using InputProcessor::FindItemInList;
		using General::TrimSigDigits;
		using HybridEvapCoolingModel::Model;

		using CurveManager::GetCurveIndex;
		using CurveManager::GetCurveType;
		using CurveManager::GetCurveMinMaxValues;
		using CurveManager::CurveValue;
		using HybridEvapCoolingModel::CMode;

		Array1D< Model > ZoneHybridUnitaryAirConditioner;
		int NumZoneHybridEvap(0);
		Array1D_bool CheckZoneHybridEvapName;
		bool GetInputZoneHybridEvap(true);
		//ConfigFile* pConfig = new ConfigFile;
		//Begin routines for zone HVAC Hybrid Evaporative cooler unit
		//_______________________________________________________________________________________________________________________
		//***************
		void
			SimZoneHybridUnitaryAirConditioners(
				std::string const & CompName, // name of the packaged terminal heat pump
				int const ZoneNum, // number of zone being served
				Real64 & SensibleOutputProvided, // sensible capacity delivered to zone
				Real64 & LatentOutputProvided, // Latent add/removal  (kg/s), dehumid = negative
				int & CompIndex // index to zone hvac unit
				)
		{
			using InputProcessor::FindItemInList;
			using General::TrimSigDigits;
			// SUBROUTINE LOCAL VARIABLE DECLARATIONS:
			int CompNum;
			bool errorsfound = false;
			if (GetInputZoneHybridEvap) {
				GetInputZoneHybridUnitaryAirConditioners(errorsfound);
				GetInputZoneHybridEvap = false;
			}

			if (CompIndex == 0) {
				CompNum = FindItemInList(CompName, ZoneHybridUnitaryAirConditioner);
				if (CompNum == 0) {
					ShowFatalError("SimZoneHybridUnitaryAirConditioners: Zone evaporative cooler unit not found.");
				}
				CompIndex = CompNum;
			}
			else {
				CompNum = CompIndex;
				if (CompNum < 1 || CompNum > NumZoneHybridEvap) {
					ShowFatalError("SimZoneHybridUnitaryAirConditioners: Invalid CompIndex passed=" + TrimSigDigits(CompNum) + ", Number of units =" + TrimSigDigits(NumZoneHybridEvap) + ", Entered Unit name = " + CompName);
				}
				if (CheckZoneHybridEvapName(CompNum)) {
					if (CompName != ZoneHybridUnitaryAirConditioner(CompNum).Name) {
						ShowFatalError("SimZoneHybridUnitaryAirConditioners: Invalid CompIndex passed=" + TrimSigDigits(CompNum) + ", Unit name=" + CompName + ", stored unit name for that index=" + ZoneHybridUnitaryAirConditioner(CompNum).Name);
					}
					CheckZoneHybridEvapName(CompNum) = false;
				}
			}
			try
			{
				InitZoneHybridUnitaryAirConditioners(CompNum, ZoneNum);
			}
			catch (int e)
			{
				cout << "An exception occurred in InitZoneHybridUnitaryAirConditioners. Exception Nr. " << e << '\n';
				return;
			}
			try
			{
				CalcZoneHybridUnitaryAirConditioners(CompNum, ZoneNum, SensibleOutputProvided, LatentOutputProvided);
			}
			catch (int e)
			{
				cout << "An exception occurred in CalcZoneHybridUnitaryAirConditioners. Exception Nr. " << e << '\n';
				return;
			}
			try
			{
				ReportZoneHybridUnitaryAirConditioners(CompNum);
			}
			catch (int e)
			{
				cout << "An exception occurred in ReportZoneHybridUnitaryAirConditioners. Exception Nr. " << e << '\n';
				return;
			}
		}
		Model* HandelToHybridUnitaryAirConditioner(int UnitNum)
		{
			Model*p = &(ZoneHybridUnitaryAirConditioner(UnitNum));
			return &(ZoneHybridUnitaryAirConditioner(UnitNum));
		}


		void
			InitZoneHybridUnitaryAirConditioners(
				int const UnitNum, // unit number
				int const ZoneNum // number of zone being served
				)
		{

			// Using/Aliasing
			using namespace DataLoopNode;
			using namespace Psychrometrics;
			using DataGlobals::TimeStep;
			using DataGlobals::TimeStepZone;
			using DataGlobals::WarmupFlag;
			using DataGlobals::HourOfDay;
			using DataHVACGlobals::ZoneComp;
			using DataHVACGlobals::SysTimeElapsed;
			using DataSizing::AutoSize;
			using DataEnvironment::StdRhoAir;
			using Fans::GetFanVolFlow;

			// Locals
			static Array1D_bool MySizeFlag;

			static bool HybridCoolOneTimeFlag(true); // one time flag
			static Array1D_bool MyEnvrnFlag;
			static Array1D_bool MyFanFlag;
			static Array1D_bool MyZoneEqFlag; // used to set up zone equipment availability managers
			int Loop;
			static bool ZoneEquipmentListChecked(false); // True after the Zone Equipment List has been checked for items
			Real64 TimeElapsed;

			int InletNode;
			int SecInletNode; // local index for secondary inlet node.
			Real64 RhoAir; // Air Density
			int ControlNode;
			int OutNode;
			int EvapUnitNum;

			if (HybridCoolOneTimeFlag) {
				MySizeFlag.dimension(NumZoneHybridEvap, true);
				MyEnvrnFlag.dimension(NumZoneHybridEvap, true);
				MyFanFlag.dimension(NumZoneHybridEvap, true);
				MyZoneEqFlag.allocate(NumZoneHybridEvap);
				MyZoneEqFlag = true;
				HybridCoolOneTimeFlag = false;
			}
			if(!ZoneHybridUnitaryAirConditioner(UnitNum).Initialized)
			{
				ZoneHybridUnitaryAirConditioner(UnitNum).Initialize(ZoneNum);//, pConfig);
			}
			ZoneHybridUnitaryAirConditioner(UnitNum).RequestedLoadToHeatingSetpoint = 0;
			ZoneHybridUnitaryAirConditioner(UnitNum).RequestedLoadToCoolingSetpoint = 0;
			ZoneHybridUnitaryAirConditioner(UnitNum).UnitTotalCoolingRate = 0.0;
			ZoneHybridUnitaryAirConditioner(UnitNum).UnitTotalCoolingEnergy = 0.0;

			ZoneHybridUnitaryAirConditioner(UnitNum).UnitSensibleCoolingRate = 0.0;
			ZoneHybridUnitaryAirConditioner(UnitNum).UnitSensibleCoolingEnergy = 0.0;
			ZoneHybridUnitaryAirConditioner(UnitNum).UnitLatentCoolingRate = 0.0;
			ZoneHybridUnitaryAirConditioner(UnitNum).UnitLatentCoolingEnergy = 0.0;
		

			ZoneHybridUnitaryAirConditioner(UnitNum).InitializeModelParams();
			// Do the following initializations (every time step): This should be the info from
			// the previous components outlets or the node data in this section.

			//Transfer the node data to EvapCond data structure
			InletNode = ZoneHybridUnitaryAirConditioner(UnitNum).InletNode;

			ZoneHybridUnitaryAirConditioner(UnitNum).InletMassFlowRate = Node(InletNode).MassFlowRate;

			//Set all of the inlet state variables from the inlet nodes
			ZoneHybridUnitaryAirConditioner(UnitNum).InletTemp = Node(InletNode).Temp;
			ZoneHybridUnitaryAirConditioner(UnitNum).InletHumRat = Node(InletNode).HumRat;
			ZoneHybridUnitaryAirConditioner(UnitNum).InletEnthalpy = Node(InletNode).Enthalpy;
			ZoneHybridUnitaryAirConditioner(UnitNum).InletPressure = Node(InletNode).Press;
			ZoneHybridUnitaryAirConditioner(UnitNum).InletRH = PsyRhFnTdbWPb(ZoneHybridUnitaryAirConditioner(UnitNum).InletTemp, ZoneHybridUnitaryAirConditioner(UnitNum).InletHumRat, ZoneHybridUnitaryAirConditioner(UnitNum).OutletPressure, "InitZoneHybridUnitaryAirConditioners");

			//Set default outlet state to inlet states(?)
			ZoneHybridUnitaryAirConditioner(UnitNum).OutletTemp = ZoneHybridUnitaryAirConditioner(UnitNum).InletTemp;
			ZoneHybridUnitaryAirConditioner(UnitNum).OutletHumRat = ZoneHybridUnitaryAirConditioner(UnitNum).InletHumRat;
			ZoneHybridUnitaryAirConditioner(UnitNum).OutletEnthalpy = ZoneHybridUnitaryAirConditioner(UnitNum).InletEnthalpy;
			ZoneHybridUnitaryAirConditioner(UnitNum).OutletPressure = ZoneHybridUnitaryAirConditioner(UnitNum).InletPressure;
			ZoneHybridUnitaryAirConditioner(UnitNum).OutletRH = PsyRhFnTdbWPb(ZoneHybridUnitaryAirConditioner(UnitNum).OutletTemp, ZoneHybridUnitaryAirConditioner(UnitNum).OutletHumRat, ZoneHybridUnitaryAirConditioner(UnitNum).OutletPressure, "InitZoneHybridUnitaryAirConditioners");

			ZoneHybridUnitaryAirConditioner(UnitNum).OutletMassFlowRate = ZoneHybridUnitaryAirConditioner(UnitNum).InletMassFlowRate;

			ZoneHybridUnitaryAirConditioner(UnitNum).SecInletTemp = Node(ZoneHybridUnitaryAirConditioner(UnitNum).SecondaryInletNode).Temp;
			ZoneHybridUnitaryAirConditioner(UnitNum).SecInletHumRat = Node(ZoneHybridUnitaryAirConditioner(UnitNum).SecondaryInletNode).HumRat;
			ZoneHybridUnitaryAirConditioner(UnitNum).SecInletEnthalpy = Node(ZoneHybridUnitaryAirConditioner(UnitNum).SecondaryInletNode).Enthalpy;
			ZoneHybridUnitaryAirConditioner(UnitNum).SecInletPressure = Node(ZoneHybridUnitaryAirConditioner(UnitNum).SecondaryInletNode).Press;
			double RHosa = Part_press(101.325, ZoneHybridUnitaryAirConditioner(UnitNum).SecInletHumRat) / Sat_press(ZoneHybridUnitaryAirConditioner(UnitNum).SecInletTemp);
			ZoneHybridUnitaryAirConditioner(UnitNum).SecInletRH = PsyRhFnTdbWPb(ZoneHybridUnitaryAirConditioner(UnitNum).SecInletTemp, ZoneHybridUnitaryAirConditioner(UnitNum).SecInletHumRat, ZoneHybridUnitaryAirConditioner(UnitNum).SecInletPressure, "InitZoneHybridUnitaryAirConditioners");

		}

		double Part_press(double P, double W)
		{
			// Function to compute partial vapor pressure in [kPa]
			// From page 6.9 equation 38 in ASHRAE Fundamentals handbook (2005)
			//   P = ambient pressure [kPa]
			//   W = humidity ratio [kg/kg dry air]

			return (P * W / (0.62198 + W));
		}

		double Sat_press(double Tdb)
		{
			// Function to compute saturation vapor pressure in [kPa]
			//ASHRAE Fundamentals handbood (2005) p 6.2, equation 5 and 6
			//   Tdb = Dry bulb temperature [degC]
			// Valid from -100C to 200 C

			double  C1 = -5674.5359;
			double  C2 = 6.3925247;
			double  C3 = -0.009677843;
			double  C4 = 0.00000062215701;
			double  C5 = 2.0747825E-09;
			double  C6 = -9.484024E-13;
			double  C7 = 4.1635019;
			double  C8 = -5800.2206;
			double  C9 = 1.3914993;
			double  C10 = -0.048640239;
			double  C11 = 0.000041764768;
			double  C12 = -0.000000014452093;
			double  C13 = 6.5459673;
			double  Sat_press_val = 0;

			double   TK = Tdb + 273.15;         //Converts from degC to degK

			if (TK <= 273.15)
			{
				Sat_press_val = exp(C1 / TK + C2 + C3 * TK + C4 * pow(TK, 2) + C5 * pow(TK, 3) + C6 * pow(TK, 4) + C7 * log(TK)) / 1000;
			}
			else
			{
				Sat_press_val = exp(C8 / TK + C9 + C10 * TK + C11 * pow(TK, 2) + C12 * pow(TK, 3) + C13 * log(TK)) / 1000;
			}
			return Sat_press_val;

		}

		

		void
			CalcZoneHybridUnitaryAirConditioners(
				int const UnitNum, // unit number
				int const ZoneNum, // number of zone being served
				Real64 & SensibleOutputProvided, // sensible capacity delivered to zone
				Real64 & LatentOutputProvided // Latent add/removal  (kg/s), dehumid = negative
				)
		{
			using DataZoneEnergyDemands::ZoneSysEnergyDemand;
			using DataZoneEnergyDemands::ZoneSysMoistureDemand;
			using DataHVACGlobals::TimeStepSys;
			using DataGlobals::SecInHour;
			using namespace DataLoopNode;
			using namespace Psychrometrics;
			using DataEnvironment::StdRhoAir;

			Real64 QTotUnitOut = 0;
			Real64 QSensUnitOut=0;
			Real64 EnvDryBulbT, AirTempRoom, EnvRelHumm, RoomRelHum, RemainQ, DesignMinVR, returnQSensible, returnQLatent, returnSupplyAirMassFlow, returnSupplyAirTemp, returnSupplyAirRelHum, returnVentilationAir, ElectricalPowerUse;
		
			Real64 ZoneCoolingLoad = ZoneSysEnergyDemand(ZoneNum).RemainingOutputReqToCoolSP; // Remaining load required to meet cooling setpoint (<0 is a cooling load)
			Real64 ZoneHeatingLoad = ZoneSysEnergyDemand(ZoneNum).RemainingOutputReqToHeatSP; // Remaining load required to meet heating setpoint (>0 is a heating load)
			double OutputRequiredToHumidify= ZoneSysMoistureDemand(ZoneNum).OutputRequiredToHumidifyingSP; // Load required to meet humidifying setpoint (>0 = a humidify load) [kgWater/s]
			
			
			double OutputRequiredToDehumidify =ZoneSysMoistureDemand(ZoneNum).OutputRequiredToDehumidifyingSP; // Load required to meet dehumidifying setpoint (<0 = a dehumidify load)  [kgWater/s]
			

			SensibleOutputProvided = ZoneCoolingLoad;
			LatentOutputProvided = 0;
			// taking class members out of the object and then using them in the calcualtion is odd but its for clarity with unit testing.
			EnvDryBulbT = ZoneHybridUnitaryAirConditioner(UnitNum).SecInletTemp;// eg. 34.95;
			AirTempRoom = ZoneHybridUnitaryAirConditioner(UnitNum).InletTemp;//23.94039067;
			EnvRelHumm = ZoneHybridUnitaryAirConditioner(UnitNum).SecInletRH;//RH 78; 
			RoomRelHum = ZoneHybridUnitaryAirConditioner(UnitNum).InletRH;//RH 38;
			
			bool UseOccSchFlag = 1;
			bool UseMinOASchFlag = 1;
			//OARequirementsPtr = 1;
			using DataZoneEquipment::CalcDesignSpecificationOutdoorAir;
			DesignMinVR = CalcDesignSpecificationOutdoorAir(ZoneHybridUnitaryAirConditioner(UnitNum).OARequirementsPtr, ZoneNum, UseOccSchFlag, UseMinOASchFlag); //[m3/s]
			double DesignMinVRMassFlow = 0;
			if (StdRhoAir>1) 	DesignMinVRMassFlow = DesignMinVR * StdRhoAir;
			else DesignMinVRMassFlow = DesignMinVR *  1.225;
			// note: the multplication of the max values of the "Fraction of peak Msa" and the OSAF as specified in the config must be greater than the ratio of MinVR/SystemMaximumSupplyAirFlowRate otherwise it will never reach mi								 
			ZoneHybridUnitaryAirConditioner(UnitNum).doStep(EnvDryBulbT, AirTempRoom, EnvRelHumm, RoomRelHum, ZoneCoolingLoad, ZoneHeatingLoad, OutputRequiredToHumidify, OutputRequiredToDehumidify, DesignMinVRMassFlow);


		}

		void
			ReportZoneHybridUnitaryAirConditioners(int const UnitNum) // unit number
		{
			using namespace DataLoopNode;
			using namespace Psychrometrics;
			ZoneHybridUnitaryAirConditioner(UnitNum).PrimaryMode = ZoneHybridUnitaryAirConditioner(UnitNum).PrimaryMode;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).InletNode).MassFlowRate =  ZoneHybridUnitaryAirConditioner(UnitNum).InletMassFlowRate;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).InletNode).MassFlowRate = ZoneHybridUnitaryAirConditioner(UnitNum).InletMassFlowRate;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).OutletNode).Temp =         ZoneHybridUnitaryAirConditioner(UnitNum).OutletTemp;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).OutletNode).HumRat =       ZoneHybridUnitaryAirConditioner(UnitNum).OutletHumRat;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).OutletNode).MassFlowRate = ZoneHybridUnitaryAirConditioner(UnitNum).OutletMassFlowRate;
			Node(ZoneHybridUnitaryAirConditioner(UnitNum).OutletNode).Enthalpy = ZoneHybridUnitaryAirConditioner(UnitNum).OutletEnthalpy;//PsyHFnTdbW(ZoneHybridUnitaryAirConditioner(UnitNum).OutletTemp, ZoneHybridUnitaryAirConditioner(UnitNum).OutletHumRat);
		

		}

		void
			GetInputZoneHybridUnitaryAirConditioners(bool & Errors)
		{
			using BranchNodeConnections::TestCompSet;
			using namespace ScheduleManager;
			using DataGlobals::ScheduleAlwaysOn;
			using NodeInputManager::GetOnlySingleNode;
			using BranchNodeConnections::SetUpCompSets;
			using namespace DataIPShortCuts; // Data for field names, blank numerics
			using namespace DataLoopNode;
			using InputProcessor::GetNumObjectsFound;
			using InputProcessor::GetObjectDefMaxArgs;
			using InputProcessor::GetObjectItem;
			using InputProcessor::FindItemInList;
			using InputProcessor::VerifyName;
			using DataSizing::OARequirements; // to find DesignSpecification:OutdoorAir pointer
			std::string CurrentModuleObject; // Object type for getting and error messages
			Array1D_string Alphas; // Alpha items for object
			Array1D< Real64 > Numbers; // Numeric items for object
			Array1D_string cAlphaFields; // Alpha field names
			Array1D_string cNumericFields; // Numeric field names
			Array1D_bool lAlphaBlanks; // Logical array, alpha field input BLANK = .TRUE.
			Array1D_bool lNumericBlanks; // Logical array, numeric field input BLANK = .TRUE.
			int NumAlphas; // Number of Alphas for each GetObjectItem call
			int NumNumbers; // Number of Numbers for each GetObjectItem call
			int MaxAlphas; // Maximum number of alpha fields in all objects
			int MaxNumbers; // Maximum number of numeric fields in all objects
			int NumFields; // Total number of fields in object
			int IOStatus; // Used in GetObjectItem
			static bool ErrorsFound(false); // Set to true if errors in input, fatal at end of routine
			bool IsNotOK; // Flag to verify name
			bool IsBlank; // Flag for blank name
			bool errFlag;
			Real64 FanVolFlow;
			int UnitLoop;
			int CtrlZone; // index to loop counter
			int NodeNum; // index to loop counter

						 // SUBROUTINE PARAMETER DEFINITIONS:
			static std::string const RoutineName("GetInputZoneEvaporativeCoolerUnit: ");
			MaxNumbers = 0;
			MaxAlphas = 0;

			CurrentModuleObject = "ZoneHVAC:HybridUnitaryHVAC";
			NumZoneHybridEvap = GetNumObjectsFound(CurrentModuleObject);
			GetObjectDefMaxArgs(CurrentModuleObject, NumFields, NumAlphas, NumNumbers);
			MaxNumbers = max(MaxNumbers, NumNumbers);
			MaxAlphas = max(MaxAlphas, NumAlphas);
			Alphas.allocate(MaxAlphas);
			Numbers.dimension(MaxNumbers, 0.0);
			cAlphaFields.allocate(MaxAlphas);
			cNumericFields.allocate(MaxNumbers);
			lAlphaBlanks.dimension(MaxAlphas, true);
			lNumericBlanks.dimension(MaxNumbers, true); 
			vector<std::string> test;
			vector<bool> blanks;

			if (NumZoneHybridEvap > 0) {
				CheckZoneHybridEvapName.dimension(NumZoneHybridEvap, true);
				ZoneHybridUnitaryAirConditioner.allocate(NumZoneHybridEvap);
				//ZoneEvapCoolerUnitFields.allocate(NumZoneEvapUnits);

				for (UnitLoop = 1; UnitLoop <= NumZoneHybridEvap; ++UnitLoop) {
					GetObjectItem(CurrentModuleObject, UnitLoop, Alphas, NumAlphas, Numbers, NumNumbers, IOStatus, lNumericBlanks, lAlphaBlanks, cAlphaFields, cNumericFields);
					for (int k = 1; k <= MaxAlphas; ++k) {

						test.push_back(Alphas(k));
						blanks.push_back(lAlphaFieldBlanks(k));


					}

					IsNotOK = false;
					IsBlank = false;
					VerifyName(Alphas(1), ZoneHybridUnitaryAirConditioner, UnitLoop - 1, IsNotOK, IsBlank, CurrentModuleObject + " Name");

						//A1, \field Name
					ZoneHybridUnitaryAirConditioner(UnitLoop).Name = Alphas(1);
						//A2, \field Availability Schedule Name
					ZoneHybridUnitaryAirConditioner(UnitLoop).Schedule = Alphas(2);
					if (lAlphaFieldBlanks(2)) {
						ZoneHybridUnitaryAirConditioner(UnitLoop).SchedPtr = ScheduleAlwaysOn;
					}
					else {
						ZoneHybridUnitaryAirConditioner(UnitLoop).SchedPtr = GetScheduleIndex(Alphas(2));
						if (ZoneHybridUnitaryAirConditioner(UnitLoop).SchedPtr == 0) {
							ShowSevereError("Invalid " + cAlphaFieldNames(2) + '=' + Alphas(2));
							ShowContinueError("Entered in " + cCurrentModuleObject + '=' + Alphas(1));
							ErrorsFound = true;
						}
					}
						//A3, \field Availability Manager List Name
		
						//A4, \field Minimum Supply Air Temperature Schedule Name
					
					ZoneHybridUnitaryAirConditioner(UnitLoop).TsaMin_schedule_pointer = GetScheduleIndex(Alphas(4));
					if (ZoneHybridUnitaryAirConditioner(UnitLoop).TsaMin_schedule_pointer == 0) {
						ShowSevereError("Invalid " + cAlphaFields(4) + '=' + Alphas(4));
						ShowContinueError("Entered in " + cCurrentModuleObject + '=' + cAlphaArgs(1));
						ErrorsFound = true;
					}
						//A5, \field Maximum Supply Air Temperature Schedule Name
					ZoneHybridUnitaryAirConditioner(UnitLoop).TsaMax_schedule_pointer = GetScheduleIndex(Alphas(5));
					if (ZoneHybridUnitaryAirConditioner(UnitLoop).TsaMax_schedule_pointer == 0) {
						ShowSevereError("Invalid " + cAlphaFields(5) + '=' + Alphas(5));
						ShowContinueError("Entered in " + cCurrentModuleObject + '=' + cAlphaArgs(1));
						ErrorsFound = true;
					}
						//A6, \field Minimum Supply Air Humidity Ratio Schedule Name
						//A7, \field Maximum Supply Air Humidity Ratio Schedule Name
						//A8, \field Method to Choose Value of Controlled Inputs

						//A9, \field Return Air Node Name
						//A10, \field Outside Air Node Name
						//A11, \field Supply Air Node Name
						//A12, \field Relief Node Name
					ZoneHybridUnitaryAirConditioner(UnitLoop).InletNode = GetOnlySingleNode(Alphas(9), ErrorsFound, CurrentModuleObject, Alphas(1), NodeType_Air, NodeConnectionType_Inlet, 1, ObjectIsNotParent);
					ZoneHybridUnitaryAirConditioner(UnitLoop).SecondaryInletNode = GetOnlySingleNode(Alphas(10), ErrorsFound, CurrentModuleObject, Alphas(1), NodeType_Air, NodeConnectionType_OutsideAirReference, 1, ObjectIsNotParent);
					ZoneHybridUnitaryAirConditioner(UnitLoop).OutletNode = GetOnlySingleNode(Alphas(11), ErrorsFound, CurrentModuleObject, Alphas(1), NodeType_Air, NodeConnectionType_Outlet, 1, ObjectIsNotParent); 
					ZoneHybridUnitaryAirConditioner(UnitLoop).SecondaryOutletNode = GetOnlySingleNode(Alphas(12), ErrorsFound, CurrentModuleObject, Alphas(1), NodeType_Air, NodeConnectionType_ReliefAir, 1, ObjectIsNotParent);
					TestCompSet(CurrentModuleObject, Alphas(1), Alphas(9), Alphas(11), "Hybrid Evap Air Zone Nodes");
					TestCompSet(CurrentModuleObject, Alphas(1), Alphas(10), Alphas(12), "Hybrid Evap Air Zone Secondary Nodes");
						
						//N1, \field System Maximum Supply AirFlow Rate
					ZoneHybridUnitaryAirConditioner(UnitLoop).SystemMaximumSupplyAirFlowRate = Numbers(1);
					
					//In each time step, the result for system power, fan power, gas use, water user, or supply airflow rate will be determined as :
					//TableValue * SysMaxSupply * ScalingFactor

						//N2, \field External Static Pressure at System Maximum Supply Air Flow Rate

						//N3, \field Scaling Factor
					ZoneHybridUnitaryAirConditioner(UnitLoop).ScalingFactor = Numbers(3);
					
					ZoneHybridUnitaryAirConditioner(UnitLoop).ScaledSystemMaximumSupplyAirMassFlowRate = Numbers(1)*Numbers(3)*1.2041;
					CMode* pMode;// = ZoneHybridUnitaryAirConditioner(UnitLoop).AddNewOperatingMode();
						//N4, \field Number of Operating Modes
					int Numberofoperatingmodes = 0;
					if (lNumericBlanks(4)) {
						ShowSevereError("Invalid number of operating modes" + cNumericFields(5)  );
						ShowFatalError(RoutineName + "Errors found in getting input.");
						ShowContinueError("... Preceding condition causes terminascaler*1.2041*pZoneHybridUnitaryAirConditioner->SystemMaximumSupplyAirFlowRatetion.");
					}
					else {
						Numberofoperatingmodes= Numbers(4)-1; //zero based count
					}
						//N5, \field Minimum Time Between Mode Change
						//A13, \field First fuel type
						//A14, \field Second fuel type
						//A15, \field Third fuel type
						//A16, \field Objective Function Minimizes
						//A17, \field Mode0 Name
						//A18, \field Mode0 Supply Air Temperature Lookup Table Name

					
					// get Design specification outdoor air object
					//if (!lAlphaFieldBlanks(15)) {
	
						ZoneHybridUnitaryAirConditioner(UnitLoop).OARequirementsPtr = FindItemInList(Alphas(17), OARequirements);
						if (ZoneHybridUnitaryAirConditioner(UnitLoop).OARequirementsPtr == 0) {
							ShowSevereError(RoutineName + cCurrentModuleObject + "=\"" + Alphas(1) + " invalid data");
							ShowContinueError("Invalid-not found" + cAlphaFieldNames(17) + "=\"" + Alphas(17) + "\".");
							ErrorsFound = true;
						}
						else {
							ZoneHybridUnitaryAirConditioner(UnitLoop).OutdoorAir = true;
						}
					//}

					pMode = ZoneHybridUnitaryAirConditioner(UnitLoop).AddNewOperatingMode(ZoneHybridUnitaryAirConditioner(UnitLoop).ScaledSystemMaximumSupplyAirMassFlowRate);
					ErrorsFound = pMode->ParseMode0(Alphas, cAlphaFields, Numbers, cNumericFields, lAlphaBlanks, cCurrentModuleObject);
					if (ErrorsFound) {
						ShowFatalError(RoutineName + "Errors found parsing modes");
						ShowContinueError("... Preceding condition causes termination.");
						break;
					}

					for (int modeIter = 1; modeIter <= Numberofoperatingmodes; ++modeIter) {
						pMode= ZoneHybridUnitaryAirConditioner(UnitLoop).AddNewOperatingMode(ZoneHybridUnitaryAirConditioner(UnitLoop).ScaledSystemMaximumSupplyAirMassFlowRate);
						ErrorsFound = pMode->ParseMode(Alphas, cAlphaFields, Numbers, cNumericFields, lAlphaBlanks, cCurrentModuleObject);
						if (ErrorsFound) {
							ShowFatalError(RoutineName + "Errors found parsing modes");
							ShowContinueError("... Preceding condition causes termination.");
							break;
						}
					}

				}
			}
			// setup output variables
			for (UnitLoop = 1; UnitLoop <= NumZoneHybridEvap; ++UnitLoop) {

				SetUpCompSets(CurrentModuleObject, ZoneHybridUnitaryAirConditioner(UnitLoop).Name, CurrentModuleObject, ZoneHybridUnitaryAirConditioner(UnitLoop).Name, NodeID(ZoneHybridUnitaryAirConditioner(UnitLoop).InletNode), NodeID(ZoneHybridUnitaryAirConditioner(UnitLoop).OutletNode));
				SetUpCompSets(CurrentModuleObject, ZoneHybridUnitaryAirConditioner(UnitLoop).Name, CurrentModuleObject, ZoneHybridUnitaryAirConditioner(UnitLoop).Name, NodeID(ZoneHybridUnitaryAirConditioner(UnitLoop).SecondaryInletNode), NodeID(ZoneHybridUnitaryAirConditioner(UnitLoop).SecondaryOutletNode));

				SetupOutputVariable("Zone Hybrid Unitary HVAC System Total Cooling Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemTotalCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Total Cooling Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemTotalCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Sensible Cooling Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemSensibleCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Sensible Cooling Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemSensibleCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Latent Cooling Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemLatentCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Latent Cooling Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemLatentCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");

				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Total Cooling Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitTotalCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Total Cooling Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitTotalCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Sensible Cooling Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitSensibleCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Sensible Cooling Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitSensibleCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Latent Cooling Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitLatentCoolingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Latent Cooling Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitLatentCoolingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");

				SetupOutputVariable("Zone Hybrid Unitary HVAC System Total Heating Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemTotalHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Total Heating Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemTotalHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "HeatingCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Sensible Heating Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemSensibleHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Sensible Heating Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemSensibleHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "HeatingCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Latent Heating Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemLatentHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC System Latent Heating Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).SystemLatentHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "HeatingCOILS", _, "System");

				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Total Heating Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitTotalHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Total Heating Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitTotalHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "HeatingCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Sensible Heating Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitSensibleHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Sensible Heating Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitSensibleHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "HeatingCOILS", _, "System");
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Latent Heating Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitLatentHeatingRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Zone Latent Heating Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitLatentHeatingEnergy, "System", "Sum", ZoneHybridUnitaryAirConditioner(UnitLoop).Name, _, "ENERGYTRANSFER", "COOLINGCOILS", _, "System");
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Predicted Sensible Load to Setpoint Heat Transfer Rate [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedLoadToCoolingSetpoint,"System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				//SetupOutputVariable("Average Mode Number []", ZoneHybridUnitaryAirConditioner(UnitLoop).PrimaryMode, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("ErrorCode []", ZoneHybridUnitaryAirConditioner(UnitLoop).ErrorCode, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				//SetupOutputVariable("Attempted Tsa []", ZoneHybridUnitaryAirConditioner(UnitLoop).Tsa, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);

				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Supply Air Temperature [C]", ZoneHybridUnitaryAirConditioner(UnitLoop).OutletTemp, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Return Air Temperature [C]", ZoneHybridUnitaryAirConditioner(UnitLoop).InletTemp, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Outside Air Temperature [C]", ZoneHybridUnitaryAirConditioner(UnitLoop).SecInletTemp, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Supply Air Humidity Ratio [kgWater/kgDryAir]", ZoneHybridUnitaryAirConditioner(UnitLoop).Wsa, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Return Air Humidity Ratio [kgWater/kgDryAir]", ZoneHybridUnitaryAirConditioner(UnitLoop).InletHumRat, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Outside Air Humidity Ratio [kgWater/kgDryAir]", ZoneHybridUnitaryAirConditioner(UnitLoop).SecInletHumRat, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Supply Air Relative Humidity [%]", ZoneHybridUnitaryAirConditioner(UnitLoop).OutletRH, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Return Air Relative Humidity [%]", ZoneHybridUnitaryAirConditioner(UnitLoop).InletRH, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Outside Air Relative Humidity [%]", ZoneHybridUnitaryAirConditioner(UnitLoop).SecInletRH, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Supply Air Mass Flow Rate [kg/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).OutletMassFlowRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Supply Air Standard Density Volume Flow Rate [m3/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).OutletVolumetricFlowRate, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Ventilation Air Standard Density Volume Flow Rate [m3/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).SupplyVentilationVolume, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Electric Power [W]", ZoneHybridUnitaryAirConditioner(UnitLoop).FinalElectricalPower, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Electric Energy [J]", ZoneHybridUnitaryAirConditioner(UnitLoop).FinalElectricalEnergy, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);

				SetupOutputVariable("Zone Hybrid Unitary HVAC Requested Outdoor Air Ventilation Mass Flow Rate [kg/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).MinOA_Msa, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Ventilation Air Mass Flow Rate[kg/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).SupplyVentilationAir, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Availability Status[]", ZoneHybridUnitaryAirConditioner(UnitLoop).UnitOn, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Outside Air Fraction []", ZoneHybridUnitaryAirConditioner(UnitLoop).averageOSAF, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Dehumidification Load to Humidistat Setpoint Moisture Transfer Rate[kg/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedDeHumdificationMass, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Dehumidification Load to Humidistat Setpoint Heat Transfer Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedDeHumdificationLoad, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC DehumidificationLoad to Humidistat Setpoint Heat Tansfer Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedDeHumdificationEnergy, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				SetupOutputVariable("Zone Hybrid Unitary HVAC Humidification Load to Humidistat Setpoint Moisture Transfer Rate[kg/s]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedHumdificationMass, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Humidification Load to Humidistat Setpoint Heat Transfer Rate[W]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedHumdificationLoad, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				SetupOutputVariable("Zone Hybrid Unitary HVAC Humidification Load to Humidistat Setpoint Heat Tansfer Energy[J]", ZoneHybridUnitaryAirConditioner(UnitLoop).RequestedHumdificationEnergy, "System", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
				
				//add these !!!!
				/*HVAC,Average,Zone Hybrid Unitary HVAC Supply Fan Electric Power [W]
				HVAC,Sum,Zone Hybrid Unitary HVAC Supply Fan Electric Energy [J]
				HVAC,Average,Zone Hybrid Unitary HVAC Secondary Fuel Consumption Rate [W]
				HVAC,Sum,Zone Hybrid Unitary HVAC Secondary Fuel Consumption [J]
				HVAC,Average,Zone Hybrid Unitary HVAC Third Fuel Consumption Rate [W]
				HVAC,Sum,Zone Hybrid Unitary HVAC Third Fuel Consumption [J]
				HVAC,Average,Zone Hybrid Unitary HVAC Water Consumption Rate [m3/hr]
				HVAC,Sum,Zone Hybrid Unitary HVAC Water Consumption [m3]
				*/
					
				list<CSetting*>::iterator iterOperatingSettings;
				int index=0;
				for (iterOperatingSettings = ZoneHybridUnitaryAirConditioner(UnitLoop).CurrentOperatingSettings.begin(); iterOperatingSettings != ZoneHybridUnitaryAirConditioner(UnitLoop).CurrentOperatingSettings.end(); ++iterOperatingSettings) {
					CSetting* po = (*iterOperatingSettings);
					std::ostringstream strs;
					strs << "Zone Hybrid Unitary HVAC Runtime Fraction in Setting "<<index<<" []";
					SetupOutputVariable(strs.str(), po->Runtime_Fraction,"HVAC", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
					strs.str("");
					strs << "Zone Hybrid Unitary HVAC Mode in Setting "<<index<<" []";
					SetupOutputVariable(strs.str(), po->Mode,"HVAC", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
					strs.str("");
					strs << "Zone Hybrid Unitary HVAC Outside Air Fraction in Setting "<<index<<" []";
					SetupOutputVariable(strs.str(), po->Outside_Air_Fraction,"HVAC", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
					strs.str("");
					strs << "Zone Hybrid Unitary HVAC Supply Air Mass Flow Rate in Setting "<<index<<" [kg/s]";
					SetupOutputVariable(strs.str(), po->Supply_Air_Mass_Flow_Rate,"HVAC", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
					strs.str("");
					strs << "Zone Hybrid Unitary HVAC Supply Air Mass Flow Rate Ratio in Setting "<<index<<" []";
					SetupOutputVariable(strs.str(), po->Supply_Air_Mass_Flow_Rate_Ratio,"HVAC", "Average", ZoneHybridUnitaryAirConditioner(UnitLoop).Name);
					strs.str("");
					index++;
				}
			
			}
			Errors = ErrorsFound;
			if (ErrorsFound) {
				ShowFatalError(RoutineName + "Errors found in getting input.");
				ShowContinueError("... Preceding condition causes termination.");
			}
		}
		

	}
}

