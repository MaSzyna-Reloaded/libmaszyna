Brake: BrakeValve=EP2 NBpA=4 MBF=85.0 MaxBP=4 BCN=1 BCR=0.203 BCD=0.15 BCM=9.39 HiPP=5 LoPP=3.5 Vv=0.3 MinCP=5.8 MaxCP=7.2 CompressorSpeed=0.0 TareMaxBP=2.5 BVV=88 BM=P10-Bgu BCS=1.5 BSA=5.00 BRE=0.9
// ilosc elementow ciernych na os, max sila docisku ham. recznego [kN], max. cisnienie w cyl., 
// cylindry: ilosc, promien, skok roboczy, mnoznik przekladni,
// cisnienie nominalne w przew. glownym [MPa], opcjonalnie dane dotyczace sprezarki i zbiornika glownego
Cntrl. BrakeHandle=FVel6 BrakeSystem=ElectroPneumatic MaxBPMass=52 BCPN=6 BDelay1=15 BDelay2=4 ASB=Manual LocalBrake=ManualBrake MCPN=3  SCPN=3 IniCDelay=0.0 SCDelay=0.975 CoupledCtrl=Yes CompartmentLightsStart=Manual ConverterStart=Disabled
-1 0.5 	0.0	6  ElectroPneumatic
 0 0.5	-1	4  ElectroPneumatic
 1 0.5  0.35	4  ElectroPneumatic
 2 0.5  0.35    4  Pneumatic
 3 0.5	0.35	4  Pneumatic
 4 -1	0.35	4 Pneumatic
 5 0.34	-1	4  Pneumatic
 6 -1	-1	0 Pneumatic
// tabelka z parametrami nastaw glownego hamulca: cisnienie w przewodzie glowny, cylindrach (-1: nie dotyczy), szybkosc nap.,
// powyzej rodzaj hamulca zespolonego, ilosc nastaw hamulca oraz opoznienie dzialania, typ hamulca niesamoczynnego
// ilosc nastaw glownego i dodatkowego nastawnika, rozruch aut., opoznienia zadzialania stycznikow
