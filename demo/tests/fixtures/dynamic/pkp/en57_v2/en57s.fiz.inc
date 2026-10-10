// to jest plik z parametrami dla EN57s 
// stale uniwersalne stosowane dla kazdego pojazdu
# prametry hamulca dla SPKS: youBy
Param. Category=train M=57000 Mred=8000 Vmax=110 PWR=608 SandCap=1 Type=ezt HeatingP=22.4
Load: MaxLoad=135 LoadQ=pieces LoadAccepted=Passengers,Pantstate OverLoadFactor=2 LoadSpeed=1 UnLoadSpeed=2
// rodzaj (train,auto,ship), masa [kg], Vmax [km/h], moc [kW] 
Dimensions: L=22.07 H=4.12 W=2.96 Cx=0.1
// dlugosc ze zderzakami, wysokosc, szerokosc [m], wspolcz. oporu aerodynam.
Wheels: D=1.0 AIM=2000 Tw=1.435 Axle=Bo'Bo' Ad=2.75 Bd=15.6 Rmin=80 BearingType=Roll
// srednica kol napednych, moment bezwladosci [kgm^2], szerokosc, uklad i rozstaw osi, rozstaw czopow, min. promien skretu [m]
//include misc/fiz_brakes_s_(p1).inc end
//drzwi: kto je otwiera i zamyka, szybkosc i sposob otwierania
//include misc/fiz_door_set_(p2).inc end
BuffCoupl. CType=Automatic kB=19200 DmaxB=0.05 FmaxB=740 kC=6800 DmaxC=0.15 FmaxC=780 beta=0.58 AllowedFlag=-887 PowerFlag=768
// urzadzenia zderzno-cieglowe, sily w kN (uwaga - sprezystosc kB to 2*sprezystosc pojedynczego zderzaka!)
//Security: AwareSystem=Active,CabSignal AwareDelay=60 SoundSignalDelay=5 EmergencyBrakeDelay=6 RadioStop=Yes 
// systemy zabezpieczajace: czuwaki, sygnalizacja kabinowa itp
Light: Light=Generator LGeneratorEngine=ElectricSeriesMotor LMaxVoltage=110 LMaxCurrent=155  
// zrodlo mocy dla oswietlenia
Clima: Heating=Main HPowerTrans=ElectricPower HMaxVoltage=3000
Power: EnginePower=CurrentCollector CollectorsNo=2 MaxVoltage=3600 MaxCurrent=1200 MinH=0.38 MinV=1900 MaxH=2.4 CSW=1.4 PhysicalLayout=3
// zrodlo mocy dla silnikow trakcyjnych
Engine: EngineType=ElectricSeriesMotor Volt=1500 Trans=19:70 WindingRes=0.3836 nmax=2390
Switches: Pantograph=impulse Converter=impulse RelayResetButton1=1 RelayResetButton2=64

// Parametry stale dla elektrycznych silnikow trakcyjnych


// parametry silnika dla roznych stopni bocznikowania
// pozycja, alfa,beta,gamma(->moment), fi,Isat(->SEM)
// zmiana - fi,Isat->moment, fi,Isat->SEM
MotorParamTable0:      
0 21.52 81 -0.02 148 81 -0.02 1
1 21.68 120 -0.02 148.2 120 -0.02 1
2 21.17 130 0.01 142.3 130 0.01 1
3 22.46 70 0.24 150.1 70 0.24 0
END-MPT  
Circuit: CircuitRes=0.01 ImaxLo=280 ImaxHi=281 IminLo=135 IminHi=175
// informacje o obwodzie elektrycznym

// naglowek listy: ilosc pozycji nastawnika, rodzaj wentylatorow oporow rozr. i ich predkosc obr., rez. zal. went. 
// lista: nastawnik, rezystory rozruchowe, ilosc galezi, ilosc silnikow w galezi, automatycznie przeskakiwanie pozycji
RList: Size=14 RVent=No
0               0		0	0		0
1               17.682		1	4		0
2               14.182		1	4		1
2               10.682		1	4		1
2                8.474		1	4		1
2                6.266		1	4		1
2                4.605		1	4		1
2                2.944		1	4		1
2                1.472		1	4		1
2                0		1	4		1
3                8.841			2	2		1
3                5.341			2	2		1
3                3.133			2	2		1
3                1.472			2	2		1
3                0			2	2		0
END-RL
