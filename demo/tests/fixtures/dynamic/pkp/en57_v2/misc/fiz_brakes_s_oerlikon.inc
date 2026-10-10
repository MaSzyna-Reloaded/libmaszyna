Brake: BrakeValve=EP2 NBpA=4 MBF=85.0 MaxBP=4.1 BCN=2 BCR=0.178 BCD=0.15 BCM=7.32 HiPP=5 LoPP=3.5 Vv=0.1 MinCP=5.8 MaxCP=7.2 CompressorSpeed=0.0 TareMaxBP=2.8 BVV=100 BM=P10-Bgu BCS=1.5 BSA=4.00 BRE=0.9
// ilosc elementow ciernych na os, max sila docisku ham. recznego [kN], max. cisnienie w cyl., 
// cylindry: ilosc, promien, skok roboczy, mnoznik przekladni,
// cisnienie nominalne w przew. glownym [MPa], opcjonalnie dane dotyczace sprezarki i zbiornika glownego
Cntrl. BrakeSystem=ElectroPneumatic MaxBPMass=75 BCPN=1 BDelay1=15 BDelay2=4 ASB=Manual DynamicBrake=Passive MCPN=3 SCPN=3 AutoRelay=Yes CoupledCtrl=Yes IniCDelay=0.1 SCDelay=0.975 CompartmentLightsStart=Manual BatteryStart=Disabled
-1 0.5 	0.0	6  ElectroPneumatic 
 0 -1	-1	0  ElectroPneumatic
 1 -1   0.35	4  ElectroPneumatic