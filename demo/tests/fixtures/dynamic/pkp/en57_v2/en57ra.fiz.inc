// to jest plik z parametrami dla EN57ra 
// stale uniwersalne stosowane dla kazdego pojazdu
# prametry hamulca dla SPKS: youBy
Param. Category=train M=34000 Vmax=110 PWR=0 SandCap=500 Type=ezt HeatingP=19.6
// rodzaj (train,auto,ship), masa [kg], Vmax [km/h], moc [kW] 
Load: MaxLoad=115 LoadQ=pieces LoadAccepted=Passengers,Pantstate OverLoadFactor=2 LoadSpeed=1 UnLoadSpeed=2
Dimensions: L=21.41 H=4 W=2.9 Cx=0.1
// dlugosc ze zderzakami, wysokosc, szerokosc [m], wspolcz. oporu aerodynam.
Wheels: D=0.92 AIM=2000 Tw=1.435 Axle=2'2' Ad=2.83 Bd=13.9 Rmin=80 BearingType=Roll
// srednica kol napednych, moment bezwladosci [kgm^2], szerokosc, uklad i rozstaw osi, rozstaw czopow, min. promien skretu [m]
//include misc/fiz_brakes_ra_(p1).inc end
//drzwi: kto je otwiera i zamyka, szybkosc i sposob otwierania
//include misc/fiz_door_set_(p2).inc end
BuffCoupl1. CType=Automatic kB=4200 DmaxB=0.05 FmaxB=1740 kC=6800 DmaxC=0.15 FmaxC=780 beta=0.58 AllowedFlag=39 AutomaticFlag=39
BuffCoupl2. CType=Automatic kB=4200 DmaxB=0.05 FmaxB=1740 kC=6800 DmaxC=0.15 FmaxC=780 beta=0.58 AllowedFlag=-887 PowerFlag=768
Security: AwareSystem=Active,CabSignal AwareDelay=60 SoundSignalDelay=5 MaxHoldTime=1.0 EmergencyBrakeDelay=6 RadioStop=Yes MagnetLocation=5 
// systemy zabezpieczajace: czuwaki, sygnalizacja kabinowa itp
Light: Light=Generator LGeneratorEngine=ElectricSeriesMotor LMaxVoltage=110 LMaxCurrent=155  
Clima: Heating=PowerCable HPowerTrans=ElectricPower HMaxVoltage=3000
Switches: Pantograph=impulse Converter=impulse
