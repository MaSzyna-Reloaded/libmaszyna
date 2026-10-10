/*
   This Source Code Form is subject to the
   terms of the Mozilla Public License, v.
   2.0. If a copy of the MPL was not
   distributed with this file, You can
   obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "../utilities.h"
#include "MOVER.h"
#include "Oerlikon_ESt.h"
#include <cassert>
#include <cmath>

// FIXME: GODOT? Debug flag?
#define DebugModeFlag false


#define Global_AirTemperature 15.f
#define Global_crash_damage true // Globals.h:227 default

//---------------------------------------------------------------------------

// Ra: tu należy przenosić funcje z mover.pas, które nie są z niego wywoływane.
// Jeśli jakieś zmienne nie są używane w mover.pas, też można je przenosić.
// Przeniesienie wszystkiego na raz zrobiło by zbyt wielki chaos do ogarnięcia.

namespace Maszyna {

    const double dEpsilon = 0.01;   // 1cm (zależy od typu sprzęgu...)
    const double CouplerTune = 0.1; // skalowanie tlumiennosci

    int ConversionError = 0;

    std::vector<std::string> const TMoverParameters::eimc_labels = {
            "dfic: ", "dfmax:", "p:    ", "scfu: ", "cim:  ", "icif: ", "Uzmax:",
            "Uzh:  ", "DU:   ", "I0:   ", "fcfu: ", "F0:   ", "a1:   ", "Pmax: ",
            "Fh:   ", "Ph:   ", "Vh0:  ", "Vh1:  ", "Imax: ", "abed: ", "eped: "};

    std::vector<std::string> const TMoverParameters::eimv_labels = {
            "Fkrt:", "Fmax:", "ks:  ", "df:  ", "fp:  ", "Us:  ", "pole:", "Ic:  ", "If:  ", "M:   ", "Fr:  ",
            "Ipoj:", "Pm:  ", "Pe:  ", "eta: ", "fkr: ", "Uzsm:", "Pmax:", "Fzad:", "Imax:", "Fful:"};

    inline double square(double val) // SQR() zle liczylo w current() ...
    {
        return val * val;
    }

    double ComputeCollision(double &v1, double &v2, double m1, double m2, double beta,
                            bool vc) { // oblicza zmiane predkosci i przyrost pedu wskutek kolizji
        assert(beta < 1.0);

        if ((v1 < v2) && (vc == true))
            return 0;
            else {
                double sum = m1 + m2;
                double w1 = (m2 * v2 * 2.0 + v1 * (m1 - m2)) / sum;
                double w2 = (m1 * v1 * 2.0 + v2 * (m2 - m1)) / sum;
                v1 = w1 * std::sqrt(1.0 - beta); // niejawna zmiana predkosci wskutek zderzenia
                v2 = w2 * std::sqrt(1.0 - beta);
                return m1 * (w2 - w1) * (1 - beta);
            }
    }

    int DirPatch(int Coupler1, int Coupler2) { // poprawka dla liczenia sil przy ustawieniu przeciwnym obiektow
        return (Coupler1 != Coupler2 ? 1 : -1);
    }

    int DirF(int CouplerN) {
            switch (CouplerN) {
                case 0:
                    return -1;
                case 1:
                    return 1;
                default:
                    return 0;
            }
    }

    void TSecuritySystem::set_enabled(bool e) {
        if (vigilance_enabled || cabsignal_enabled || radiostop_enabled)
            enabled = e;
        if (CabDependent)
            cabactive = 0;
    }

    void TSecuritySystem::acknowledge_press() {
        pressed = true;

            if (vigilance_timer > AwareDelay) {
                alert_timer = 0.0;
                vigilance_timer = 0.0;
                return;
        }
        vigilance_timer = 0.0;

            if (!separate_acknowledge && cabsignal_active && !cabsignal_lock) {
                cabsignal_active = false;
                alert_timer = 0.0;
        }
    }

    void TSecuritySystem::acknowledge_release() {
        pressed = false;

        if (press_timer > MaxHoldTime)
            alert_timer = 0.0;
        press_timer = 0.0;
    }

    void TSecuritySystem::cabsignal_reset() {
            if (cabsignal_active && !cabsignal_lock) {
                cabsignal_active = false;
                alert_timer = 0.0;
        }
    }

    void TSecuritySystem::update(double dt, double vel, bool pwr, int cab) {
            if (!enabled || !pwr || DebugModeFlag) {
                power = pwr;
                cabsignal_active = false;
                vigilance_timer = 0.0;
                alert_timer = 0.0;
                press_timer = 0.0;
                return;
        }

        bool just_powered_on = !power && pwr;
        bool just_activated = CabDependent && (cabactive != cab);

            /* enabling battery */
            if (cabsignal_enabled && (just_powered_on || just_activated)) {
                cabsignal_active = true;
                alert_timer = SoundSignalDelay;
        }

        power = pwr;
        velocity = vel;
        cabactive = cab;

        if (vigilance_enabled && velocity > AwareMinSpeed)
            vigilance_timer += dt;

        if (pressed && (!is_sifa || velocity > AwareMinSpeed))
            press_timer += dt;

        if (vigilance_timer > AwareDelay || press_timer > MaxHoldTime || cabsignal_active)
            alert_timer += dt;
    }

    void TSecuritySystem::set_cabsignal() {
        if (cabsignal_enabled && power)
            cabsignal_active = true;
    }

    bool TSecuritySystem::has_separate_acknowledge() const {
        return separate_acknowledge;
    }

    bool TSecuritySystem::is_blinking() const {
        if (!power)
            return false;

        return alert_timer > 0.0;
    }

    bool TSecuritySystem::is_vigilance_blinking() const {
        if (!power)
            return false;

        return press_timer > MaxHoldTime || vigilance_timer > AwareDelay;
    }

    bool TSecuritySystem::is_cabsignal_blinking() const {
        if (!power)
            return false;

        return cabsignal_active;
    }

    bool TSecuritySystem::is_beeping() const {
        if (!power)
            return false;

        return alert_timer > SoundSignalDelay && (!separate_acknowledge || is_vigilance_blinking());
    }

    bool TSecuritySystem::is_cabsignal_beeping() const {
        if (!power)
            return false;

        return alert_timer > SoundSignalDelay && is_cabsignal_blinking();
    }

    bool TSecuritySystem::is_braking() const {
        if (!power && enabled)
            return true;

        return alert_timer > SoundSignalDelay + EmergencyBrakeDelay;
    }

    bool TSecuritySystem::radiostop_available() const {
        return radiostop_enabled;
    }

    void TSecuritySystem::set_cabsignal_lock(bool v) {
        cabsignal_lock = v;
    }

    bool TSecuritySystem::is_engine_blocked() const {
        if (!is_sifa)
            return false;

        return velocity < AwareMinSpeed && pressed;
    }

    /*
    void TSecuritySystem::load(std::string const &line, double Vmax) {
        std::string awaresystem = extract_value("AwareSystem", line);
        if (awaresystem.find("Active") != std::string::npos)
            vigilance_enabled = true;
        if (awaresystem.find("CabSignal") != std::string::npos)
            cabsignal_enabled = true;
        if (awaresystem.find("Sifa") != std::string::npos)
            is_sifa = true;
        if (awaresystem.find("SeparateAcknowledge") != std::string::npos)
            separate_acknowledge = true;

        extract_value(AwareDelay, "AwareDelay", line, "");
        AwareMinSpeed = 0.1 * Vmax; // domyślnie 10% Vmax
        extract_value(AwareMinSpeed, "AwareMinSpeed", line, "");
        extract_value(SoundSignalDelay, "SoundSignalDelay", line, "");
        extract_value(EmergencyBrakeDelay, "EmergencyBrakeDelay", line, "");
        extract_value(MaxHoldTime, "MaxHoldTime", line, "");
        extract_value(radiostop_enabled, "RadioStop", line, "");
        extract_value(MagnetLocation, "MagnetLocation", line, "");
        extract_value(CabDependent, "CabDependent", line, "");
    }
    */

    double TableInterpolation(std::map<double, double> &Map, double Parameter) {
        if (Map.size() == 0)
            return 0.0;
        if (Map.size() == 1)
            return Map.begin()->second;

        auto lower = Map.lower_bound(Parameter);
        auto upper = lower;

        if (lower != Map.begin())
            lower--;
        else
            upper++;

            if (upper == Map.end()) {
                lower--;
                upper--;
        }
        double ratio = (upper->second - lower->second) / (upper->first - lower->first);
        return (lower->second + (Parameter - lower->first) * ratio);
    }

    // *************************************************************************************************
    // Q: 20160716
    // Obliczanie natężenie prądu w silnikach
    // *************************************************************************************************
    double TMoverParameters::Current(double n, double U) {
        // wazna funkcja - liczy prad plynacy przez silniki polaczone szeregowo lub rownolegle
        // w zaleznosci od polozenia nastawnikow MainCtrl i ScndCtrl oraz predkosci obrotowej n
        // a takze wywala bezpiecznik nadmiarowy gdy za duzy prad lub za male napiecie
        // jest takze mozliwosc uszkodzenia silnika wskutek nietypowych parametrow

        double R, MotorCurrent;
        double Rz, Delta, Isf;
        double Mn; // przujmuje int, ale dla poprawnosci obliczeń
        double Bn;
        int SP = 0;
        double U1; // napiecie z korekta

        MotorCurrent = 0;
            // i dzialanie hamulca ED w EP09
            if ((DynamicBrakeType == dbrake_automatic) && (TrainType != dt_EZT)) {
                if (((Hamulec->GetEDBCP() < 0.25) && (Vadd < 1)) || (BrakePress > 2.1))
                    DynamicBrakeFlag = false;
                else if ((BrakePress > 0.25) && (Hamulec->GetEDBCP() > 0.25))
                    DynamicBrakeFlag = true;
                DynamicBrakeFlag = (DynamicBrakeFlag && Power110vIsAvailable);
        }
            if ((DynamicBrakeType == dbrake_automatic) && (TrainType == dt_EZT)) {
                DynamicBrakeFlag =
                        (Power110vIsAvailable && (TUHEX_Active || (Vadd > TUHEX_MinIw)) && DynamicBrakeEMUStatus);
        }

            // wylacznik cisnieniowy yBARC - to jest chyba niepotrzebne tutaj   Q: no to usuwam...

            // BrakeSubsystem = ss_LSt;
            // if (BrakeSubsystem == ss_LSt) WriteLog("LSt");
            // if (BrakeSubsystem == ss_LSt) // zrobiona funkcja virtualna
            if (DynamicBrakeFlag) {
                Hamulec->SetED(fabs(Im / 350)); // hamulec ED na EP09 dziala az do zatrzymania lokomotywy
                                                //-     WriteLog("A");
            } else {
                Hamulec->SetED(0);
                //-     WriteLog("B");
            }

        ResistorsFlag = (RList[MainCtrlActualPos].R > 0.01); // and (!DelayCtrlFlag)
        ResistorsFlag = (ResistorsFlag || ((DynamicBrakeFlag == true) && (DynamicBrakeType == dbrake_automatic)));

        if ((TrainType == dt_ET22) && (DelayCtrlFlag) && (MainCtrlActualPos > 1))
            Bn = 1.0 - 1.0 / RList[MainCtrlActualPos].Bn;
        else
            Bn = 1; // to jest wykonywane dla EU07

        R = RList[MainCtrlActualPos].R * Bn + CircuitRes;

            if ((TrainType != dt_EZT) || (Imin != IminLo) || (false == ScndS)) {
                // yBARC - boczniki na szeregu poprawnie
                Mn = RList[MainCtrlActualPos].Mn; // to jest wykonywane dla EU07
            } else {
                Mn = RList[MainCtrlActualPos].Mn * RList[MainCtrlActualPos].Bn;
                    if (RList[MainCtrlActualPos].Bn > 1) {
                        Bn = 1;
                        R = CircuitRes;
                }
            }

            if (DynamicBrakeFlag && (!FuseFlag) && (DynamicBrakeType == dbrake_automatic) && Power110vIsAvailable &&
                Mains) // hamowanie EP09   //TUHEX
            {
                // TODO: zrobic bardziej uniwersalne nie tylko dla EP09
                MotorCurrent = -Max0R(MotorParam[0].fi * (Vadd / (Vadd + MotorParam[0].Isat) - MotorParam[0].fi0), 0) *
                               n * 2.0 / DynamicBrakeRes;
            } else if ((RList[MainCtrlActualPos].Bn == 0) || (false == StLinFlag)) {
                // wylaczone
                MotorCurrent = 0;
            } else { // wlaczone...
                SP = ScndCtrlActualPos;

                    if (ScndCtrlActualPos < 255) // tak smiesznie bede wylaczal
                    {
                            if ((ScndInMain) && (RList[MainCtrlActualPos].ScndAct != 255)) {
                                SP = RList[MainCtrlActualPos].ScndAct;
                        }

                        Rz = Mn * WindingRes + R;

                            if (DynamicBrakeFlag) // hamowanie
                            {
                                    if (DynamicBrakeType > 1) {
                                            // if DynamicBrakeType<>dbrake_automatic then
                                            // MotorCurrent:=-fi*n/Rz  {hamowanie silnikiem na oporach rozruchowych}
                                            /*               begin
                    U:=0;
                    Isf:=Isat;
                    Delta:=SQR(Isf*Rz+Mn*fi*n-U)+4*U*Isf*Rz;
                    MotorCurrent:=(U-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2*Rz)
                    end*/
                                            if ((DynamicBrakeType == dbrake_switch) &&
                                                (TrainType == dt_ET42)) { // z Megapacka
                                                Rz = WindingRes + R;
                                                MotorCurrent = -MotorParam[SP].fi * n /
                                                               Rz; //{hamowanie silnikiem na oporach rozruchowych}
                                        }
                                } else
                                    MotorCurrent = 0; // odciecie pradu od silnika
                            } else {
                                U1 = U + Mn * n * MotorParam[SP].fi0 * MotorParam[SP].fi;
                                //          writepaslog("U1             ", FloatToStr(U1));
                                //          writepaslog("Isat           ", FloatToStr(MotorParam[SP].Isat));
                                //          writepaslog("fi             ", FloatToStr(MotorParam[SP].fi));
                                Isf = Sign(U1) * MotorParam[SP].Isat;
                                //          writepaslog("Isf            ", FloatToStr(Isf));
                                Delta = square(Isf * Rz + Mn * MotorParam[SP].fi * n - U1) +
                                        4.0 * U1 * Isf * Rz; // 105 * 1.67 + Mn * 140.9 * 20.532 - U1
                                                             //          DeltaQ = Isf * Rz + Mn * MotorParam[SP].fi * n
                                                             //          - U1 + 4 * U1 * Isf * Rz; writepaslog("Delta ",
                                                             //          FloatToStr(Delta)); writepaslog("DeltaQ ",
                                                             //          FloatToStr(DeltaQ)); writepaslog("U ",
                                                             //          FloatToStr(U));
                                    if (Mains) {
                                        if (U > 0)
                                            MotorCurrent =
                                                    (U1 - Isf * Rz - Mn * MotorParam[SP].fi * n + std::sqrt(Delta)) /
                                                    (2.0 * Rz);
                                        else
                                            MotorCurrent =
                                                    (U1 - Isf * Rz - Mn * MotorParam[SP].fi * n - std::sqrt(Delta)) /
                                                    (2.0 * Rz);
                                } else
                                    MotorCurrent = 0;
                            } // else DBF

                } // 255
                else
                    MotorCurrent = 0;
            }
        //  writepaslog("MotorCurrent   ", FloatToStr(MotorCurrent));

            if ((DynamicBrakeType == dbrake_switch) && ((BrakePress > 2.0) || (PipePress < 3.6))) {
                Im = 0;
                MotorCurrent = 0;
                // Im:=0;
                Itot = 0;
        } else
            Im = MotorCurrent;

        EnginePower = fabs(Itot) * (1 + RList[MainCtrlActualPos].Mn) * fabs(U) / 1000.0;

        // awarie
        MotorCurrent = fabs(Im); // zmienna pomocnicza

            if (MotorCurrent > 0) {
                if (FuzzyLogic(fabs(n), nmax * 1.1, p_elengproblem))
                    if (MainSwitch(false))
                        EventFlag = true; /*zbyt duze obroty - wywalanie wskutek ognia okreznego*/
                if (TestFlag(DamageFlag, dtrain_engine))
                    if (FuzzyLogic(MotorCurrent, (double)ImaxLo / 10.0, p_elengproblem))
                        if (MainSwitch(false))
                            EventFlag = true; /*uszkodzony silnik (uplywy)*/
                if ((FuzzyLogic(fabs(Im), Imax * 2, p_elengproblem) ||
                     FuzzyLogic(fabs(n), nmax * 1.11, p_elengproblem)))
                    /*       or FuzzyLogic(Abs(U/Mn),2*NominalVoltage,1)) then */ /*poprawic potem*/
                    if ((SetFlag(DamageFlag, dtrain_engine)))
                        EventFlag = true;
                /*! dorobic grzanie oporow rozruchowych i silnika*/
        }

        return Im;
    }

    // *************************************************************************************************
    //  główny konstruktor
    // *************************************************************************************************
    TMoverParameters::TMoverParameters(double VelInitial, std::string TypeNameInit, std::string NameInit, int Cab) :
        TypeName(TypeNameInit), Name(NameInit), CabOccupied(Cab) {
        // WriteLog("------------------------------------------------------"); // GODOT
        // WriteLog("init default physic values for " + NameInit + ", [" + TypeNameInit + "]"); // GODOT
        Dim = TDimension();

        // BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odcięcie (CHK jest jeszcze nie wczytane!)
        iLights[0] = 0;
        iLights[1] = 0; // światła zgaszone

            // inicjalizacja stalych
            for (int b = 0; b < ResArraySize + 1; ++b) {
                RList[b] = TScheme();
            }
        RlistSize = 0;
            for (int b = 0; b < MotorParametersArraySize + 1; ++b) {
                MotorParam[b] = TMotorParameters();
            }

        for (int b = 0; b < 2; ++b)
            for (int k = 0; k < 17; ++k)
                Lights[b][k] = 0;

        for (int b = 0; b < 4; ++b)
            for (int k = 1; k < 9; ++k)
                CompressorList[b][k] = 0;
        CompressorList[0][0] = 0.0;
        CompressorList[1][0] = CompressorList[2][0] = CompressorList[3][0] = 1.0;

            for (int b = -1; b <= MainBrakeMaxPos; ++b) {
                BrakePressureTable[b].PipePressureVal = 0.0;
                BrakePressureTable[b].BrakePressureVal = 0.0;
                BrakePressureTable[b].FlowSpeedVal = 0.0;
            }
        // with BrakePressureTable[-2] do  {pozycja odciecia}
        {
            BrakePressureTable[-2].PipePressureVal = -1.0;
            BrakePressureTable[-2].BrakePressureVal = -1.0;
            BrakePressureTable[-2].FlowSpeedVal = 0.0;
        }
            for (int b = 0; b < 4; ++b) {
                BrakeDelay[b] = 0.0;
            }

            for (int b = 0; b < 2; ++b) // Ra: kto tu zrobił "for b:=1 to 2 do" ???
            {
                Couplers[b].CouplerType = TCouplerType::NoCoupler;
                Couplers[b].SpringKB = 1.0;
                Couplers[b].SpringKC = 1.0;
                Couplers[b].DmaxB = 0.1;
                Couplers[b].FmaxB = 1000.0;
                Couplers[b].DmaxC = 0.1;
                Couplers[b].FmaxC = 1000.0;
            }
            for (int b = 0; b < 3; ++b) {
                BrakeCylMult[b] = 0.0;
            }

            for (int b = 0; b < 26; ++b) {
                eimc[b] = 0.0;
            }
        eimc[eimc_p_eped] = 1.5;

            for (int b = 0; b < 2; ++b) {
                Couplers[b].AllowedFlag = 3; // domyślnie hak i hamulec, inne trzeba włączyć jawnie w FIZ
                Couplers[b].CouplingFlag = 0;
                Couplers[b].Connected = NULL;
                Couplers[b].ConnectedNr = 0; // Ra: to nie ma znaczenia jak nie podłączony
                Couplers[b].Render = false;
                Couplers[b].CForce = 0.0;
                Couplers[b].Dist = 0.0;
                Couplers[b].CheckCollision = false;
            }

            for (int b = 0; b < 5; ++b) {
                MaxBrakePress[b] = 0.0;
            }

        Vel = fabs(VelInitial);
        V = VelInitial / 3.6;

            for (int b = 0; b < 21; b++) {
                eimv[b] = 0.0;
            }

        RunningShape.Len = 1.0;

        RunningTrack.CategoryFlag = CategoryFlag;
        RunningTrack.Width = TrackW;
        RunningTrack.friction = Steel2Steel_friction;
        RunningTrack.QualityFlag = 20;
        RunningTrack.DamageFlag = 0;
        RunningTrack.Velmax = 100.0; // dla uzytku maszynisty w ai_driver}

        RunningTraction.TractionVoltage = 0.0;
        RunningTraction.TractionFreq = 0.0;
        RunningTraction.TractionMaxCurrent = 0.0;
        RunningTraction.TractionResistivity = 1.0;
    };

    double TMoverParameters::Distance(const TLocation &Loc1, const TLocation &Loc2, const TDimension &Dim1,
                                      const TDimension &Dim2) { // zwraca odległość pomiędzy pojazdami (Loc1) i (Loc2) z
                                                                // uwzględnieneim ich długości (kule!)
        return hypot(Loc2.X - Loc1.X, Loc1.Y - Loc2.Y) - 0.5 * (Dim2.L + Dim1.L);
    };

    double
    TMoverParameters::CouplerDist(TMoverParameters const *Left,
                                  TMoverParameters const *Right) { // obliczenie odległości pomiędzy sprzęgami (kula!)
        return Distance(Left->Loc, Right->Loc, Left->Dim, Right->Dim); // odległość pomiędzy sprzęgami (kula!)
    };

    bool TMoverParameters::Attach(
            int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo, int CouplingType, bool Enforce,
            bool Audible) { // łączenie do swojego sprzęgu (ConnectNo) pojazdu (ConnectTo) stroną (ConnectToNr)
                            //  Ra: zwykle wykonywane dwukrotnie, dla każdego pojazdu oddzielnie
                            //  Ra: trzeba by odróżnić wymóg dociśnięcia od uszkodzenia sprzęgu przy podczepianiu AI do
                            //  składu

            if ((ConnectTo == nullptr) || (CouplingType == coupling::faux)) {
                return false;
        }

        auto &coupler{Couplers[ConnectNo]};
        auto &othercoupler = ConnectTo->Couplers[(ConnectToNr != 2 ? ConnectToNr : coupler.ConnectedNr)];
        auto const distance{CouplerDist(this, ConnectTo) - (coupler.adapter_length + othercoupler.adapter_length)};

        auto const couplercheck{(Enforce) || ((distance <= dEpsilon) && (coupler.type() != TCouplerType::NoCoupler) &&
                                              (coupler.type() == othercoupler.type()))};

            if (false == couplercheck) {
                return false;
        }

            // stykaja sie zderzaki i kompatybilne typy sprzegow, chyba że łączenie na starcie
            if (coupler.CouplingFlag == coupling::faux) {
                // jeśli wcześniej nie było połączone, ustalenie z której strony rysować sprzęg
                coupler.Render = true;       // tego rysować
                othercoupler.Render = false; // a tego nie
        };
        auto const couplingchange{CouplingType ^ coupler.CouplingFlag};
        coupler.Connected = ConnectTo;
        coupler.CouplingFlag = CouplingType; // ustawienie typu sprzęgu
            if (ConnectToNr != 2) {
                coupler.ConnectedNr = ConnectToNr; // 2=nic nie podłączone
        }
        othercoupler.Connected = this;
        othercoupler.CouplingFlag = CouplingType;
        othercoupler.ConnectedNr = ConnectNo;

            if ((true == Audible) && (couplingchange != 0)) {
                // set sound event flag
                int soundflag{sound::none};
                std::vector<std::pair<coupling, sound>> const soundmappings = {
                        {coupling::coupler, sound::attachcoupler},   {coupling::brakehose, sound::attachbrakehose},
                        {coupling::mainhose, sound::attachmainhose}, {coupling::control, sound::attachcontrol},
                        {coupling::gangway, sound::attachgangway},   {coupling::heating, sound::attachheating}};
                    for (auto const &soundmapping: soundmappings) {
                            if ((couplingchange & soundmapping.first) != 0) {
                                soundflag |= soundmapping.second;
                        }
                    }
                SetFlag(coupler.sounds, soundflag);
        }

        return true;
    }

    int TMoverParameters::DettachStatus(int ConnectNo) { // Ra: sprawdzenie, czy odległość jest dobra do rozłączania
                                                         // powinny być 3 informacje: =0 sprzęg już rozłączony, <0 da
                                                         // się rozłączyć. >0 nie da się rozłączyć
        if (!Couplers[ConnectNo].Connected)
            return 0; // nie ma nic, to rozłączanie jest OK
        if ((Couplers[ConnectNo].CouplingFlag & ctrain_coupler) == 0)
            return -Couplers[ConnectNo].CouplingFlag; // hak nie połączony - rozłączanie jest OK
        if (TestFlag(DamageFlag, dtrain_coupling))
            return -Couplers[ConnectNo].CouplingFlag; // hak urwany - rozłączanie jest OK
                                                      //    CouplerDist(ConnectNo);
        if ((Couplers[ConnectNo].type() != TCouplerType::Screw) || (Neighbours[ConnectNo].distance < 0.01))
            return -Couplers[ConnectNo].CouplingFlag; // można rozłączać, jeśli dociśnięty
        return (Neighbours[ConnectNo].distance > 0.2) ? -Couplers[ConnectNo].CouplingFlag
                                                      : Couplers[ConnectNo].CouplingFlag;
    };

    bool TMoverParameters::Dettach(int ConnectNo) { // rozlaczanie

        auto &coupler{Couplers[ConnectNo]};
        auto &othervehicle{coupler.Connected};
        auto &othercoupler{othervehicle->Couplers[coupler.ConnectedNr]};

            if (othervehicle == nullptr) {
                return true;
        } // nie ma nic, to odczepiono

        auto couplingchange{coupler.CouplingFlag};          // presume we'll uncouple all active flags
        auto const couplingstate{DettachStatus(ConnectNo)}; // stan sprzęgu
            if (couplingstate < 0) {
                // gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
                std::tie(coupler.Connected, coupler.ConnectedNr, coupler.CouplingFlag) =
                        std::tie(othercoupler.Connected, othercoupler.ConnectedNr, othercoupler.CouplingFlag) =
                                std::make_tuple(nullptr, -1, coupling::faux);
            } else if (couplingstate > 0) { // odłączamy węże i resztę, pozostaje sprzęg fizyczny, który wymaga
                                            // dociśnięcia (z wirtualnym nic)
                coupler.CouplingFlag &= coupling::coupler;
                othercoupler.CouplingFlag &= coupling::coupler;
        }
        // set sound event flag
        couplingchange ^= coupler.CouplingFlag; // remaining bits were removed from coupling
            if (couplingchange != 0) {
                int soundflag{sound::detach}; // HACK: use detach flag to indicate removal of listed coupling
                std::vector<std::pair<coupling, sound>> const soundmappings = {
                        {coupling::coupler, sound::attachcoupler},   {coupling::brakehose, sound::attachbrakehose},
                        {coupling::mainhose, sound::attachmainhose}, {coupling::control, sound::attachcontrol},
                        {coupling::gangway, sound::attachgangway},   {coupling::heating, sound::attachheating}};
                    for (auto const &soundmapping: soundmappings) {
                            if ((couplingchange & soundmapping.first) != 0) {
                                soundflag |= soundmapping.second;
                        }
                    }
                SetFlag(coupler.sounds, soundflag);
        }

        return (couplingstate < 0);
    };

    bool TMoverParameters::DirectionForward() {
            if (false == EIMDirectionChangeAllow()) {
                return false;
        }

            if ((MainCtrlPosNo > 0) && (DirActive < 1) &&
                ((CabActive != 0) || ((InactiveCabFlag & activation::neutraldirection) == 0))) {
                ++DirActive;
                DirAbsolute = DirActive * CabActive;
                SendCtrlToNext("Direction", DirActive, CabActive);
                return true;
        } else if ((DirActive == 1) && (IsMainCtrlNoPowerPos()) && (TrainType == dt_EZT) &&
                   (EngineType != TEngineType::ElectricInductionMotor))
            return MinCurrentSwitch(true); //"wysoki rozruch" EN57
        return false;
    };

    // Nastawianie hamulców

    void TMoverParameters::BrakeLevelSet(
            double b) { // ustawienie pozycji hamulca na wartość (b) w zakresie od -2 do BrakeCtrlPosNo
                        // jedyny dopuszczalny sposób przestawienia hamulca zasadniczego
        if (fBrakeCtrlPos == b)
            return; // nie przeliczać, jak nie ma zmiany
        fBrakeCtrlPos = b;
        if (fBrakeCtrlPos < Handle->GetPos(bh_MIN))
            fBrakeCtrlPos = Handle->GetPos(bh_MIN); // odcięcie
        else if (fBrakeCtrlPos > Handle->GetPos(bh_MAX))
            fBrakeCtrlPos = Handle->GetPos(bh_MAX);
        // TODO: verify whether BrakeCtrlPosR and fBrakeCtrlPos can be rolled into single variable
        BrakeCtrlPosR = fBrakeCtrlPos;
        int x = static_cast<int>(std::floor(fBrakeCtrlPos)); // jeśli odwołujemy się do BrakeCtrlPos w pośrednich, to
                                                             // musi być obcięte a nie zaokrągone
        while ((x > BrakeCtrlPos) && (BrakeCtrlPos < BrakeCtrlPosNo)) // jeśli zwiększyło się o 1
            if (!IncBrakeLevelOld())                                  // T_MoverParameters::
                break;                                                // wyjście awaryjne
        while ((x < BrakeCtrlPos) && (BrakeCtrlPos >= -1))            // jeśli zmniejszyło się o 1
            if (!DecBrakeLevelOld())                                  // T_MoverParameters::
                break;
        BrakePressureActual = BrakePressureTable[BrakeCtrlPos]; // skopiowanie pozycji
        /*
        //youBy: obawiam sie, ze tutaj to nie dziala :P
        //Ra 2014-03: było tak zrobione, że działało - po każdej zmianie pozycji była wywoływana ta
        funkcja
        // if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje ułamki
        if(false)
        if (fBrakeCtrlPos>0.0)
        {//wartości pośrednie wyliczamy tylko dla hamowania
        double u=fBrakeCtrlPos-double(x); //ułamek ponad wartość całkowitą
        if (u>0.0)
        {//wyliczamy wartości ważone
        BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
        //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;
        //to chyba nie będzie tak działać, zwłaszcza w EN57
        BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
        }
        }
        */
    };

    bool TMoverParameters::BrakeLevelAdd(double b) { // dodanie wartości (b) do pozycji hamulca (w tym ujemnej)
                                                     // zwraca false, gdy po dodaniu było by poza zakresem
        BrakeLevelSet(fBrakeCtrlPos + b);
        return b > 0.0 ? (fBrakeCtrlPos < BrakeCtrlPosNo) : (BrakeCtrlPos > -1.0); // true, jeśli można kontynuować
    };

    bool TMoverParameters::IncBrakeLevel() { // nowa wersja na użytek AI, false gdy osiągnięto pozycję BrakeCtrlPosNo
        return BrakeLevelAdd(1.0);
    };

    bool TMoverParameters::DecBrakeLevel() {
        return BrakeLevelAdd(-1.0);
    }; // nowa wersja na użytek AI, false gdy osiągnięto pozycję -1

    bool TMoverParameters::ChangeCab(int direction) { // zmiana kabiny i resetowanie ustawien
            if (std::abs(CabOccupied + direction) < 2) {
                CabOccupied = CabOccupied + direction;
                    if ((BrakeCtrlPosNo > 0) &&
                        ((BrakeSystem == TBrakeSystem::Pneumatic) || (BrakeSystem == TBrakeSystem::ElectroPneumatic))) {
                        BrakeLevelSet(Handle->GetPos(bh_NP));
                        LimPipePress = PipePress;
                        ActFlowSpeed = 0;
                } else
                    BrakeLevelSet(Handle->GetPos(bh_NP));
                MainCtrlPos = MainCtrlNoPowerPos();
                ScndCtrlPos = 0;
                return true;
        }
        return false;
    };

    // rozruch wysoki (true) albo niski (false)
    bool TMoverParameters::CurrentSwitch(bool const State) {

            if (MaxCurrentSwitch(State)) {
                    if (TrainType != dt_EZT) {
                        (MinCurrentSwitch(State));
                }
                return true;
        }
            // TBD, TODO: split off shunt mode toggle into a separate command? It doesn't make much sense to have these
            // two together like that dla 2Ls150
            if ((EngineType == TEngineType::DieselEngine) && (true == ShuntModeAllow) && (DirActive == 0)) {
                // przed ustawieniem kierunku
                ShuntMode = State;
                return true;
        }
            // for SM42/SP42
            if ((EngineType == TEngineType::DieselElectric) && (true == ShuntModeAllow) && (IsMainCtrlNoPowerPos())) {
                ShuntMode = State;
                return true;
        }

        return false;
    };

    bool TMoverParameters::IsMotorOverloadRelayHighThresholdOn() const {

        return ((ImaxHi > ImaxLo) && (Imax > ImaxLo));
    }

    // KURS90 - sprężarka pantografów; Ra 2014-07: teraz jest to zbiornik rozrządu, chociaż to jeszcze nie tak
    void TMoverParameters::UpdatePantVolume(double dt) {
        // check the pantograph compressor while at it
        // TODO: move the check to a separate method
        // automatic start if the pressure is too low
        PantCompFlag |= ((PantPress < 4.2) &&
                         (true == (Pantographs[end::front].is_active |
                                   Pantographs[end::rear].is_active)) // TODO: any_pantograph_is_active method
                         && ((PantographCompressorStart == start_t::automatic) ||
                             (PantographCompressorStart == start_t::manualwithautofallback)));

        auto const lowvoltagepower{Power24vIsAvailable || Power110vIsAvailable};
        PantCompFlag &= lowvoltagepower;

            if ((EnginePowerSource.SourceType == TPowerSource::CurrentCollector) // tylko jeśli pantografujący
                && (EnginePowerSource.CollectorParameters.CollectorsNo > 0)) {
                    // Ra 2014-07: zasadniczo, to istnieje zbiornik rozrządu i zbiornik pantografów - na razie mamy
                    // razem Ra 2014-07: kurek trójdrogowy łączy spr.pom. z pantografami i wyłącznikiem ciśnieniowym WS
                    // Ra 2014-07: zbiornika rozrządu nie pompuje się tu, tylko pantografy; potem można zamknąć
                    // WS i odpalić resztę
                    if (PantAutoValve ? (PantPress < ScndPipePress) : bPantKurek3) // kurek zamyka połączenie z ZG
                    { // zbiornik pantografu połączony ze zbiornikiem głównym - małą sprężarką się tego nie napompuje
                      // Ra 2013-12: Niebugocław mówi, że w EZT nie ma potrzeby odcinać kurkiem
                        PantPress = ScndPipePress;
                        // ograniczenie ciśnienia do MaxPress (tylko w pantografach!)
                        PantPress = clamp(ScndPipePress, 0.0, EnginePowerSource.CollectorParameters.MaxPress);
                        PantVolume = (PantPress + 1.0) * 0.1; // objętość, na wypadek odcięcia kurkiem
                    } else {                                  // zbiornik główny odcięty, można pompować pantografy
                            if (PantCompFlag) {
                                // włączona mała sprężarka
                                PantVolume += dt
                                              // Ra 2013-12: Niebugocław mówi, że w EZT nabija 1.5 raz wolniej niż jak
                                              // było 0.005
                                              * (TrainType == dt_EZT ? 0.003 : 0.005) / std::max(1.0, PantPress) *
                                              (0.45 - ((0.1 / PantVolume / 10) - 0.1)) / 0.45;
                        }
                        PantPress = clamp((10.0 * PantVolume) - 1.0, 0.0,
                                          EnginePowerSource.CollectorParameters
                                                  .MaxPress); // tu by się przydała objętość zbiornika
                    }
                if (!PantCompFlag && (PantVolume > 0.1))
                    PantVolume -= dt * 0.0003 * std::max(1.0, PantPress * 0.5); // nieszczelności: 0.0003=0.3l/s

                    if (PantPress < EnginePowerSource.CollectorParameters.MinPress) {
                            // 3.5 wg http://www.transportszynowy.pl/eu06-07pneumat.php
                            if (true == PantPressSwitchActive) {
                                    // opuszczenie pantografów przy niskim ciśnieniu
                                    if (TrainType != dt_EZT) {
                                            // pressure switch safety measure -- open the line breaker, unless there's
                                            // alternate source of traction voltage
                                            if (GetTrainsetHighVoltage() < EnginePowerSource.CollectorParameters.MinV) {
                                                // TODO: check whether line breaker should be open EMU-wide
                                                MainSwitch(false,
                                                           (TrainType == dt_EZT ? range_t::unit : range_t::local));
                                        }
                                    } else {
                                        // specialized variant for EMU -- pwr system disables converter and heating,
                                        // and prevents their activation until pressure switch is set again
                                        PantPressLockActive = true;
                                        // TODO: separate 'heating allowed' from actual heating flag, so we can disable
                                        // it here without messing up heating toggle
                                        ConverterSwitch(false, range_t::unit);
                                    }
                                // mark the pressure switch as spent
                                PantPressSwitchActive = false;
                        }
                    } else {
                            if (PantPress >= 4.6) {
                                    // NOTE: we require active low power source to prime the pressure switch
                                    // this is a work-around for potential isssues caused by the switch activating on
                                    // otherwise idle vehicles, but should check whether it's accurate
                                    if ((true == Power24vIsAvailable) || (true == Power110vIsAvailable)) {
                                        // prime the pressure switch
                                        PantPressSwitchActive = true;
                                        // turn off the subsystems lock
                                        PantPressLockActive = false;
                                }

                                    if (PantPress >= 4.8) {
                                        // Winger - automatyczne wylaczanie malej sprezarki
                                        // TODO: governor lock, disables usage until pressure drop below 3.8 (should
                                        // really make compressor object we could reuse)
                                        PantCompFlag = false;
                                }
                        }
                    }
            } else { // a tu coś dla SM42 i SM31, aby pokazywać na manometrze
                PantPress = CntrlPipePress;
            }
    };

    void TMoverParameters::UpdateBatteryVoltage(double dt) { // przeliczenie obciążenia baterii
        double sn1 = 0.0, sn2 = 0.0, sn3 = 0.0, sn4 = 0.0,
               sn5 = 0.0; // Ra: zrobić z tego amperomierz NN
            if ((BatteryVoltage > 0) && (EngineType != TEngineType::DieselEngine) &&
                (EngineType != TEngineType::WheelsDriven) && (NominalBatteryVoltage > 0)) {

                // HACK: allow to draw power also from adjacent converter, applicable for EMUs
                // TODO: expand power cables system to include low voltage power transfers
                // HACK: emulate low voltage generator powered directly by the diesel engine
                auto const converteractive{(Power110vIsAvailable) ||
                                           ((EngineType == TEngineType::DieselElectric) && (true == Mains)) ||
                                           ((EngineType == TEngineType::DieselEngine) && (true == Mains))};

                    if ((NominalBatteryVoltage / BatteryVoltage < 1.22) && Battery) { // 110V
                        if (!converteractive)
                            sn1 = (dt * 2.0); // szybki spadek do ok 90V
                        else
                            sn1 = 0;
                        if (converteractive)
                            sn2 = -(dt * 2.0); // szybki wzrost do 110V
                        else
                            sn2 = 0;
                        if (Mains)
                            sn3 = (dt * 0.05);
                        else
                            sn3 = 0;
                        if (iLights[0] & 63) // 64=blachy, nie ciągną prądu //rozpisać na poszczególne żarówki...
                            sn4 = dt * 0.003;
                        else
                            sn4 = 0;
                        if (iLights[1] & 63) // 64=blachy, nie ciągną prądu
                            sn5 = dt * 0.001;
                        else
                            sn5 = 0;
                };
                    if ((NominalBatteryVoltage / BatteryVoltage >= 1.22) && Battery) { // 90V
                        if (PantCompFlag)
                            sn1 = (dt * 0.0046);
                        else
                            sn1 = 0;
                        if (converteractive)
                            sn2 = -(dt * 50); // szybki wzrost do 110V
                        else
                            sn2 = 0;
                        if (Mains)
                            sn3 = (dt * 0.001);
                        else
                            sn3 = 0;
                        if (iLights[0] & 63) // 64=blachy, nie ciągną prądu
                            sn4 = (dt * 0.0030);
                        else
                            sn4 = 0;
                        if (iLights[1] & 63) // 64=blachy, nie ciągną prądu
                            sn5 = (dt * 0.0010);
                        else
                            sn5 = 0;
                };
                    if (!Battery) {
                        if (NominalBatteryVoltage / BatteryVoltage < 1.22)
                            sn1 = dt * 50;
                        else
                            sn1 = 0;
                        sn2 = dt * 0.000001;
                        sn3 = dt * 0.000001;
                        sn4 = dt * 0.000001;
                        sn5 = dt * 0.000001; // bardzo powolny spadek przy wyłączonych bateriach
                };
                BatteryVoltage -= (sn1 + sn2 + sn3 + sn4 + sn5);
                if (NominalBatteryVoltage / BatteryVoltage > 1.57)
                    if (MainSwitch(false) && (EngineType != TEngineType::DieselEngine) &&
                        (EngineType != TEngineType::WheelsDriven))
                        EventFlag = true; // wywalanie szybkiego z powodu zbyt niskiego napiecia
                if (BatteryVoltage > NominalBatteryVoltage)
                    BatteryVoltage = NominalBatteryVoltage; // wstrzymanie ładowania pow. 110V
                if (BatteryVoltage < 0.01)
                    BatteryVoltage = 0.01;
            } else {
                // TODO: check and implement proper way to handle this for diesel engines
                BatteryVoltage = NominalBatteryVoltage;
            }
    };

    /* Ukrotnienie EN57:
       1 //układ szeregowy
       2 //układ równoległy
       3 //bocznik 1
       4 //bocznik 2
       5 //bocznik 3
       6 //do przodu
       7 //do tyłu
       8 //1 przyspieszenie
       9 //minus obw. 2 przyspieszenia
       10 //jazda na oporach
       11 //SHP
       12A //podnoszenie pantografu przedniego
       12B //podnoszenie pantografu tylnego
       13A //opuszczanie pantografu przedniego
       13B //opuszczanie wszystkich pantografów
       14 //załączenie WS
       15 //rozrząd (WS, PSR, wał kułakowy)
       16 //odblok PN
       18 //sygnalizacja przetwornicy głównej
       19 //luzowanie EP
       20 //hamowanie EP
       21 //rezerwa** (1900+: zamykanie drzwi prawych)
       22 //zał. przetwornicy głównej
       23 //wył. przetwornicy głównej
       24 //zał. przetw. oświetlenia
       25 //wył. przetwornicy oświetlenia
       26 //sygnalizacja WS
       28 //sprężarka
       29 //ogrzewanie
       30 //rezerwa* (1900+: zamykanie drzwi lewych)
       31 //otwieranie drzwi prawych
       32H //zadziałanie PN siln. trakcyjnych
       33 //sygnał odjazdu
       34 //rezerwa (sygnalizacja poślizgu)
       35 //otwieranie drzwi lewych
       ZN //masa
       */

    // *****************************************************************************
    // Q: 20160714
    // Oblicza iloraz aktualnej pozycji do maksymalnej hamulca pomocnicznego
    // *****************************************************************************
    double TMoverParameters::LocalBrakeRatio(void) {
        double LBR;
        if (BrakeHandle == TBrakeHandle::MHZ_EN57)
            if ((BrakeOpModeFlag >= bom_EP))
                LBR = Handle->GetEP(BrakeCtrlPosR);
            else
                LBR = 0;
            else {
                if (LocalBrakePosNo > 0)
                    LBR = LocalBrakePosA;
                else
                    LBR = 0;
            }
        // if (TestFlag(BrakeStatus, b_antislip))
        //   LBR = Max0R(LBR, PipeRatio) + 0.4;
        return LBR;
    }

    // *****************************************************************************
    // Q: 20160714
    // Oblicza iloraz aktualnej pozycji do maksymalnej hamulca ręcznego
    // *****************************************************************************
    double TMoverParameters::ManualBrakeRatio(void) {
        double MBR;

        if (ManualBrakePosNo > 0)
            MBR = (double)ManualBrakePos / ManualBrakePosNo;
        else
            MBR = 0;
        return MBR;
    }

    // *****************************************************************************
    // Q: 20160713
    // Zwraca objętość
    // *****************************************************************************
    double TMoverParameters::BrakeVP(void) const {
        if (BrakeVVolume > 0)
            return Volume / (10.0 * BrakeVVolume);
        else
            return 0;
    }

    // *****************************************************************************
    // Q: 20160713
    // Zwraca iloraz różnicy między przewodem kontrolnym i głównym oraz DeltaPipePress
    // *****************************************************************************
    double TMoverParameters::RealPipeRatio(void) {
        double rpp;

        if (DeltaPipePress > 0)
            rpp = (CntrlPipePress - PipePress) / (DeltaPipePress);
        else
            rpp = 0;
        return rpp;
    }

    // *****************************************************************************
    // Q: 20160713
    // Zwraca iloraz ciśnienia w przewodzie do DeltaPipePress
    // *****************************************************************************
    double TMoverParameters::PipeRatio(void) {
        double pr;

        if (DeltaPipePress > 0)
                if (false) // SPKS!! no to jak nie wchodzimy to po co branch?
                {
                    if ((3.0 * PipePress) > (HighPipePress + LowPipePress + LowPipePress))
                        pr = (HighPipePress - Min0R(HighPipePress, PipePress)) / (DeltaPipePress * 4.0 / 3.0);
                    else
                        pr = (HighPipePress - 1.0 / 3.0 * DeltaPipePress - Max0R(LowPipePress, PipePress)) /
                             (DeltaPipePress * 2.0 / 3.0);
                    // if (not TestFlag(BrakeStatus, b_Ractive))
                    //     and(BrakeMethod and 1 = 0) and TestFlag(BrakeDelays, bdelay_R) and (Power < 1) and
                    //         (BrakeCtrlPos < 1) then pr : = Min0R(0.5, pr);
                    // if (Compressor > 0.5)
                    //     then pr : = pr * 1.333; // dziwny rapid wywalamy
            } else
                pr = (HighPipePress - Max0R(LowPipePress, Min0R(HighPipePress, PipePress))) / DeltaPipePress;
        else
            pr = 0;
        return pr;
    }

    double TMoverParameters::EngineRPMRatio() const {

        return clamp((EngineType == TEngineType::DieselElectric
                              ? ((60.0 * std::fabs(enrot)) / DElist[MainCtrlPosNo].RPM)
                      : EngineType == TEngineType::DieselEngine ? (std::fabs(enrot) / nmax)
                                                                : 1.0), // shouldn't ever get here but, eh
                     0.0, 1.0);
    }

    double TMoverParameters::EngineIdleRPM() const {

        return (EngineType == TEngineType::DieselEngine ? dizel_nmin * 60
                : EngineType == TEngineType::DieselElectric
                        ? DElist[MainCtrlNoPowerPos()].RPM
                        : std::numeric_limits<double>::max()); // shouldn't ever get here but, eh
    }

    double TMoverParameters::EngineMaxRPM() const {

        return (EngineType == TEngineType::DieselEngine ? dizel_nmax * 60
                : EngineType == TEngineType::DieselElectric
                        ? DElist[MainCtrlPosNo].RPM
                        : std::numeric_limits<double>::max()); // shouldn't ever get here but, eh
    }

    // *************************************************************************************************
    // Q: 20160716
    // Wykrywanie kolizji
    // *************************************************************************************************
    void TMoverParameters::CollisionDetect(int const End, double const dt) {
                if (Neighbours[End].vehicle == nullptr) {
                    return;
            } // shouldn't normally happen but, eh

            auto &coupler{Couplers[End]};
            auto *othervehicle{Neighbours[End].vehicle};
            auto const otherend{Neighbours[End].vehicle_end};
            auto &othercoupler{othervehicle->Couplers[otherend]};

            auto velocity{V};
            auto othervehiclevelocity{othervehicle->V};
            // calculate collision force and new velocities for involved vehicles
            auto const VirtualCoupling{(coupler.CouplingFlag == coupling::faux)};
            auto CCF{0.0};

                switch (End) {
                        case 0: {
                            CCF = ComputeCollision(velocity, othervehiclevelocity, TotalMass, othervehicle->TotalMass,
                                                   (coupler.beta + othercoupler.beta) / 2.0, VirtualCoupling) /
                                  (dt);
                            break; // yB: ej ej ej, a po
                        }
                        case 1: {
                            CCF = ComputeCollision(othervehiclevelocity, velocity, othervehicle->TotalMass, TotalMass,
                                                   (coupler.beta + othercoupler.beta) / 2.0, VirtualCoupling) /
                                  (dt);
                            break;
                        }
                        default: {
                            break;
                        }
                }

                if (Global_crash_damage) {
                        if ((-coupler.Dist >= coupler.DmaxB) &&
                            (FuzzyLogic(std::fabs(CCF), 5.0 * (coupler.FmaxC + 1.0), p_coupldmg))) {
                            // small chance to smash the coupler if it's hit with excessive force
                            damage_coupler(End);
                    }

                        if ((coupler.CouplingFlag == coupling::faux ||
                             (true == TestFlag(othervehicle->DamageFlag,
                                               dtrain_out)))) { // HACK: limit excessive speed derailment checks to
                                                                // vehicles which aren't part of the same consist
                            auto const safevelocitylimit{15.0};
                            // original: glm::length( glm::angleAxis( Rot.Rz, {0,1,0} ) * V - ... ) (Mover.cpp:1242),
                            // expanded here without glm: angleAxis gives (cos(a/2), 0, sin(a/2), 0)
                            auto const velocitydifference{
                                    hypot(std::cos(0.5 * Rot.Rz) * V - std::cos(0.5 * othervehicle->Rot.Rz) * othervehicle->V,
                                          std::sin(0.5 * Rot.Rz) * V - std::sin(0.5 * othervehicle->Rot.Rz) * othervehicle->V) *
                                    3.6}; // m/s -> km/h

                                if (velocitydifference > safevelocitylimit) {
                                        // HACK: crude estimation for potential derail, will take place with velocity
                                        // difference > 15 km/h adjusted for vehicle mass ratio
                                        if ((false == TestFlag(DamageFlag, dtrain_out)) ||
                                            (false == TestFlag(othervehicle->DamageFlag, dtrain_out))) {
                                            // WriteLog("Bad driving: " + Name + " and " + othervehicle->Name + " collided ..."); // GODOT
                                    }

                                        if (velocitydifference >
                                            safevelocitylimit * (TotalMass / othervehicle->TotalMass)) {
                                            derail(5);
                                    }
                                        if (velocitydifference >
                                            safevelocitylimit * (othervehicle->TotalMass / TotalMass)) {
                                            othervehicle->derail(5);
                                    }
                            }
                    }
            }

                // adjust velocity and acceleration of affected vehicles
                if (false == TestFlag(DamageFlag, dtrain_out)) {
                    auto const accelerationchange{(velocity - V) / dt};
                    //       if( accelerationchange / AccS < 1.0 ) {
                    // HACK: prevent excessive vehicle pinball cases
                    AccS += accelerationchange;
                    //           AccS = clamp( AccS, -2.0, 2.0 );
                    V = velocity;
                    //        }
            }
                if (false == TestFlag(othervehicle->DamageFlag, dtrain_out)) {
                    auto const othervehicleaccelerationchange{(othervehiclevelocity - othervehicle->V) / dt};
                    //        if( othervehicleaccelerationchange / othervehicle->AccS < 1.0 ) {
                    // HACK: prevent excessive vehicle pinball cases
                    othervehicle->AccS += othervehicleaccelerationchange;
                    othervehicle->V = othervehiclevelocity;
                    //        }
            }
    }

    void TMoverParameters::damage_coupler(int const End) {

        auto &coupler{Couplers[End]};

            if (coupler.type() == TCouplerType::Articulated) {
                return;
        } // HACK: don't break articulated couplings no matter what

        if (SetFlag(DamageFlag, dtrain_coupling))
            EventFlag = true;

            if ((coupler.CouplingFlag & coupling::brakehose) == coupling::brakehose) {
                // hamowanie nagle - zerwanie przewodow hamulcowych
                AlarmChainFlag = true;
        }

        coupler.CouplingFlag = 0;

            if (coupler.Connected != nullptr) {
                    switch (End) {
                            // break connection with other vehicle, if there's any
                            case 0: {
                                coupler.Connected->Couplers[end::rear].CouplingFlag = coupling::faux;
                                break;
                            }
                            case 1: {
                                coupler.Connected->Couplers[end::front].CouplingFlag = coupling::faux;
                                break;
                            }
                            default: {
                                break;
                            }
                    }
        }

        // WriteLog("Bad driving: " + Name + " broke a coupler"); // GODOT
    }

    void TMoverParameters::derail(int const Reason) {

            if (SetFlag(DamageFlag, dtrain_out)) {

                DerailReason = Reason; // TODO: enum derail causes
                EventFlag = true;
                MainSwitch(false, range_t::local);

                AccS *= 0.65;
                V *= 0.65;
                    if (Vel < 5.0) {
                        // HACK: prevent permanent axle spin in static vehicle after a collision
                        nrot = 0.0;
                        SlippingWheels = false;
                }

                // WriteLog("Bad driving: " + Name + " derailed"); // GODOT
        }
    }

    // *************************************************************************************************
    // Oblicza przemieszczenie taboru
    // *************************************************************************************************
    double TMoverParameters::ComputeMovement(double dt, double dt1, const TTrackShape &Shape, TTrackParam &Track,
                                             TTractionParam &ElectricTraction, TLocation const &NewLoc,
                                             TRotation const &NewRot) {
        const double Vepsilon = 1e-5;
        const double Aepsilon = 1e-3; // ASBSpeed=0.8;

            if (!TestFlag(DamageFlag, dtrain_out)) { // Ra: to przepisywanie tu jest bez sensu
                RunningShape = Shape;
                RunningTrack = Track;
                RunningTraction = ElectricTraction;

                // if (!DynamicBrakeFlag)
                //     RunningTraction.TractionVoltage = ElectricTraction.TractionVoltage /*-
                //                                       fabs(ElectricTraction.TractionResistivity *
                //                                           (Itot + HVCouplers[0][0] + HVCouplers[1][0]))*/;
                // else
                //     RunningTraction.TractionVoltage =
                //         ElectricTraction.TractionVoltage /*-
                //         fabs(ElectricTraction.TractionResistivity * Itot *
                //             0)*/; // zasadniczo ED oporowe nie zmienia napięcia w sieci
        }

        if (CategoryFlag == 4)
            OffsetTrackV = TotalMass / (Dim.L * Dim.W * 1000.0);
        else if (TestFlag(CategoryFlag, 1) && TestFlag(RunningTrack.CategoryFlag, 1))
                if (TestFlag(DamageFlag, dtrain_out)) {
                    OffsetTrackV = -0.2;
                    OffsetTrackH = Sign(RunningShape.R) * 0.2;
            }

        // TODO: investigate, seems supplied NewRot is always 0 although the code here suggests some actual values are
        // expected
        Loc = NewLoc;
        Rot = NewRot;

            if (dL == 0) // oblicz przesuniecie}
            {
                auto const AccSprev{AccS};
                // przyspieszenie styczne
                AccS = interpolate(AccSprev, FTotal / TotalMass, 0.5);
                // clamp( dt * 3.0, 0.0, 1.0 ) ); // prawo Newtona ale z wygladzaniem (średnia z poprzednim)

                if (TestFlag(DamageFlag, dtrain_out))
                    AccS = -Sign(V) * g * 1; // random(0.0, 0.1)

                // przyspieszenie normalne
                if (fabs(Shape.R) > 0.01)
                    AccN = square(V) / Shape.R + g * Shape.dHrail / TrackW; // Q: zamieniam SQR() na sqr()
                else
                    AccN = g * Shape.dHrail / TrackW;

                // velocity change
                auto const Vprev{V};
                V += (3.0 * AccS - AccSprev) * dt / 2.0; // przyrost predkosci
                    if ((V * Vprev <= 0) && (std::fabs(FStand) > std::fabs(FTrain))) {
                        // tlumienie predkosci przy hamowaniu
                        // zahamowany
                        V = 0;
                }

                // tangential acceleration, from velocity change
                AccSVBased = interpolate(AccSVBased, (V - Vprev) / dt, clamp(dt * 3.0, 0.0, 1.0));

                // vertical acceleration
                AccVert = (std::fabs(AccVert) < 0.01 ? 0.0 : AccVert * 0.5);
                    // szarpanie
                    /*
                #ifdef EU07_USE_FUZZYLOGIC
                if( FuzzyLogic( ( 10.0 + Track.DamageFlag ) * Mass * Vel / Vmax, 500000.0, p_accn ) ) {
                    // Ra: czemu tu masa bez ładunku?
                    AccV /= ( 2.0 * 0.95 + 2.0 * Random() * 0.1 ); // 95-105% of base modifier (2.0)
                    }
                    else
                #endif
                AccV = AccV / 2.0;

                if (AccV > 1.0)
                AccN += (7.0 - Random(5)) * (100.0 + Track.DamageFlag / 2.0) * AccV / 2000.0;
                */
                    // wykolejanie na luku oraz z braku szyn
                    if (TestFlag(CategoryFlag, 1)) {
                        if (FuzzyLogic((AccN / g) * (1.0 + 0.1 * (Track.DamageFlag && dtrack_freerail)), TrackW / Dim.H,
                                       1) ||
                            TestFlag(Track.DamageFlag, dtrack_norail))
                                if (SetFlag(DamageFlag, dtrain_out)) {
                                    EventFlag = true;
                                    MainSwitch(false, range_t::local);
                                    RunningShape.R = 0;
                                    if (TestFlag(Track.DamageFlag, dtrack_norail))
                                        DerailReason = 1; // Ra: powód wykolejenia: brak szyn
                                    else
                                        DerailReason = 2; // Ra: powód wykolejenia: przewrócony na łuku
                            }
                        // wykolejanie na poszerzeniu toru
                        if (FuzzyLogic(fabs(Track.Width - TrackW), TrackW / 10.0, 1))
                                if (SetFlag(DamageFlag, dtrain_out)) {
                                    EventFlag = true;
                                    MainSwitch(false, range_t::local);
                                    RunningShape.R = 0;
                                    DerailReason = 3; // Ra: powód wykolejenia: za szeroki tor
                            }
                }
                // wykolejanie wkutek niezgodnosci kategorii toru i pojazdu
                if (!TestFlag(RunningTrack.CategoryFlag, CategoryFlag))
                        if (SetFlag(DamageFlag, dtrain_out)) {
                            EventFlag = true;
                            MainSwitch(false, range_t::local);
                            DerailReason = 4; // Ra: powód wykolejenia: nieodpowiednia trajektoria
                    }
                    if ((true == TestFlag(DamageFlag, dtrain_out)) && (Vel < 1.0)) {
                        V = 0.0;
                        AccS = 0.0;
                }

                // dL:=(V+AccS*dt/2)*dt;
                // przyrost dlugosci czyli przesuniecie
                dL = (3.0 * V - Vprev) * dt / 2.0; // metoda Adamsa-Bashfortha}
                                                   // ale jesli jest kolizja (zas. zach. pedu) to...}
                for (int b = 0; b < 2; b++)
                        if (Couplers[b].CheckCollision) {
                            CollisionDetect(b, dt); // zmienia niejawnie AccS, V !!!
                    }

        } // liczone dL, predkosc i przyspieszenie

        auto const d{(EngineType == TEngineType::WheelsDriven ? dL * CabActive : // na chwile dla testu
                              dL)};

        DistCounter += fabs(dL) / 1000.0;
        dL = 0;

        // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze
        compute_movement_(dt);

        // security system
        SecuritySystemCheck(dt1);

        return d;
    };

    // *************************************************************************************************
    // Oblicza przemieszczenie taboru - uproszczona wersja
    // *************************************************************************************************

    double TMoverParameters::FastComputeMovement(double dt, const TTrackShape &Shape, TTrackParam &Track,
                                                 TLocation const &NewLoc, TRotation const &NewRot) {
        int b;
        // T_MoverParameters::FastComputeMovement(dt, Shape, Track, NewLoc, NewRot);

        Loc = NewLoc;
        Rot = NewRot;

            if (dL == 0) // oblicz przesuniecie
            {
                auto const AccSprev{AccS};
                // przyspieszenie styczne
                AccS = interpolate(AccSprev, FTotal / TotalMass, 0.5);
                // clamp( dt * 3.0, 0.0, 1.0 ) ); // prawo Newtona ale z wygladzaniem (średnia z poprzednim)

                if (TestFlag(DamageFlag, dtrain_out))
                    AccS = -Sign(V) * g * 1; // * random(0.0, 0.1)

                // simple mode skips calculation of normal acceleration

                // velocity change
                auto const Vprev{V};
                V += (3.0 * AccS - AccSprev) * dt / 2.0; // przyrost predkosci
                    if ((V * Vprev <= 0) && (std::fabs(FStand) > std::fabs(FTrain))) {
                        // tlumienie predkosci przy hamowaniu
                        // zahamowany
                        V = 0;
                }

                // simple mode skips calculation of tangential acceleration

                // simple mode skips calculation of vertical acceleration
                AccVert = 0.0;

                    if ((true == TestFlag(DamageFlag, dtrain_out)) && (Vel < 1.0)) {
                        V = 0.0;
                        AccS = 0.0;
                }

                dL = (3.0 * V - Vprev) * dt / 2.0; // metoda Adamsa-Bashfortha
                                                   // ale jesli jest kolizja (zas. zach. pedu) to...
                    for (b = 0; b < 2; b++) {
                            if (Couplers[b].CheckCollision) {
                                CollisionDetect(b, dt); // zmienia niejawnie AccS, V !!!
                        }
                    }
        } // liczone dL, predkosc i przyspieszenie

        auto const d{(EngineType == TEngineType::WheelsDriven ? dL * CabActive : // na chwile dla testu
                              dL)};

        DistCounter += fabs(dL) / 1000.0;
        dL = 0;

        // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze
        compute_movement_(dt);

        return d;
    };

    // updates shared between 'fast' and regular movement computation methods
    void TMoverParameters::compute_movement_(double const Deltatime) {

        // sprawdzanie i ewentualnie wykonywanie->kasowanie poleceń
        if (LoadStatus > 0)                  // czas doliczamy tylko jeśli trwa (roz)ładowanie
            LastLoadChangeTime += Deltatime; // czas (roz)ładunku

        RunInternalCommand();

            // relay settings
            if (EngineType == TEngineType::ElectricSeriesMotor) {
                    // adjust motor overload relay threshold
                    if (ImaxHi > ImaxLo) {
                            if (MotorOverloadRelayHighThreshold) { // set high threshold
                                    if ((TrainType != dt_ET42) ? (RList[MainCtrlPos].Bn < 2) : (MainCtrlPos == 0)) {
                                        Imax = ImaxHi;
                                }
                            } else { // set low threshold
                                    if ((TrainType != dt_ET42) || (MainCtrlPos == 0)) {
                                        Imax = ImaxLo;
                                }
                            }
                }
        }

            // automatyczny rozruch
            if (EngineType == TEngineType::ElectricSeriesMotor) {
                    if (AutoRelayCheck()) {
                        SetFlag(SoundFlag, sound::relay);
                }
        }

            if ((EngineType == TEngineType::DieselEngine) || (EngineType == TEngineType::DieselElectric)) {
                    if (dizel_Update(Deltatime)) {
                        SetFlag(SoundFlag, sound::relay);
                }
        }

        // TODO: gather and move current calculations to dedicated method
        TotalCurrent = 0;

        // low voltage power sources
        LowVoltagePowerCheck(Deltatime);
        // power sources
        PantographsCheck(Deltatime);
        // main circuit
        MainsCheck(Deltatime);
        // traction motors
        MotorBlowersCheck(Deltatime);
        // uklady hamulcowe:
        ConverterCheck(Deltatime);
        if (VeselVolume > 0)
            Compressor = CompressedVolume / VeselVolume;
            else {
                Compressor = 0;
                CompressorFlag = false;
            };
            if (VeselVolume > 0.0) {
                // sprężarka musi mieć jakąś niezerową wydajność żeby rozważać jej załączenie i pracę
                CompressorCheck(Deltatime);
        }
            if (Power > 1.0) {
                // w rozrządczym nie (jest błąd w FIZ!) - Ra 2014-07: teraz we wszystkich
                UpdatePantVolume(Deltatime); // Ra 2014-07: obsługa zbiornika rozrządu oraz pantografów
        }
        // heating
        HeatingCheck(Deltatime);
        // lighting
        LightsCheck(Deltatime);

        UpdateBrakePressure(Deltatime);
        UpdatePipePressure(Deltatime);
        UpdateBatteryVoltage(Deltatime);
        UpdateScndPipePressure(Deltatime); // druga rurka, youBy

            if (((DCEMUED_CC & 1) != 0) && ((Couplers[end::front].CouplingFlag & coupling::control) != 0)) {
                DynamicBrakeEMUStatus &= Couplers[end::front].Connected->DynamicBrakeEMUStatus;
        }
            if (((DCEMUED_CC & 2) != 0) && ((Couplers[end::rear].CouplingFlag & coupling::control) != 0)) {
                DynamicBrakeEMUStatus &= Couplers[end::rear].Connected->DynamicBrakeEMUStatus;
        }

            if ((BrakeSlippingTimer > 0.8) && (ASBType != 128)) { // ASBSpeed=0.8
                                                                  // hamulec antypoślizgowy - wyłączanie
                Hamulec->ASB(0);
        }
        BrakeSlippingTimer += Deltatime;
        // automatic doors
        update_doors(Deltatime);

        PowerCouplersCheck(Deltatime, coupling::highvoltage);
        PowerCouplersCheck(Deltatime, coupling::power110v);
        PowerCouplersCheck(Deltatime, coupling::power24v);

        Power24vVoltage = std::max(PowerCircuits[0].first, GetTrainsetVoltage(coupling::power24v));
        Power24vIsAvailable = (Power24vVoltage > 0);
        Power110vIsAvailable = ((PowerCircuits[1].first > 0) || (GetTrainsetVoltage(coupling::power110v) > 0));
    }

    void TMoverParameters::MainsCheck(double const Deltatime) {

        //    if( MainsInitTime == 0.0 ) { return; }

        // TBD, TODO: move voltage calculation to separate method and use also in power coupler state calculation?
        auto localvoltage{0.0};
            switch (EnginePowerSource.SourceType) {
                    case TPowerSource::CurrentCollector: {
                        localvoltage = std::max(localvoltage, PantographVoltage);
                        break;
                    }
                    case TPowerSource::Accumulator: {
                        localvoltage = std::max(localvoltage, Power24vVoltage);
                        break;
                    }
                    default: {
                        break;
                    }
            }
        auto const maincircuitpowersupply{(std::fabs(localvoltage) > 0.1) || (GetTrainsetHighVoltage() > 0.1)};

            if (true == maincircuitpowersupply) {
                    // all is well
                    if (MainsInitTimeCountdown >= 0.0) {
                        // NOTE: we ensure main circuit readiness meets condition MainsInitTimeCountdown < 0
                        // this allows for simpler rejection of cases where MainsInitTime == 0
                        MainsInitTimeCountdown -= Deltatime;
                    } else {
                            // optional automatic circuit start
                            if ((MainsStart != start_t::manual) && (false == (Mains || dizel_startup))) {
                                MainSwitch(true);
                        }
                    }
            } else {
                // no power supply
                MainsInitTimeCountdown = MainsInitTime;
            }
    }

    void TMoverParameters::LowVoltagePowerCheck(double const Deltatime) {

        auto const lowvoltagepower{Power24vIsAvailable || Power110vIsAvailable};

            switch (EngineType) {
                    case TEngineType::ElectricSeriesMotor: {
                        GroundRelay &= lowvoltagepower;
                            if (GroundRelayStart != start_t::manual) {
                                // NOTE: we're ignoring intricaties of battery and converter types as they're unlikely
                                // to be used
                                // TODO: generic check method which takes these into account
                                GroundRelay |= lowvoltagepower;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
            }
    }

    void TMoverParameters::PowerCouplersCheck(double const Deltatime, coupling const Coupling) {

        auto localvoltage{0.0};

            // local power sources
            // TODO: make local voltage calculations a separate method, store results in PowerCircuit fields
            switch (Coupling) {

                    case coupling::highvoltage: {
                            // heating power sources
                            if (Heating) {
                                    switch (HeatingPowerSource.SourceType) {
                                            case TPowerSource::Generator: {
                                                localvoltage = HeatingPowerSource.EngineGenerator.voltage -
                                                               TotalCurrent * 0.02;
                                                break;
                                            }
                                            case TPowerSource::CurrentCollector: {
                                                localvoltage = PantographVoltage;
                                                break;
                                            }
                                            case TPowerSource::Main: {
                                                // HACK: main circuit can be fed through couplers, so we explicitly
                                                // check pantograph supply here
                                                localvoltage = (true == Mains ? PantographVoltage : 0.0);
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                    }
                        }
                            // high voltage power sources
                            switch (EnginePowerSource.SourceType) {
                                    case TPowerSource::CurrentCollector: {
                                        localvoltage = std::max(localvoltage, PantographVoltage);
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                            }
                        break;
                    }

                    case coupling::power110v: {
                            if (ConverterFlag) {
                                localvoltage = NominalBatteryVoltage;
                        }
                        // TBD, TODO: reduce by current draw?
                        PowerCircuits[1].first = localvoltage;
                        break;
                    }

                    case coupling::power24v: {
                            if (Battery) {
                                localvoltage = BatteryVoltage;
                        }
                        // TBD, TODO: reduce by current draw?
                        PowerCircuits[0].first = localvoltage;
                        break;
                    }

                    default: {
                        break;
                    }
            }

        auto const abslocalvoltage{std::fabs(localvoltage)};
        auto const localpowersource{(abslocalvoltage > 1.0)};

            // przekazywanie napiec
            for (auto side = 0; side < 2; ++side) {

                auto &coupler{Couplers[side]};
                // NOTE: in the loop we actually update the state of the coupler on the opposite end of the vehicle
                auto &oppositecoupler{Couplers[(side == end::front ? end::rear : end::front)]};

                bool oppositecouplingispresent;
                bool localpowerexportisenabled;

                    switch (Coupling) {

                            case coupling::highvoltage: {
                                auto const oppositehighvoltagecoupling{
                                        (oppositecoupler.CouplingFlag & coupling::highvoltage) != 0};
                                auto const oppositeheatingcoupling{(oppositecoupler.CouplingFlag & coupling::heating) !=
                                                                   0};

                                oppositecouplingispresent = (oppositehighvoltagecoupling || oppositeheatingcoupling);
                                localpowerexportisenabled = (oppositehighvoltagecoupling ||
                                                             (oppositeheatingcoupling && localpowersource && Heating));
                                break;
                            }

                            case coupling::power110v: {
                                oppositecouplingispresent =
                                        (TestFlag(oppositecoupler.CouplingFlag, oppositecoupler.PowerCoupling)) &&
                                        ((oppositecoupler.PowerFlag & coupling::power110v) != 0);
                                localpowerexportisenabled = (oppositecouplingispresent);
                                break;
                            }

                            case coupling::power24v: {
                                oppositecouplingispresent =
                                        (TestFlag(oppositecoupler.CouplingFlag, oppositecoupler.PowerCoupling)) &&
                                        ((oppositecoupler.PowerFlag & coupling::power24v) != 0);
                                localpowerexportisenabled = (oppositecouplingispresent);
                                break;
                            }

                            default: {
                                break;
                            }
                    }

                auto const *coupling = (Coupling == coupling::highvoltage ? &coupler.power_high
                                        : Coupling == coupling::power110v ? &coupler.power_110v
                                        : Coupling == coupling::power24v  ? &coupler.power_24v
                                                                          : nullptr);
                auto *oppositecoupling = (Coupling == coupling::highvoltage ? &oppositecoupler.power_high
                                          : Coupling == coupling::power110v ? &oppositecoupler.power_110v
                                          : Coupling == coupling::power24v  ? &oppositecoupler.power_24v
                                                                            : nullptr);

                // start with base voltage
                oppositecoupling->voltage = abslocalvoltage;
                oppositecoupling->is_live = false;
                oppositecoupling->is_local = localpowersource; // indicate power source
                                                               // draw from external source
                    if (coupler.Connected != nullptr) {
                        auto const &connectedcoupler{coupler.Connected->Couplers[coupler.ConnectedNr]};
                        auto const *connectedcoupling =
                                (Coupling == coupling::highvoltage ? &connectedcoupler.power_high
                                 : Coupling == coupling::power110v ? &connectedcoupler.power_110v
                                 : Coupling == coupling::power24v  ? &connectedcoupler.power_24v
                                                                   : nullptr);
                        auto const connectedvoltage{(connectedcoupling->is_live ? connectedcoupling->voltage : 0.0)};
                        oppositecoupling->voltage =
                                std::max(oppositecoupling->voltage, connectedvoltage - coupling->current * 0.02);
                        oppositecoupling->is_live = (connectedvoltage > 0.1) && (oppositecouplingispresent);
                }
                    // draw from local source
                    if (localpowersource) {
                        oppositecoupling->voltage =
                                std::max(oppositecoupling->voltage, abslocalvoltage - coupling->current * 0.02);
                        oppositecoupling->is_live |= (abslocalvoltage > 0.1) && (localpowerexportisenabled);
                }
            }

        // przekazywanie pradow
        auto couplervoltage{0};
            switch (Coupling) {
                    case coupling::highvoltage: {
                        couplervoltage =
                                Couplers[end::front].power_high.voltage + Couplers[end::rear].power_high.voltage;
                        break;
                    }
                    case coupling::power110v: {
                        couplervoltage =
                                Couplers[end::front].power_110v.voltage + Couplers[end::rear].power_110v.voltage;
                        break;
                    }
                    case coupling::power24v: {
                        couplervoltage = Couplers[end::front].power_24v.voltage + Couplers[end::rear].power_24v.voltage;
                        break;
                    }
                    default: {
                        break;
                    }
            }

        auto *totalcurrent = (Coupling == coupling::highvoltage ? &TotalCurrent
                              : Coupling == coupling::power110v ? &PowerCircuits[1].second
                              : Coupling == coupling::power24v  ? &PowerCircuits[0].second
                                                                : nullptr);

            for (auto side = 0; side < 2; ++side) {

                auto &coupler{Couplers[side]};
                auto *coupling = (Coupling == coupling::highvoltage ? &coupler.power_high
                                  : Coupling == coupling::power110v ? &coupler.power_110v
                                  : Coupling == coupling::power24v  ? &coupler.power_24v
                                                                    : nullptr);

                coupling->current = 0.0;

                    if (coupler.Connected == nullptr) {
                        continue;
                }

                auto const &connectedothercoupler{
                        coupler.Connected->Couplers[(coupler.ConnectedNr == end::front ? end::rear : end::front)]};
                auto const *connectedothercoupling =
                        (Coupling == coupling::highvoltage ? &connectedothercoupler.power_high
                         : Coupling == coupling::power110v ? &connectedothercoupler.power_110v
                         : Coupling == coupling::power24v  ? &connectedothercoupler.power_24v
                                                           : nullptr);
                auto const extracurrent =
                        (Coupling == coupling::highvoltage ? std::fabs(Itot) * IsVehicleEIMBrakingFactor() : 0.0);

                    if (false == localpowersource) {
                            // bez napiecia...
                            if (couplervoltage != 0.0) {
                                // ...ale jest cos na sprzegach:
                                coupling->current = (*totalcurrent + extracurrent) * coupling->voltage /
                                                    couplervoltage; // obciążenie rozkladane stosownie do napiec
                                    if (true == coupling->is_live) {
                                        coupling->current += connectedothercoupling->current;
                                }
                        }
                    } else {
                            if (true == coupling->is_live) {
                                *totalcurrent += connectedothercoupling->current;
                        }
                    }
            }
    }

    double TMoverParameters::ShowEngineRotation(
            int VehN) { // Zwraca wartość prędkości obrotowej silnika wybranego pojazdu. Do 3 pojazdów (3×SN61).
        int b;
            switch (VehN) { // numer obrotomierza
                case 1:
                    return std::fabs(enrot);
                case 2:
                    for (b = 0; b <= 1; ++b)
                        if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                            if (Couplers[b].Connected->Power > 0.01)
                                return fabs(Couplers[b].Connected->enrot);
                    break;
                case 3: // to nie uwzględnia ewentualnego odwrócenia pojazdu w środku
                    for (b = 0; b <= 1; ++b)
                        if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                            if (Couplers[b].Connected->Power > 0.01)
                                if (TestFlag(Couplers[b].Connected->Couplers[b].CouplingFlag, coupling::control))
                                    if (Couplers[b].Connected->Couplers[b].Connected->Power > 0.01)
                                        return fabs(Couplers[b].Connected->Couplers[b].Connected->enrot);
                    break;
            };
        return 0.0;
    };

    // sprawdzanie przetwornicy
    void TMoverParameters::ConverterCheck(double const Timestep) {
            // TODO: move other converter checks here, to have it all in one place for potential device object
            if ((ConverterStart != start_t::disabled) && (ConverterOverloadRelayOffWhenMainIsOff)) {
                ConvOvldFlag |= (!Mains && Power24vIsAvailable);
        }

            switch (ConverterStart) {
                    case start_t::disabled: {
                        ConverterAllow = false;
                        // NOTE: if there's no converter in vehicle we can end the check here
                        return;
                    }
                    case start_t::automatic: {
                        ConverterAllow = Mains;
                        break;
                    }
                    case start_t::direction: {
                        ConverterAllow = (DirActive != 0);
                    }
                    default: {
                        break;
                    }
            }

            if ((ConverterAllow) && (ConverterAllowLocal) && (false == ConvOvldFlag) &&
                (false == PantPressLockActive)
                // HACK: allow carriages to operate converter without (missing) fuse prerequisite
                && ((Power > 1.0 ? Mains : GetTrainsetHighVoltage() > 0.0))) {
                    // delay timer can be optionally configured, and is set anew whenever converter goes off
                    if (ConverterStartDelayTimer <= 0.0) {
                        ConverterFlag = true;
                    } else {
                        ConverterStartDelayTimer -= Timestep;
                    }
            } else {
                ConverterFlag = false;
                ConverterStartDelayTimer = static_cast<double>(ConverterStartDelay);
            }

            if ((ConverterOverloadRelayStart == start_t::converter) &&
                (false == (ConverterAllow && ConverterAllowLocal)) && (false == TestFlag(EngDmgFlag, 4))) {
                // reset converter overload relay if the converter was switched off, unless it's damaged
                ConvOvldFlag = false;
        }
    };

    // heating system status check
    void TMoverParameters::HeatingCheck(double const Timestep) {

            // update heating devices
            // TBD, TODO: move this to a separate method?
            switch (HeatingPowerSource.SourceType) {
                    case TPowerSource::Generator: {
                            if ((HeatingPowerSource.EngineGenerator.engine_revolutions != nullptr) &&
                                (HeatingPowerSource.EngineGenerator.revolutions_max > 0)) {

                                auto &generator{HeatingPowerSource.EngineGenerator};
                                // TBD, TODO: engine-generator transmission
                                generator.revolutions = *(generator.engine_revolutions);

                                auto const absrevolutions{std::fabs(generator.revolutions)};
                                generator.voltage =
                                        (false == HeatingAllow ? 0.0 :
                                                               // TODO: add support for desired voltage selector
                                                 absrevolutions < generator.revolutions_min
                                                 ? generator.voltage_min * absrevolutions / generator.revolutions_min
                                                 :
                                                 //                    absrevolutions > generator.revolutions_max ?
                                                 //                    generator.voltage_max * absrevolutions /
                                                 //                    generator.revolutions_max :
                                                 interpolate(generator.voltage_min, generator.voltage_max,
                                                             clamp((absrevolutions - generator.revolutions_min) /
                                                                           (generator.revolutions_max -
                                                                            generator.revolutions_min),
                                                                   0.0, 1.0))) *
                                        sign(generator.revolutions);
                        }
                        break;
                    }
                    default: {
                        break;
                    }
            }

            // quick check first to avoid unnecessary calls...
            if (false == HeatingAllow) {
                Heating = false;
                return;
        }
        // ...detailed check if we're still here
        auto const heatingpowerthreshold{0.1};
        // start with blank slate
        auto voltage{0.0};
            // then try specified power source
            switch (HeatingPowerSource.SourceType) {
                    case TPowerSource::Generator: {
                        voltage = HeatingPowerSource.EngineGenerator.voltage;
                        break;
                    }
                    case TPowerSource::CurrentCollector: {
                        voltage = PantographVoltage;
                        break;
                    }
                    case TPowerSource::PowerCable: {
                            if (HeatingPowerSource.PowerType == TPowerType::ElectricPower) {
                                // TBD, TODO: limit input voltage to heating coupling type?
                                voltage = GetTrainsetHighVoltage();
                        }
                        break;
                    }
                    case TPowerSource::Main: {
                        voltage = (true == Mains ? std::max(GetTrainsetHighVoltage(), PantographVoltage) : 0.0);
                        break;
                    }
                    default: {
                        break;
                    }
            }

        Heating = (voltage > heatingpowerthreshold);

            if (Heating) {
                TotalCurrent += 1000 * HeatingPower / voltage; // heater power cost presumably specified in kilowatts
        }
    }

    // water pump status check
    void TMoverParameters::WaterPumpCheck(double const Timestep) {
        // NOTE: breaker override with start type is sm42 specific hack, replace with ability to define the presence of
        // the breaker
        WaterPump.is_active = ((true == (Power24vIsAvailable || Power110vIsAvailable)) && (true == WaterPump.breaker) &&
                               (false == WaterPump.is_disabled) &&
                               ((true == WaterPump.is_active) || (true == WaterPump.is_enabled) ||
                                (WaterPump.start_type == start_t::battery)));
    }

    // water heater status check
    void TMoverParameters::WaterHeaterCheck(double const Timestep) {

        WaterHeater.is_active =
                ((false == WaterHeater.is_damaged) && (true == (Power24vIsAvailable || Power110vIsAvailable)) &&
                 (true == WaterHeater.is_enabled) && (true == WaterHeater.breaker) &&
                 ((WaterHeater.is_active) || (WaterHeater.config.temp_min < 0) ||
                  (dizel_heat.temperatura1 < WaterHeater.config.temp_min)));

        WaterHeater.is_damaged = ((true == WaterHeater.is_damaged) ||
                                  ((true == WaterHeater.is_active) && (false == WaterPump.is_active)));

            if ((WaterHeater.config.temp_max > 0) && (dizel_heat.temperatura1 > WaterHeater.config.temp_max)) {
                WaterHeater.is_active = false;
        }
    }

    // fuel pump status update
    void TMoverParameters::FuelPumpCheck(double const Timestep) {

        FuelPump.is_active =
                ((true == (Power24vIsAvailable || Power110vIsAvailable)) && (false == FuelPump.is_disabled) &&
                 ((FuelPump.is_active) || (FuelPump.start_type == start_t::manual      ? (FuelPump.is_enabled)
                                           : FuelPump.start_type == start_t::automatic ? (dizel_startup || Mains)
                                           : FuelPump.start_type == start_t::manualwithautofallback
                                                   ? (FuelPump.is_enabled || dizel_startup || Mains)
                                                   : false))); // shouldn't ever get this far but, eh
    }

    // oil pump status update
    void TMoverParameters::OilPumpCheck(double const Timestep) {

        OilPump.is_active = ((true == (Power24vIsAvailable || Power110vIsAvailable)) && (false == Mains) &&
                             (false == OilPump.is_disabled) &&
                             ((OilPump.is_active) || (OilPump.start_type == start_t::manual      ? (OilPump.is_enabled)
                                                      : OilPump.start_type == start_t::automatic ? (dizel_startup)
                                                      : OilPump.start_type == start_t::manualwithautofallback
                                                              ? (OilPump.is_enabled || dizel_startup)
                                                              : false))); // shouldn't ever get this far but, eh

        auto const minpressure{OilPump.pressure_minimum > 0.f ? OilPump.pressure_minimum
                                                              : 0.15f}; // arbitrary fallback value

        OilPump.pressure_target =
                (enrot > 0.1
                         ? interpolate(minpressure, OilPump.pressure_maximum, static_cast<float>(EngineRPMRatio())) *
                                   OilPump.resource_amount
                 : true == OilPump.is_active
                         ? std::min(minpressure + 0.1f, OilPump.pressure_maximum)
                         : // slight pressure margin to give time to switch off the pump and start the engine
                         0.f);

            if (OilPump.pressure < OilPump.pressure_target) {
                // TODO: scale change rate from 0.01-0.05 with oil/engine temperature/idle time
                OilPump.pressure = std::min<float>(OilPump.pressure_target,
                                                   OilPump.pressure + (enrot > 5.0 ? 0.05 : 0.035) * Timestep);
        }
            if (OilPump.pressure > OilPump.pressure_target) {
                OilPump.pressure = std::max<float>(OilPump.pressure_target,
                                                   OilPump.pressure - (enrot > 5.0 ? 0.05 : 0.035) * 0.5 * Timestep);
        }
        OilPump.pressure = clamp(OilPump.pressure, 0.f, 1.5f);
    }

    void TMoverParameters::MotorBlowersCheck(double const Timestep) {
            // activation check
            for (auto &blower: MotorBlowers) {
                auto disable = blower.is_disabled;
                auto const start{(Vel >= blower.min_start_velocity && std::fabs(Im) > 0.5)};
                auto const stop{(Vel < 0.5 && std::fabs(Im) < 0.5)};
                    if (blower.min_start_velocity >= 0) {
                            if (stop) {
                                blower.stop_timer += Timestep;
                                    if (blower.stop_timer > blower.sustain_time) {
                                        disable = true;
                                }
                            } else if (start) {
                                blower.stop_timer = 0;
                            } else {
                                disable |= !blower.is_active;
                            }
                }
                blower.is_active = (
                        // TODO: bind properly power source when ld is in place
                        (blower.start_type == start_t::battery ? Power24vIsAvailable
                         : blower.start_type == start_t::converter
                                 ? Power110vIsAvailable
                                 : Mains) // power source
                                          // breaker condition disabled until it's implemented in the class data
                                          //         && ( true == blower.breaker )
                        && (false == disable) &&
                        ((true == blower.is_active) ||
                         ((blower.stop_timer == 0.f) // HACK: will be true for blower with exceeded start_velocity, and
                                                     // for one without start_velocity
                          && (blower.start_type == start_t::manual ? blower.is_enabled : true))));
            }
            // update
            for (auto &fan: MotorBlowers) {

                auto const revolutionstarget{
                        (fan.is_active ? (fan.speed > 0.f ? fan.speed * static_cast<float>(enrot) * 60 : fan.speed * -1)
                                       : 0.f)};

                    if (std::fabs(fan.revolutions - revolutionstarget) < 0.01f) {
                        fan.revolutions = revolutionstarget;
                        continue;
                }
                    if (revolutionstarget > 0.f) {
                        auto const speedincreasecap{
                                std::max(50.f, fan.speed * 0.05f * -1)}; // 5% of fixed revolution speed, or 50
                        fan.revolutions +=
                                clamp(revolutionstarget - fan.revolutions, speedincreasecap * -2, speedincreasecap) *
                                Timestep;
                    } else {
                        fan.revolutions *= std::max(0.0, 1.0 - Timestep);
                    }
            }
    }

    void TMoverParameters::PantographsCheck(double const Timestep) {

        {
            auto &valve{PantsValve};
            auto const lowvoltagepower{valve.solenoid ? (Power24vIsAvailable || Power110vIsAvailable) : true};
            auto const autostart{valve.start_type == start_t::automatic ||
                                 valve.start_type == start_t::manualwithautofallback};
            auto const manualcontrol{valve.start_type == start_t::manual ||
                                     valve.start_type == start_t::manualwithautofallback};

            PantsValve.is_active =
                    (((valve.spring ? lowvoltagepower
                                    : true)) // spring actuator needs power to maintain non-default state
                     && (((manualcontrol && lowvoltagepower) ? false == valve.is_disabled
                                                             : true)) // needs power to change state
                     && ((valve.is_active) || (autostart    ? lowvoltagepower
                                               : !autostart ? (lowvoltagepower && valve.is_enabled)
                                                            : false))); // shouldn't ever get this far but, eh
        }

        size_t pant_id = 0;
            for (auto &pantograph: Pantographs) {

                auto &valve{pantograph.valve};
                auto const lowvoltagepower{valve.solenoid ? (Power24vIsAvailable || Power110vIsAvailable) : true};
                auto const autostart{valve.start_type == start_t::automatic ||
                                     valve.start_type == start_t::manualwithautofallback};
                auto const manualcontrol{valve.start_type == start_t::manual ||
                                         valve.start_type == start_t::manualwithautofallback};

                valve.is_active = (((valve.spring ? lowvoltagepower
                                                  : true)) // spring actuator needs power to maintain non-default state
                                   && (((manualcontrol && lowvoltagepower)
                                                ? false == valve.is_disabled
                                                : true)) // needs power to change state, without it just pass through
                                   && (((manualcontrol && lowvoltagepower) ? false == PantAllDown : true)) &&
                                   ((valve.is_active) || (manualcontrol && lowvoltagepower && valve.is_enabled) ||
                                    (autostart && lowvoltagepower))); // shouldn't ever get this far but, eh

                auto const pantographexists{(EnginePowerSource.SourceType == TPowerSource::CurrentCollector) &&
                                            (EnginePowerSource.CollectorParameters.PhysicalLayout & (1 << pant_id))};

                pantograph.is_active = ((valve.is_active) && (PantsValve.is_active) && (pantographexists)
                                        //         && ( ) // TODO: add other checks
                );

                pant_id++;
            }
    }

    void TMoverParameters::LightsCheck(double const Timestep) {

        auto &light{CompartmentLights};

        light.is_active = (
                // TODO: bind properly power source when ld is in place
                (Power24vIsAvailable || Power110vIsAvailable) // power source
                && (false == light.is_disabled) &&
                ((true == light.is_active) || (light.start_type == start_t::manual ? light.is_enabled : true)));

        light.intensity = (light.is_active ? 1.0f : 0.0f)
                          // TODO: bind properly power source when ld is in place
                          * (Power110vIsAvailable  ? 1.0f
                             : Power24vIsAvailable ? 0.5f
                                                   : 0.0f) *
                          light.dimming;
    }

    double TMoverParameters::ShowCurrent(int AmpN) const { // Odczyt poboru prądu na podanym amperomierzu
            switch (EngineType) {
                case TEngineType::ElectricInductionMotor:
                        switch (AmpN) { // do asynchronicznych
                            case 1:
                                return WindingRes * Mm / Vadd;
                            case 2:
                                return eimv_pr * WindingRes;
                            default:
                                return ShowCurrentP(AmpN); // T_MoverParameters::
                        }
                    break;
                case TEngineType::DieselElectric:
                    return fabs(Im);
                    break;
                default:
                    return ShowCurrentP(AmpN); // T_MoverParameters::
            }
    };

    // *************************************************************************************************
    // queuedEU
    // *************************************************************************************************

    // *************************************************************************************************
    // Q: 20160710
    // zwiększenie nastawinika
    // *************************************************************************************************

    bool TMoverParameters::IncMainCtrl(int CtrlSpeed) {
            // basic fail conditions:
            if ((MainCtrlPosNo <= 0) || (CabActive == 0)) {
                // nie ma sterowania
                return false;
        }
            if ((TrainType == dt_ET22) && (ScndCtrlPos != 0)) {
                // w ET22 nie da się kręcić nastawnikiem przy włączonym boczniku
                return false;
        }
            if ((TrainType == dt_EZT) && (DirActive == 0)) {
                // w EZT nie da się załączyć pozycji bez ustawienia kierunku
                return false;
        }

        bool OK = false;
            if (MainCtrlPos < MainCtrlPosNo) {
                    switch (EngineType) {
                        case TEngineType::None:
                        case TEngineType::Dumb:
                        case TEngineType::DieselElectric:
                            case TEngineType::ElectricInductionMotor: {
                                    if (CtrlSpeed > 1) {
                                        OK = (IncMainCtrl(1) &&
                                              IncMainCtrl(CtrlSpeed - 1)); // a fail will propagate up the recursion
                                                                           // chain. should this be || instead?
                                    } else {
                                        ++MainCtrlPos;
                                        OK = true;
                                            if ((EIMCtrlType == 0) && (SpeedCtrlAutoTurnOffFlag & 1 == 1) &&
                                                (MainCtrlActualPos != MainCtrlPos)) {
                                                DecScndCtrl(2);
                                                SpeedCtrlUnit.IsActive = false;
                                        }
                                    }
                                break;
                            }

                            case TEngineType::ElectricSeriesMotor: {
                                    if (DirActive == 0) {
                                        return false;
                                }

                                    if (CtrlSpeed > 1) {
                                            // szybkie przejœcie na bezoporow¹
                                            if (TrainType == dt_ET40) {
                                                break; // this means ET40 won't react at all to fast acceleration
                                                       // command. should it issue just IncMainCtrl(1) instead?
                                        }
                                            while ((RList[MainCtrlPos].R > 0.0) && IncMainCtrl(1)) {
                                                // all work is done in the loop header
                                                ;
                                            }
                                        OK = false; // shouldn't this be part of the loop above?
                                                    // if (TrainType=dt_ET40)  then
                                                    // while Abs (Im)>IminHi do
                                                    //   dec(MainCtrlPos);
                                                    //  OK:=false ;
                                    } else {        // CtrlSpeed == 1
                                        ++MainCtrlPos;
                                        OK = true;
                                            if (Imax == ImaxHi) {
                                                    if (RList[MainCtrlPos].Bn > 1) {
                                                            /* NOTE: disabled, relay configuration was moved to
                                                            compute_movement_ if( true == MaxCurrentSwitch( false )) {
                                                            // wylaczanie wysokiego rozruchu
                                                            SetFlag( SoundFlag, sound::relay );
                                                            }
                                                            */
                                                            if (TrainType == dt_ET42) {
                                                                --MainCtrlPos;
                                                                OK = false;
                                                        }
                                                }
                                        }
                                        //
                                        // if (TrainType == "et40")
                                        //  if (Abs(Im) > IminHi)
                                        //   {
                                        //    MainCtrlPos--; //Blokada nastawnika po przekroczeniu minimalnego pradu
                                        //    OK = false;
                                        //   }
                                        //}
                                    }

                                    if ((TrainType == dt_ET42) && (true == DynamicBrakeFlag)) {
                                            if (MainCtrlPos > 20) {
                                                MainCtrlPos = 20;
                                                OK = false;
                                        }
                                }
                                break;
                            }

                            case TEngineType::DieselEngine: {
                                    if (CtrlSpeed > 1) {
                                            while ((MainCtrlPos < MainCtrlPosNo) && (IncMainCtrl(1))) {
                                                ;
                                            }
                                    } else {
                                        ++MainCtrlPos;
                                    }
                                CompressorAllow = (MainCtrlPowerPos() > 0);
                                OK = true;
                                break;
                            }

                            case TEngineType::WheelsDriven: {
                                OK = AddPulseForce(CtrlSpeed);
                                break;
                            }
                    } // switch EngineType of
            } else { // MainCtrlPos>=MainCtrlPosNo
                    if (true == CoupledCtrl) {
                            // wspólny wał nastawnika jazdy i bocznikowania
                            if (ScndCtrlPos < ScndCtrlPosNo) { // 3<3 -> false
                                ++ScndCtrlPos;
                                OK = true;
                            } else {
                                OK = false;
                            }
                }
            }

            if (true == OK) {
                SendCtrlToNext("MainCtrl", MainCtrlPos, CabActive); //???
                SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabActive);
        }

        // hunter-101012: poprawka
        // poprzedni warunek byl niezbyt dobry, bo przez to przy trzymaniu +
        // styczniki tkwily na tej samej pozycji (LastRelayTime byl caly czas 0 i rosl
        // po puszczeniu plusa)

            if (OK) {
                    if (DelayCtrlFlag) {
                        if ((LastRelayTime >= InitialCtrlDelay) && (MainCtrlPos == 1))
                            LastRelayTime = 0;
                } else if (LastRelayTime > CtrlDelay)
                    LastRelayTime = 0;
        }
        return OK;
    }

    // *****************************************************************************
    // Q: 20160710
    // zmniejszenie nastawnika
    // *****************************************************************************
    bool TMoverParameters::DecMainCtrl(int CtrlSpeed) {
        bool OK = false;
            // basic fail conditions:
            if ((MainCtrlPosNo <= 0) || (CabActive == 0)) {
                // nie ma sterowania
                OK = false;
            } else if (CtrlSpeed == 0) {
                return false;
            } else {
                    // TBD, TODO: replace with mainctrlpowerpos() check?
                    if (MainCtrlPos > 0) {
                            if ((TrainType != dt_ET22) ||
                                (ScndCtrlPos == 0)) // Ra: ET22 blokuje nastawnik przy boczniku
                            {
                                    if (CoupledCtrl && (ScndCtrlPos > 0)) {
                                        ScndCtrlPos--; // wspolny wal
                                        OK = true;
                                } else
                                        switch (EngineType) {
                                            case TEngineType::None:
                                            case TEngineType::Dumb:
                                            case TEngineType::DieselElectric:
                                                case TEngineType::ElectricInductionMotor: {
                                                        if (((CtrlSpeed == 1) &&
                                                             (EngineType != TEngineType::DieselElectric)) ||
                                                            ((CtrlSpeed == 1) &&
                                                             (EngineType == TEngineType::DieselElectric))) {
                                                            MainCtrlPos--;
                                                            OK = true;
                                                                if ((EIMCtrlType == 0) &&
                                                                    (SpeedCtrlAutoTurnOffFlag & 1 == 1) &&
                                                                    (MainCtrlActualPos != MainCtrlPos)) {
                                                                    DecScndCtrl(2);
                                                                    SpeedCtrlUnit.IsActive = false;
                                                            }
                                                    } else if (CtrlSpeed > 1)
                                                        OK = (DecMainCtrl(1) &&
                                                              DecMainCtrl(CtrlSpeed - 1)); // CtrlSpeed-1);
                                                    break;
                                                }

                                                case TEngineType::ElectricSeriesMotor: {
                                                        if (CtrlSpeed == 1) /*and (ScndCtrlPos=0)*/
                                                        {
                                                            MainCtrlPos--;
                                                            //                if (MainCtrlPos=0) and (ScndCtrlPos=0) and
                                                            //                (TrainType<>dt_ET40)and(TrainType<>dt_EP05)
                                                            //                then
                                                            //                 StLinFlag:=false;
                                                            //                if (MainCtrlPos=0) and
                                                            //                (TrainType<>dt_ET40) and
                                                            //                (TrainType<>dt_EP05) then
                                                            //                 MainCtrlActualPos:=0; //yBARC: co to
                                                            //                 tutaj robi? ;)
                                                            OK = true;
                                                        } else if (CtrlSpeed > 1) /*and (ScndCtrlPos=0)*/
                                                        {
                                                            OK = true;
                                                            if (RList[MainCtrlPos].R == 0) // Q: tu zrobilem = ;]
                                                                DecMainCtrl(1);
                                                            while ((RList[MainCtrlPos].R > 0) && DecMainCtrl(1))
                                                                ; // takie chamskie, potem poprawie}
                                                    }
                                                    break;
                                                }

                                                case TEngineType::DieselEngine: {
                                                        if (CtrlSpeed == 1) {
                                                            MainCtrlPos--;
                                                            OK = true;
                                                        } else if (CtrlSpeed > 1) {
                                                            while ((MainCtrlPos > 0) || (RList[MainCtrlPos].Mn > 0))
                                                                DecMainCtrl(1);
                                                            OK = true;
                                                    }
                                                    break;
                                                }
                                        } // switch EngineType
                        }
                } else if (EngineType == TEngineType::WheelsDriven)
                    OK = AddPulseForce(-CtrlSpeed);
                else
                    OK = false;

                    if (OK) {
                        /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabActive); // hmmmm...???!!!
                        /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabActive);
                }
            }
            // if OK then LastRelayTime:=0;
            // hunter-101012: poprawka
            if (OK) {
                    if (DelayCtrlFlag) {
                        if (LastRelayTime >= InitialCtrlDelay)
                            LastRelayTime = 0;
                } else if (LastRelayTime > CtrlDownDelay)
                    LastRelayTime = 0;
        }
        return OK;
    }

    bool TMoverParameters::IsMainCtrlActualNoPowerPos() const {
        // TODO: wrap controller pieces into a class for potential specializations, similar to brake subsystems
        return MainCtrlActualPos <= MainCtrlNoPowerPos();
    }

    bool TMoverParameters::IsMainCtrlNoPowerPos() const {
        // TODO: wrap controller pieces into a class for potential specializations, similar to brake subsystems
        return MainCtrlPos <= MainCtrlNoPowerPos();
    }

    bool TMoverParameters::IsMainCtrlMaxPowerPos() const {
        // TODO: wrap controller pieces into a class for potential specializations, similar to brake subsystems
        return MainCtrlPos == MainCtrlPosNo;
    }

    int TMoverParameters::MainCtrlNoPowerPos() const {

            switch (EIMCtrlType) {
                    case 1: {
                        return 3;
                    }
                    case 2: {
                        return 3;
                    }
                    case 3: {
                        return UniCtrlNoPowerPos;
                    }
                    default: {
                        return 0;
                    }
            }
    }

    int TMoverParameters::MainCtrlActualPowerPos() const {

        return MainCtrlActualPos - MainCtrlNoPowerPos();
    }

    int TMoverParameters::MainCtrlPowerPos() const {

        return MainCtrlPos - MainCtrlNoPowerPos();
    }

    // *************************************************************************************************
    // Q: 20160710
    // zwiększenie bocznika
    // *************************************************************************************************
    bool TMoverParameters::IncScndCtrl(int CtrlSpeed) {
        bool OK = false;

            if ((DynamicBrakeFlag) && (TrainType == dt_ET42) && (CabActive != 0) && (IsMainCtrlNoPowerPos()) &&
                (ScndCtrlPos == 0)) {
                OK = DynamicBrakeSwitch(false);
            } else if ((ScndCtrlPosNo > 0) && (CabActive != 0) &&
                       !((TrainType == dt_ET42) &&
                         ((Imax == ImaxHi) || ((DynamicBrakeFlag) && (MainCtrlPowerPos() > 0))))) {
                    //     if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (ScndCtrlPos<ScndCtrlPosNo) and
                    //     (not CoupledCtrl) then
                    if ((ScndCtrlPos < ScndCtrlPosNo) && (!CoupledCtrl)) {
                            // TBD, TODO: refactor this validation as part of relay check routine (currently in
                            // tractionforce() TBD, TODO: diesel electric engine utilize scndctrlactualpos like the
                            // other types?
                            if ((EngineType == TEngineType::DieselElectric) &&
                                ((IsMainCtrlNoPowerPos()) || (AutoRelayFlag) || (ShuntMode) || (false == Mains))) {
                                OK = false;
                            } else {
                                    if (CtrlSpeed == 1) {
                                        ScndCtrlPos++;
                                    } else if (CtrlSpeed > 1) {
                                        ScndCtrlPos = ScndCtrlPosNo; // takie chamskie, potem poprawie
                                }
                                OK = true;
                            }
                } else // nie mozna zmienic
                    OK = false;
                    if (OK) {
                        /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabActive); //???
                        /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabActive);
                }
        } else // nie ma sterowania
            OK = false;
        // if OK then LastRelayTime:=0;
        // hunter-101012: poprawka
        if (OK)
            if (LastRelayTime > CtrlDelay)
                LastRelayTime = 0;

            if ((OK) && (EngineType == TEngineType::ElectricInductionMotor) && (ScndCtrlPosNo == 1) &&
                (MainCtrlPos > 0)) {
                SpeedCtrlValue = Vel;
                    if ((EIMCtrlType == 0) && (SpeedCtrlAutoTurnOffFlag & 1 == 1)) {
                        MainCtrlActualPos = MainCtrlPos;
                }
                SpeedCtrlUnit.IsActive = true;
        }

            if ((OK) && (SpeedCtrl) && (ScndCtrlPos == 1) && (EngineType == TEngineType::DieselEngine)) {
                // NOTE: round() already adds 0.5, are the ones added here as well correct?
                SpeedCtrlValue = Round(Vel);
                SpeedCtrlUnit.IsActive = true;
        }

        return OK;
    }

    // *************************************************************************************************
    // Q: 20160710
    // zmniejszenie bocznika
    // *************************************************************************************************
    bool TMoverParameters::DecScndCtrl(int CtrlSpeed) {
        bool OK = false;

            if ((IsMainCtrlNoPowerPos()) && (CabActive != 0) && (TrainType == dt_ET42) && (ScndCtrlPos == 0) &&
                !(DynamicBrakeFlag) && (CtrlSpeed == 1)) {
                // Ra: AI wywołuje z CtrlSpeed=2 albo gdy ScndCtrlPos>0
                OK = DynamicBrakeSwitch(true);
            } else if ((ScndCtrlPosNo > 0) && (CabActive != 0)) {
                    if ((ScndCtrlPos > 0) && (!CoupledCtrl) &&
                        ((EngineType != TEngineType::DieselElectric) || (!AutoRelayFlag))) {
                            if (CtrlSpeed == 1) {
                                ScndCtrlPos--;
                            } else if (CtrlSpeed > 1) {
                                ScndCtrlPos = 0; // takie chamskie, potem poprawie
                        }
                        OK = true;
                } else
                    OK = false;
                    if (OK) {
                        /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabActive); //???
                        /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabActive);
                }
        } else
            OK = false;
        // if OK then LastRelayTime:=0;
        // hunter-101012: poprawka
        if (OK)
            if (LastRelayTime > CtrlDownDelay)
                LastRelayTime = 0;

            if ((OK) && (EngineType == TEngineType::ElectricInductionMotor) && (ScndCtrlPosNo == 1)) {
                SpeedCtrlValue = 0;
                SpeedCtrlUnit.IsActive = false;
                    if (SpeedCtrlUnit.ManualStateOverride) {
                        eimic = 0.0;
                }
        }

            if ((OK) && (SpeedCtrl) && (ScndCtrlPos == 0) && (EngineType == TEngineType::DieselEngine)) {
                SpeedCtrlValue = 0;
                SpeedCtrlUnit.IsActive = false;
                    if (SpeedCtrlUnit.ManualStateOverride) {
                        eimic = 0.0;
                }
        }

        return OK;
    }

    int TMoverParameters::GetVirtualScndPos() {
            if (TrainType == dt_ET42) {
                if (DynamicBrakeFlag && !ScndCtrlPos)
                    return -1;
        }
        return ScndCtrlPos;
    }

    bool TMoverParameters::IsScndCtrlNoPowerPos() const {
        // TODO: refine the check on account of potential electric series vehicles with speed control
        return ((ScndCtrlPos == 0) || (true == SpeedCtrl));
    }

    bool TMoverParameters::IsScndCtrlMaxPowerPos() const {
        // TODO: refine the check on account of potential electric series vehicles with speed control
        return ((ScndCtrlPos == ScndCtrlPosNo) || (true == SpeedCtrl));
    }

    // *************************************************************************************************
    // Q: 20160710
    // załączenie rozrządu
    // *************************************************************************************************
    bool TMoverParameters::CabActivisation(bool const Enforce) {
        bool OK = false;

        OK = Enforce || (CabActive == 0); // numer kabiny, z której jest sterowanie
            if (OK) {
                CabActive = CabOccupied; // sterowanie jest z kabiny z obsadą
                DirAbsolute = DirActive * CabActive;
                CabMaster = true;
                SecuritySystem.set_enabled(
                        true); // activate the alerter TODO: make it part of control based cab selection
                SendCtrlToNext("CabActivisation", 1, CabActive);
                SendCtrlToNext("Direction", DirAbsolute, CabActive);
                    if (InactiveCabFlag & activation::springbrakeoff) {
                        SpringBrakeActivate(false);
                }
        }
        return OK;
    }

    bool TMoverParameters::CabActivisationAuto(bool const Enforce) {
        bool OK = AutomaticCabActivation ? CabActivisation(Enforce) : false;
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160710
    // wyłączenie rozrządu
    // *************************************************************************************************
    bool TMoverParameters::CabDeactivisation(bool const Enforce) {
        bool OK = false;

        OK = Enforce || IsCabMaster(); // o ile obsada jest w kabinie ze sterowaniem
            if (OK) {
                    if (InactiveCabFlag & activation::springbrakeon) {
                        SpringBrakeActivate(true);
                }
                    if (InactiveCabFlag & activation::pantographsup) {
                        InactiveCabPantsCheck = true;
                }
                    if (InactiveCabFlag & activation::doorpermition) {
                        PermitDoors(side::right, true, range_t::consist);
                        PermitDoors(side::left, true, range_t::consist);
                }
                    if (InactiveCabFlag & activation::neutraldirection) {
                        DirActive = 0;
                        SendCtrlToNext("Direction", 0, CabActive);
                }

                CabActive = 0;
                DirAbsolute = DirActive * CabActive;
                CabMaster = false;
                DepartureSignal = false; // nie buczeć z nieaktywnej kabiny
                SecuritySystem.set_enabled(
                        false); // deactivate alerter TODO: make it part of control based cab selection

                SendCtrlToNext("CabActivisation", 0, CabOccupied); // CabActive==0!
        }
        return OK;
    }

    bool TMoverParameters::CabDeactivisationAuto(bool const Enforce) {
        bool OK = AutomaticCabActivation ? CabDeactivisation(Enforce) : false;
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160710
    // Siła napędzająca drezynę po naciśnięciu wajhy
    // *************************************************************************************************
    bool TMoverParameters::AddPulseForce(int Multipler) {
        bool APF;
            if ((EngineType == TEngineType::WheelsDriven) &&
                (EnginePowerSource.SourceType == TPowerSource::InternalSource) &&
                (EnginePowerSource.PowerType == TPowerType::BioPower)) {
                DirActive = CabActive;
                DirAbsolute = DirActive * CabActive;
                if (Vel > 0)
                    PulseForce = Min0R(1000.0 * Power / (fabs(V) + 0.1), Ftmax);
                else
                    PulseForce = Ftmax;
                if (PulseForceCount > 1000.0)
                    PulseForce = 0;
                else
                    PulseForce = PulseForce * Multipler;
                PulseForceCount = PulseForceCount + fabs(Multipler);
                APF = (PulseForce > 0);
        } else
            APF = false;
        return APF;
    }

    // *************************************************************************************************
    // yB: 20190909
    // sypanie piasku reczne
    // *************************************************************************************************
    bool TMoverParameters::SandboxManual(bool const State, range_t const Notify) {
        bool result{false};

            if (SandDoseManual != State) {
                    if (SandDoseManual == false) {
                            // switch on
                            if (Sand > 0) {
                                SandDoseManual = true;
                                result = true;
                        }
                    } else {
                        // switch off
                        SandDoseManual = false;
                        result = true;
                    }
        }

        Sandbox(SandDoseManual || SandDoseAuto, Notify);

        return result;
    }

    // *************************************************************************************************
    // yB: 20190909
    // sypanie piasku automatyczne
    // *************************************************************************************************
    bool TMoverParameters::SandboxAuto(bool const State, range_t const Notify) {
        bool result{false};
        bool NewState = State && SandDoseAutoAllow;
            if (SandDoseAuto != NewState) {
                    if (SandDoseAuto == false) {
                            // switch on
                            if (Sand > 0) {
                                SandDoseAuto = true;
                                result = true;
                        }
                    } else {
                        // switch off
                        SandDoseAuto = false;
                        result = true;
                    }
        }

        Sandbox(SandDoseManual || SandDoseAuto, Notify);

        return result;
    }


    // *************************************************************************************************
    // Q: 20160713
    // sypanie piasku
    // *************************************************************************************************
    bool TMoverParameters::Sandbox(bool const State, range_t const Notify) {
        bool result{false};

            if (SandDose != State) {
                    if (SandDose == false) {
                            // switch on
                            if ((Sand > 0) && (DirActive != 0)) {
                                SandDose = true;
                                result = true;
                        }
                    } else {
                        // switch off
                        SandDose = false;
                        result = true;
                    }
        }

            if (Notify != range_t::local) {
                // if requested pass the command on
                auto const couplingtype =
                        (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control);

                    if (State == true) {
                        // switch on
                        SendCtrlToNext("Sandbox", 1, CabActive, couplingtype);
                    } else {
                        // switch off
                        SendCtrlToNext("Sandbox", 0, CabActive, couplingtype);
                    }
        }

        return result;
    }

    // *************************************************************************************************
    // yB: 20190909
    // włączenie / wyłączenie automatycznej piasecznicy
    // *************************************************************************************************
    bool TMoverParameters::SandboxAutoAllow(bool State) {
        // SendCtrlToNext("SandboxAutoAllow", int(State), CabActive, ctrain_controll);

            if (SandDoseAutoAllow != State) {
                SandDoseAutoAllow = State;
                return true;
        } else
            return false;
    }

    // *****************************************************************************
    // Q: 20160710
    // zbicie czuwaka / SHP
    // *****************************************************************************
    // hunter-091012: rozbicie alarmow, dodanie testu czuwaka
    void TMoverParameters::SecuritySystemReset(void) // zbijanie czuwaka/SHP
    {
        // reset all, used by AI
        SecuritySystem.acknowledge_press();
        SecuritySystem.acknowledge_release();
        SecuritySystem.cabsignal_reset();
    }

    // *************************************************************************************************
    // Q: 20160711
    // sprawdzanie stanu CA/SHP
    // *************************************************************************************************
    void TMoverParameters::SecuritySystemCheck(double dt) {
        bool isPower = Power24vIsAvailable || Power110vIsAvailable;
        SecuritySystem.update(dt, Vel, isPower, CabActive);

            if (!Battery || !Radio) { // wyłączenie baterii deaktywuje sprzęt
                RadiostopSwitch(false);
        }
    }

    // *************************************************************************************************
    // Q: 20160710
    // włączenie / wyłączenie baterii
    // *************************************************************************************************
    bool TMoverParameters::BatterySwitch(bool State, range_t const Notify) {
        auto const initialstate{Battery};

            // Ra: ukrotnienie załączania baterii jest jakąś fikcją...
            if (BatteryStart == start_t::manual) {
                Battery = State;
        }

            // switching batteries does not require activation
            if (Notify != range_t::local) {
                SendCtrlToNext("BatterySwitch", (State ? 1 : 0), 1,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
                SendCtrlToNext("BatterySwitch", (State ? 1 : 0), -1,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (Battery != initialstate);
    }

    // *************************************************************************************************
    // Q: 20160710
    // włączenie / wyłączenie hamulca elektro-pneumatycznego
    // *************************************************************************************************
    bool TMoverParameters::EpFuseSwitch(bool State) {
            if (EpFuse != State) {
                EpFuse = State;
                return true;
        } else
            return false;
        // if (EpFuse == true) SendCtrlToNext("EpFuseSwitch", 1, CabActive)
        //  else SendCtrlToNext("EpFuseSwitch", 0, CabActive);
    }

    // *************************************************************************************************
    // yB: 20190906
    // włączenie / wyłączenie hamulca sprezynowego
    // *************************************************************************************************
    bool TMoverParameters::SpringBrakeActivate(bool State) {
            if (Power24vIsAvailable || Power110vIsAvailable) {
                SendCtrlToNext("SpringBrakeActivate", int(State), CabActive, SpringBrake.MultiTractionCoupler);

                    if (SpringBrake.Activate != State) {
                        SpringBrake.Activate = State;
                        return true;
                }
        }
        return false;
    }

    // *************************************************************************************************
    // yB: 20190906
    // włączenie / wyłączenie odciecia hamulca sprezynowego
    // *************************************************************************************************
    bool TMoverParameters::SpringBrakeShutOff(bool State) {
            if (SpringBrake.ShuttOff != State) {
                SpringBrake.ShuttOff = State;
                return true;
        } else
            return false;
    }

    // *************************************************************************************************
    // yB: 20190906
    // wyluzowanie hamulca sprezynowego
    // *************************************************************************************************
    bool TMoverParameters::SpringBrakeRelease() {
            if (SpringBrake.IsReady && SpringBrake.Cylinder->P() < SpringBrake.MinForcePressure) {
                SpringBrake.IsReady = false;
                return true;
        } else
            return false;
    }

    // *************************************************************************************************
    // Q: 20160710
    // kierunek do tyłu
    // *************************************************************************************************
    bool TMoverParameters::DirectionBackward(void) {
            if (false == EIMDirectionChangeAllow()) {
                return false;
        }

        if ((DirActive == 1) && (MainCtrlPos == 0) && (TrainType == dt_EZT) &&
            (EngineType != TEngineType::ElectricInductionMotor))
                if (MinCurrentSwitch(false)) {
                    return true;
            }
            if ((MainCtrlPosNo > 0) && (DirActive > -1) &&
                ((CabActive != 0) || ((InactiveCabFlag & activation::neutraldirection) == 0))) {
                if (EngineType == TEngineType::WheelsDriven)
                    --CabActive;
                //    else
                --DirActive;
                DirAbsolute = DirActive * CabActive;
                SendCtrlToNext("Direction", DirActive, CabActive);
                return true;
        }

        return false;
    }

    bool TMoverParameters::EIMDirectionChangeAllow(void) const {
        bool OK = false;
        /*
        // NOTE: disabled while eimic variables aren't immediately synced with master controller changes inside ai
        module OK = (EngineType != TEngineType::ElectricInductionMotor || ((eimic <= 0) && (eimic_real <= 0) && (Vel <
        0.1)));
        */
        OK = (MainCtrlPos <= MainCtrlMaxDirChangePos);
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160710
    // załączenie przycisku przeciwpoślizgowego
    // *************************************************************************************************
    bool TMoverParameters::AntiSlippingButton(void) {
        // NOTE: disabled the sandbox part, it's already controlled by another part of the AI routine
        return (AntiSlippingBrake() /*|| Sandbox(true)*/);
    }

    // water pump breaker state toggle
    bool TMoverParameters::WaterPumpBreakerSwitch(bool State, range_t const Notify) {
        /*
           if( FuelPump.start_type == start::automatic ) {
        // automatic fuel pump ignores 'manual' state commands
        return false;
        }
        */
        bool const initialstate{WaterPump.breaker};

        WaterPump.breaker = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterPumpBreakerSwitch", (WaterPump.breaker ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterPump.breaker != initialstate);
    }

    // water pump state toggle
    bool TMoverParameters::WaterPumpSwitch(bool State, range_t const Notify) {

            if (WaterPump.start_type == start_t::battery) {
                // automatic fuel pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{WaterPump.is_enabled};

        WaterPump.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterPumpSwitch", (WaterPump.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterPump.is_enabled != initialstate);
    }

    // water pump state toggle
    bool TMoverParameters::WaterPumpSwitchOff(bool State, range_t const Notify) {

            if (WaterPump.start_type == start_t::battery) {
                // automatic fuel pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{WaterPump.is_disabled};

        WaterPump.is_disabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterPumpSwitchOff", (WaterPump.is_disabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterPump.is_disabled != initialstate);
    }

    // water heater breaker state toggle
    bool TMoverParameters::WaterHeaterBreakerSwitch(bool State, range_t const Notify) {
        /*
           if( FuelPump.start_type == start::automatic ) {
        // automatic fuel pump ignores 'manual' state commands
        return false;
        }
        */
        bool const initialstate{WaterHeater.breaker};

        WaterHeater.breaker = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterHeaterBreakerSwitch", (WaterHeater.breaker ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterHeater.breaker != initialstate);
    }

    // water heater state toggle
    bool TMoverParameters::WaterHeaterSwitch(bool State, range_t const Notify) {
        /*
           if( FuelPump.start_type == start::automatic ) {
        // automatic fuel pump ignores 'manual' state commands
        return false;
        }
        */
        bool const initialstate{WaterHeater.is_enabled};

        WaterHeater.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterHeaterSwitch", (WaterHeater.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterHeater.is_enabled != initialstate);
    }

    // water circuits link state toggle
    bool TMoverParameters::WaterCircuitsLinkSwitch(bool State, range_t const Notify) {

            if (false == dizel_heat.auxiliary_water_circuit) {
                // can't link the circuits if the vehicle only has one
                return false;
        }

        bool const initialstate{WaterCircuitsLink};

        WaterCircuitsLink = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("WaterCircuitsLinkSwitch", (WaterCircuitsLink ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (WaterCircuitsLink != initialstate);
    }

    // fuel pump state toggle
    bool TMoverParameters::FuelPumpSwitch(bool State, range_t const Notify) {

            if (FuelPump.start_type == start_t::automatic) {
                // automatic fuel pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{FuelPump.is_enabled};

        FuelPump.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("FuelPumpSwitch", (FuelPump.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (FuelPump.is_enabled != initialstate);
    }

    bool TMoverParameters::FuelPumpSwitchOff(bool State, range_t const Notify) {

            if (FuelPump.start_type == start_t::automatic) {
                // automatic fuel pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{FuelPump.is_disabled};

        FuelPump.is_disabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("FuelPumpSwitchOff", (FuelPump.is_disabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (FuelPump.is_disabled != initialstate);
    }

    // oil pump state toggle
    bool TMoverParameters::OilPumpSwitch(bool State, range_t const Notify) {

            if (OilPump.start_type == start_t::automatic) {
                // automatic pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{OilPump.is_enabled};

        OilPump.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("OilPumpSwitch", (OilPump.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (OilPump.is_enabled != initialstate);
    }

    bool TMoverParameters::OilPumpSwitchOff(bool State, range_t const Notify) {

            if (OilPump.start_type == start_t::automatic) {
                // automatic pump ignores 'manual' state commands
                return false;
        }

        bool const initialstate{OilPump.is_disabled};

        OilPump.is_disabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("OilPumpSwitchOff", (OilPump.is_disabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (OilPump.is_disabled != initialstate);
    }

    bool TMoverParameters::MotorBlowersSwitch(bool State, end const Side, range_t const Notify) {

        auto &fan{MotorBlowers[Side]};

            if ((fan.start_type != start_t::manual) && (fan.start_type != start_t::manualwithautofallback)) {
                // automatic device ignores 'manual' state commands
                return false;
        }

        bool const initialstate{fan.is_enabled};

        fan.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext((Side == end::front ? "MotorBlowersFrontSwitch" : "MotorBlowersRearSwitch"),
                               (fan.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (fan.is_enabled != initialstate);
    }

    bool TMoverParameters::MotorBlowersSwitchOff(bool State, end const Side, range_t const Notify) {

        auto &fan{MotorBlowers[Side]};

            if ((fan.start_type != start_t::manual) && (fan.start_type != start_t::manualwithautofallback)) {
                // automatic device ignores 'manual' state commands
                return false;
        }

        bool const initialstate{fan.is_disabled};

        fan.is_disabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext((Side == end::front ? "MotorBlowersFrontSwitchOff" : "MotorBlowersRearSwitchOff"),
                               (fan.is_disabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (fan.is_disabled != initialstate);
    }

    bool TMoverParameters::CompartmentLightsSwitch(bool State, range_t const Notify) {

            if (CompartmentLights.start_type == start_t::automatic) {
                // automatic lights ignore 'manual' state commands
                return false;
        }

        bool const initialstate{CompartmentLights.is_enabled};

        CompartmentLights.is_enabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("CompartmentLightsSwitch", (CompartmentLights.is_enabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (CompartmentLights.is_enabled != initialstate);
    }

    // water pump state toggle
    bool TMoverParameters::CompartmentLightsSwitchOff(bool State, range_t const Notify) {

            if (CompartmentLights.start_type == start_t::automatic) {
                // automatic lights ignore 'manual' state commands
                return false;
        }

        bool const initialstate{CompartmentLights.is_disabled};

        CompartmentLights.is_disabled = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("CompartmentLightsSwitchOff", (CompartmentLights.is_disabled ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (CompartmentLights.is_disabled != initialstate);
    }

    // *************************************************************************************************
    // Q: 20160713
    // włączenie / wyłączenie obwodu głownego
    // *************************************************************************************************
    bool TMoverParameters::MainSwitch(bool const State, range_t const Notify) {

        bool const initialstate{Mains || dizel_startup};

        MainSwitch_(State);

            if (Notify != range_t::local) {
                // pass the command to other vehicles
                // TBD: pass the requested state, or the actual state?
                SendCtrlToNext("MainSwitch", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return ((Mains || dizel_startup) != initialstate);
    }

    void TMoverParameters::MainSwitch_(bool const State) {

            if ((Mains == State) || (MainCtrlPosNo == 0)) {
                // nothing to do
                return;
        }

        bool const initialstate{Mains};

            if ((false == State) || (true == MainSwitchCheck())) {

                    if (true == State) {
                            // switch on
                            if ((EngineType == TEngineType::DieselEngine) ||
                                (EngineType == TEngineType::DieselElectric)) {
                                // launch diesel engine startup procedure
                                dizel_startup = true;
                            } else {
                                // additional check, as vehicles without pantographs won't fail relay checks earlier
                                Mains = true;
                            }
                    } else {
                        Mains = false;
                        // potentially knock out the pumps if their switch doesn't force them on
                        WaterPump.is_active &= WaterPump.is_enabled;
                        FuelPump.is_active &= FuelPump.is_enabled;
                    }

                    if (Mains != initialstate) {
                        LastSwitchingTime = 0;
                }
        }
    }

    bool TMoverParameters::MainSwitchCheck() const {

        // prevent the switch from working if there's no power
        // TODO: consider whether it makes sense for diesel engines and such
        bool powerisavailable{true};

            switch (EngineType) {
                case TEngineType::DieselElectric:
                case TEngineType::DieselEngine:
                    case TEngineType::Dumb: {
                        powerisavailable = Power24vIsAvailable;
                        break;
                    }
                case TEngineType::ElectricSeriesMotor:
                    case TEngineType::ElectricInductionMotor: {
                        // TODO: check whether we can simplify this check and skip the outer EngineType switch
                        powerisavailable = (EnginePowerSourceVoltage() > 0.5 * EnginePowerSource.MaxVoltage);
                        break;
                    }
                    default: {
                        break;
                    }
            }

        return ((powerisavailable) && ((ScndCtrlPos == 0) || (EngineType == TEngineType::ElectricInductionMotor)) &&
                (MainsInitTimeCountdown <= 0.0) &&
                ((ConvOvldFlag == false) || (ConverterOverloadRelayOffWhenMainIsOff)) && (true == GroundRelay) &&
                (true == NoVoltRelay) && (true == OvervoltageRelay) && (LastSwitchingTime > CtrlDelay) &&
                (HasCamshaft ? IsMainCtrlActualNoPowerPos()
                             : (LineBreakerClosesOnlyAtNoPowerPos ? IsMainCtrlNoPowerPos() : true)) &&
                (false == TestFlag(DamageFlag, dtrain_out)) && (false == TestFlag(EngDmgFlag, 1)));
    }

    // *************************************************************************************************
    // Q: 20160713
    // włączenie / wyłączenie przetwornicy
    // *************************************************************************************************
    bool TMoverParameters::ConverterSwitch(bool State, range_t const Notify) {

        auto const initialstate{ConverterAllow};

            if (ConverterStart == start_t::manual) {
                ConverterAllow = State;
        }

            if (Notify != range_t::local) {
                SendCtrlToNext("ConverterSwitch", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (ConverterAllow != initialstate);
    }

    // *************************************************************************************************
    // Q: 20160713
    // włączenie / wyłączenie sprężarki
    // *************************************************************************************************
    bool TMoverParameters::CompressorSwitch(bool State, range_t const Notify) {

            if (CompressorStart != start_t::manual) {
                // only pay attention if the compressor can be controlled manually
                return false;
        }

        auto const initialstate{CompressorAllow};

            if ((VeselVolume > 0.0) && (CompressorSpeed > 0.0)) {
                CompressorAllow = State;
        }

            if (Notify != range_t::local) {
                SendCtrlToNext("CompressorSwitch", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (CompressorAllow != initialstate);
    }

    bool TMoverParameters::ChangeCompressorPreset(int const State, range_t const Notify) {

        auto const initialstate{CompressorListPos};

        CompressorListPos = clamp(State, 0, CompressorListPosNo);

            if (Notify != range_t::local) {
                SendCtrlToNext("CompressorPreset", State, CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (CompressorListPos != initialstate);
    }

    bool TMoverParameters::HeatingSwitch(bool const State, range_t const Notify) {

        bool const initialstate{HeatingAllow};

        HeatingSwitch_(State);

            if (Notify != range_t::local) {
                // pass the command to other vehicles
                // TBD: pass the requested state, or the actual state?
                SendCtrlToNext("HeatingSwitch", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (HeatingAllow != initialstate);
    }

    void TMoverParameters::HeatingSwitch_(bool const State) {

        // TBD, TODO: activation dependencies?
        HeatingAllow = State;
    }

    // returns voltage of defined main engine power source
    double TMoverParameters::EnginePowerSourceVoltage() const {

        return (EnginePowerSource.SourceType == TPowerSource::CurrentCollector
                        ? std::max(GetTrainsetHighVoltage(), PantographVoltage)
                : EnginePowerSource.SourceType == TPowerSource::Accumulator ? Power24vVoltage
                                                                            : 0.0);
    }

    // *************************************************************************************************
    // Q: 20160711
    // zwiększenie nastawy hamulca
    // *************************************************************************************************
    bool TMoverParameters::IncBrakeLevelOld(void) {
        bool IBLO = false;

            if (BrakeCtrlPosNo > 0) {
                    if (BrakeCtrlPos < BrakeCtrlPosNo) {
                        ++BrakeCtrlPos;
                        // youBy: EP po nowemu
                        IBLO = true;
                        if ((BrakePressureActual.PipePressureVal < 0) &&
                            (BrakePressureTable[BrakeCtrlPos - 1].PipePressureVal > 0))
                            LimPipePress = PipePress;
                    } else {
                        IBLO = false;
                    }
        }

        return IBLO;
    }

    // *****************************************************************************
    // Q: 20160711
    // zmniejszenie nastawy hamulca
    // *****************************************************************************
    bool TMoverParameters::DecBrakeLevelOld(void) {
        bool DBLO = false;

            if (BrakeCtrlPosNo > 0) {
                    if (BrakeCtrlPos > ((BrakeHandle == TBrakeHandle::FV4a) ? -2 : -1)) {
                        --BrakeCtrlPos;
                        // youBy: EP po nowemu
                        DBLO = true;
                        //            if ((BrakePressureTable[BrakeCtrlPos].PipePressureVal<0.0) &&
                        //                (BrakePressureTable[BrakeCtrlPos+1].PipePressureVal > 0))
                        //                LimPipePress=PipePress;
                } else
                    DBLO = false;
        }

        return DBLO;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zwiększenie nastawy hamulca pomocnicznego
    // *************************************************************************************************
    bool TMoverParameters::IncLocalBrakeLevel(float const CtrlSpeed) {
        bool IBL;
            if ((LocalBrakePosA < 1.0) /*and (BrakeCtrlPos<1)*/) {
                LocalBrakePosA = std::min(1.0, LocalBrakePosA + CtrlSpeed / LocalBrakePosNo);
                IBL = true;
        } else
            IBL = false;

        return IBL;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zmniejszenie nastawy hamulca pomocniczego
    // *************************************************************************************************
    bool TMoverParameters::DecLocalBrakeLevel(float const CtrlSpeed) {
        bool DBL;
            if (LocalBrakePosA > 0) {
                LocalBrakePosA = std::max(0.0, LocalBrakePosA - CtrlSpeed / LocalBrakePosNo);
                DBL = true;
        } else
            DBL = false;

        return DBL;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zwiększenie nastawy hamulca ręcznego
    // *************************************************************************************************
    bool TMoverParameters::IncManualBrakeLevel(int CtrlSpeed) {
        bool IMBL;
            if (ManualBrakePos < ManualBrakePosNo) /*and (BrakeCtrlPos<1)*/
            {
                    while ((ManualBrakePos < ManualBrakePosNo) && (CtrlSpeed > 0)) {
                        ManualBrakePos++;
                        CtrlSpeed--;
                    }
                IMBL = true;
        } else
            IMBL = false;

        return IMBL;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zmniejszenie nastawy hamulca ręcznego
    // *************************************************************************************************
    bool TMoverParameters::DecManualBrakeLevel(int CtrlSpeed) {
        bool DMBL;
            if (ManualBrakePos > 0) {
                    while ((CtrlSpeed > 0) && (ManualBrakePos > 0)) {
                        ManualBrakePos--;
                        CtrlSpeed--;
                    }
                DMBL = true;
        } else
            DMBL = false;
        return DMBL;
    }

    // *************************************************************************************************
    // Q: 20160713
    // reczne przelaczanie hamulca elektrodynamicznego
    // *************************************************************************************************
    bool TMoverParameters::DynamicBrakeSwitch(bool Switch) {
        bool DBS;

            if ((DynamicBrakeType == dbrake_switch) && (IsMainCtrlNoPowerPos())) {
                DynamicBrakeFlag = Switch;
                DBS = true;
                for (int b = 0; b < 2; b++)
                    //  with Couplers[b] do
                    if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                        Couplers[b].Connected->DynamicBrakeFlag = Switch;
                // end;
                // if (DynamicBrakeType=dbrake_passive) and (TrainType=dt_ET42) then
                // begin
                // DynamicBrakeFlag:=false;
                // DynamicBrakeSwitch:=false;
        } else
            DBS = false;

        return DBS;
    }

    // *************************************************************************************************
    // Q: 20160711
    // włączenie / wyłączenie hamowania awaryjnego
    // *************************************************************************************************
    bool TMoverParameters::RadiostopSwitch(bool Switch) {
        bool EBS;
            if ((BrakeSystem != TBrakeSystem::Individual) && (BrakeCtrlPosNo > 0)) {

                    if ((true == Switch) && (false == RadioStopFlag)) {
                        RadioStopFlag = Switch;
                        EBS = true;
                    } else {
                            if ((Switch == false) && (std::fabs(V) < 0.1)) {
                                // odblokowanie hamulca bezpieczenistwa tylko po zatrzymaniu
                                RadioStopFlag = Switch;
                                EBS = true;
                            } else {
                                EBS = false;
                            }
                    }
            } else {
                // nie ma hamulca bezpieczenstwa gdy nie ma hamulca zesp.
                EBS = false;
            }

        return EBS;
    }

    bool TMoverParameters::AlarmChainSwitch(bool const State) {

            if (AlarmChainFlag != State) {
                // simple routine for the time being
                AlarmChainFlag = State;
                return true;
        }
        return false;
    }

    // *************************************************************************************************
    // Q: 20160710
    // hamowanie przeciwpoślizgowe
    // *************************************************************************************************
    bool TMoverParameters::AntiSlippingBrake(void) {
        bool ASB = false; // Ra: przeniesione z końca
            if (ASBType == 1) {
                ASB = true; // SPKS!!
                Hamulec->ASB(1);
                BrakeSlippingTimer = 0;
        }
        return ASB;
    }

    // *************************************************************************************************
    // Q: 20160711
    // włączenie / wyłączenie odluźniacza
    // *************************************************************************************************
    bool TMoverParameters::BrakeReleaser(int state) {
        bool OK = true; // false tylko jeśli nie uda się wysłać, GF 20161124
            if (state != 0) {
                    // additional limitations imposed by pressure switch
                    if ((false == ControlPressureSwitch) || (false == ReleaserEnabledOnlyAtNoPowerPos) ||
                        (true == IsMainCtrlNoPowerPos())) {
                        Hamulec->Releaser(state);
                }
            } else {
                Hamulec->Releaser(state);
            }
        if (CabActive != 0) // rekurencyjne wysłanie do następnego
            OK = SendCtrlToNext("BrakeReleaser", state, CabActive);
        return OK;
    }

    // *************************************************************************************************
    // yB: 20160711
    // włączenie / wyłączenie uniwersalnego przycisku hamulcowego
    // *************************************************************************************************
    bool TMoverParameters::UniversalBrakeButton(int button, int state) {
        bool OK = true; // false tylko jeśli nie uda się wysłać, GF 20161124
        UniversalBrakeButtonActive[button] = state > 0;
        int flag = 0;
            if (Power24vIsAvailable || Power110vIsAvailable) {
                    for (int i = 0; i < 3; i++) {
                        flag = flag | (UniversalBrakeButtonActive[i] ? UniversalBrakeButtonFlag[i] : 0);
                    }
        }

        Hamulec->SetUniversalFlag(flag);
        Handle->SetUniversalFlag(flag);
        LocHandle->SetUniversalFlag(flag);
        UnlockPipe = (flag & TUniversalBrake::ub_UnlockPipe) > 0;

            // if the releaser can be activated by switch
            if (TestFlag(UniversalBrakeButtonFlag[0] & UniversalBrakeButtonFlag[1] & UniversalBrakeButtonFlag[2],
                         TUniversalBrake::ub_Release)) {
                BrakeReleaser(TestFlag(flag, TUniversalBrake::ub_Release) ? 1 : 0);
        }
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160711
    // włączenie / wyłączenie hamulca elektro-pneumatycznego
    // *************************************************************************************************
    bool TMoverParameters::SwitchEPBrake(int state) {
        bool OK;

        OK = false;
            if ((BrakeHandle == TBrakeHandle::St113) && (CabOccupied != 0)) {
                if (state > 0)
                    EpForce = Handle->GetEP(); // TODO: przetlumaczyc
                else
                    EpForce = 0;
                Hamulec->SetEPS(EpForce);
                SendCtrlToNext("Brake", EpForce, CabActive);
        }
        //  OK:=SetFlag(BrakeStatus,((2*State-1)*b_epused));
        //  SendCtrlToNext('Brake',(state*(2*BrakeCtrlPos-1)),CabActive);
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zwiększenie ciśnienia hamowania
    // *************************************************************************************************
    bool TMoverParameters::IncBrakePress(double &brake, double PressLimit, double dp) {
        bool IBP;
            //  if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and
            //  ((BrakePress>2.0) or (PipePress<3.7{(LowPipePress+0.5)})) then
            if ((DynamicBrakeType != dbrake_switch) && (DynamicBrakeType != dbrake_none) && (BrakePress > 2.0) &&
                (TrainType != dt_EZT)) // yB radzi nie sprawdzać ciśnienia w przewodzie
                                       // hunter-301211: dla EN57 silnikow nie odlaczamy
            {
                DynamicBrakeFlag = true; // uruchamianie hamulca ED albo odlaczanie silnikow
                if ((DynamicBrakeType == dbrake_automatic) && (fabs(Im) > 60)) // nie napelniaj wiecej, jak na EP09
                    dp = 0.0;
        }
            if (brake + dp < PressLimit) {
                brake = brake + dp;
                IBP = true;
            } else {
                IBP = false;
                brake = PressLimit;
            }
        return IBP;
    }

    // *************************************************************************************************
    // Q: 20160711
    // zmniejszenie ciśnienia hamowania
    // *************************************************************************************************
    bool TMoverParameters::DecBrakePress(double &brake, double PressLimit, double dp) {
        bool DBP;

            if (brake - dp > PressLimit) {
                brake = brake - dp;
                DBP = true;
            } else {
                DBP = false;
                brake = PressLimit;
            }
        //  if ((DynamicBrakeType != dbrake_switch) && ((BrakePress < 0.1) && (PipePress > 0.45
        //  /*(LowPipePress+0.06)*/ )))
        if ((DynamicBrakeType != dbrake_switch) && (BrakePress < 0.1)) // yB radzi nie sprawdzać ciśnienia w przewodzie
            DynamicBrakeFlag = false; // wylaczanie hamulca ED i/albo zalaczanie silnikow

        return DBP;
    }

    // *************************************************************************************************
    // Q: 20160711
    // przełączenie nastawy hamulca O/P/T
    // *************************************************************************************************
    bool TMoverParameters::BrakeDelaySwitch(int BDS) {
        bool rBDS;
            if (Hamulec->SetBDF(BDS)) {
                BrakeDelayFlag = BDS;
                rBDS = true;
                Hamulec->SetBrakeStatus(Hamulec->GetBrakeStatus() & ~64);
                // kopowanie nastawy hamulca do kolejnego czlonu - do przemyślenia
                if (CabActive != 0)
                    SendCtrlToNext("BrakeDelay", BrakeDelayFlag, CabActive);
        } else
            rBDS = false;

        return rBDS;
    }

    // *************************************************************************************************
    // Q: 20160712
    // zwiększenie przełożenia hamulca
    // *************************************************************************************************
    bool TMoverParameters::IncBrakeMult(void) {
        bool IBM;

            if ((LoadFlag > 0) && (MBPM < 2) && (LoadFlag < 3)) {
                if ((MaxBrakePress[2] > 0) && (LoadFlag == 1))
                    LoadFlag = 2;
                else
                    LoadFlag = 3;
                IBM = true;
                if (BrakeCylMult[2] > 0)
                    BrakeCylMult[0] = BrakeCylMult[2];
        } else
            IBM = false;

        return IBM;
    }

    // *************************************************************************************************
    // Q: 20160712
    // zmniejszenie przełożenia hamulca
    // *************************************************************************************************
    bool TMoverParameters::DecBrakeMult(void) {
        bool DBM;
            if ((LoadFlag > 1) && (MBPM < 2)) {
                if ((MaxBrakePress[2] > 0) && (LoadFlag == 3))
                    LoadFlag = 2;
                else
                    LoadFlag = 1;
                DBM = true;
                if (BrakeCylMult[1] > 0)
                    BrakeCylMult[0] = BrakeCylMult[1];
        } else
            DBM = false;

        return DBM;
    }

    // *************************************************************************************************
    // Q: 20160712
    // zaktualizowanie ciśnienia w hamulcach
    // *************************************************************************************************
    void TMoverParameters::UpdateBrakePressure(double dt) {
        // const double LBDelay = 5.0; // stala czasowa hamulca
        // double Rate, Speed, dp, sm;

        dpLocalValve = 0;
        dpBrake = 0;

        Hamulec->ForceLeak(dt * AirLeakRate * 0.25); // fake air leaks from brake system reservoirs

        BrakePress = Hamulec->GetBCP();
        //  BrakePress:=(Hamulec as TEst4).ImplsRes.pa;
        Volume = Hamulec->GetBRP();
    }

    // *************************************************************************************************
    // Q: 20160712
    // Obliczanie pracy sprężarki
    // *************************************************************************************************
    void TMoverParameters::CompressorCheck(double dt) {

            if (CompressorSpeed == 0.0) {
                CompressorAllow = false;
                return;
        }

            if (CabDependentCompressor) {
                    if (CabActive > 0) {
                        MinCompressor = MinCompressor_cabA;
                        MaxCompressor = MaxCompressor_cabA;
                }
                    if (CabActive < 0) {
                        MinCompressor = MinCompressor_cabB;
                        MaxCompressor = MaxCompressor_cabB;
                }
        }

        // EmergencyValve
        EmergencyValveOpen = (Compressor > (EmergencyValveOpen ? EmergencyValveOff : EmergencyValveOn));
            if (EmergencyValveOpen) {
                float dV = PF(0, Compressor, EmergencyValveArea) * dt;
                CompressedVolume -= dV;
        }

        CompressedVolume = std::max(0.0, CompressedVolume - dt * AirLeakRate * 0.1); // nieszczelności: 0.001=1l/s

        Compressor = CompressedVolume / VeselVolume;

        // assorted operational logic
        auto const MaxCompressorF{CompressorList[TCompressorList::cl_MaxFactor][CompressorListPos] * MaxCompressor};
        auto const MinCompressorF{CompressorList[TCompressorList::cl_MinFactor][CompressorListPos] * MinCompressor};
        auto const CompressorSpeedF{CompressorList[TCompressorList::cl_SpeedFactor][CompressorListPos] *
                                    CompressorSpeed};
        auto const AllowFactor{CompressorList[TCompressorList::cl_Allow][CompressorListPos]};
            // checking the impact on the compressor allowance
            if (AllowFactor > 0.5) {
                CompressorAllow = (AllowFactor > 1.5);
        }

            switch (CompressorPower) {
                    case 2: {
                        CompressorAllow = ConverterAllow;
                        break;
                    }
                    case 3: {
                        // HACK: make sure compressor coupled with diesel engine is always ready for work
                        CompressorStart = start_t::automatic;
                        break;
                    }
                    default: {
                        break;
                    }
            }

        auto const compressorpower{(CompressorPower == 0   ? Mains
                                    : CompressorPower == 3 ? Mains
                                                           : Power110vIsAvailable)};
        // TBD: split CompressorAllow into separate enable/disable flags, inherit compressor from basic_device
        auto const compressorenable{(CompressorAllowLocal) &&
                                    ((CompressorStart == start_t::automatic) || (CompressorAllow))};
        auto const compressordisable{false == compressorenable};

        auto const pressureistoolow{Compressor < MinCompressorF};
        auto const pressureistoohigh{Compressor > MaxCompressorF};

        // TBD, TODO: break the lock with no low voltage power?
        auto const governorlockispresent{MaxCompressorF - MinCompressorF > 0.0001};
        CompressorGovernorLock =
                (governorlockispresent) &&
                (false == pressureistoolow) // unlock if pressure drops below minimal threshold
                && (pressureistoohigh ||
                    CompressorGovernorLock); // lock if pressure goes above maximum threshold
                                             // for these multi-unit engines compressors turn off whenever any of them
                                             // was affected by the governor NOTE: this is crude implementation, limited
                                             // only to adjacent vehicles
                                             // TODO: re-implement when a more elegant/flexible system is in place
        auto const coupledgovernorlock{((Couplers[end::rear].Connected != nullptr) &&
                                        (true == TestFlag(Couplers[end::rear].CouplingFlag, coupling::permanent)) &&
                                        (Couplers[end::rear].Connected->CompressorGovernorLock)) ||
                                       ((Couplers[end::front].Connected != nullptr) &&
                                        (true == TestFlag(Couplers[end::front].CouplingFlag, coupling::permanent)) &&
                                        (Couplers[end::front].Connected->CompressorGovernorLock))};
        auto const governorlock{CompressorGovernorLock || coupledgovernorlock};

        auto const compressorflag{CompressorFlag};
        CompressorFlag = (compressorpower) && (false == compressordisable) &&
                         ((false == governorlock) || (CompressorPower == 3)) &&
                         ((CompressorFlag) || ((compressorenable) && (LastSwitchingTime > CtrlDelay)));

            if ((CompressorFlag) && (CompressorFlag != compressorflag)) {
                // jeśli została załączona to trzeba ograniczyć ponowne włączenie
                LastSwitchingTime = 0;
        }

            if (false == CompressorFlag) {
                return;
        }

            // working compressor adds air to the air reservoir
            switch (CompressorPower) {
                    case 3: {
                        // the compressor is coupled with the diesel engine, engine revolutions affect the output
                        CompressedVolume +=
                                CompressorSpeedF * (2.0 * MaxCompressorF - Compressor) / MaxCompressorF *
                                EngineRPMRatio() * dt *
                                (CompressorGovernorLock ? 0.0 : 1.0); // with the lock active air is vented out
                        break;
                    }
                    default: {
                        // the compressor is a stand-alone device, working at steady pace
                        CompressedVolume +=
                                CompressorSpeedF * (2.0 * MaxCompressorF - Compressor) / MaxCompressorF * dt;
                        break;
                    }
            }

            if ((pressureistoohigh) && ((false == governorlockispresent) || (CompressorPower == 3))) {
                // vent some air out if there's no governor lock to stop the compressor from exceeding acceptable
                // pressure level
                SetFlag(SoundFlag, sound::relay | sound::loud);
                CompressedVolume *= (false == governorlockispresent ? 0.80 : // arbitrary amount
                                             CompressorTankValve ? MinCompressorF / MaxCompressorF
                                                                 : // drop to mincompressor level
                                             0.999); // HACK: drop a tiny bit so the sound doesn't trigger repeatedly
                    if ((false == governorlockispresent) || (CompressorTankValve)) {
                        CompressorGovernorLock = false;
                }
        }

            // tymczasowo tylko obciążenie sprężarki, tak z 5A na sprężarkę
            // TODO: draw power from proper high- or low voltage circuit
            switch (CompressorPower) {
                    case 3: {
                        // diesel-powered compressor doesn't draw power
                        break;
                    }
                    default: {
                        // TODO: drain power from 110v circuit
                        /*
                           if( compressorowner != nullptr ) {
                           compressorowner->TotalCurrent += 0.0015 * compressorowner->PantographVoltage;
                           }
                           */
                        break;
                    }
            }
    }

    // *************************************************************************************************
    // Q: 20160712
    // aktualizacja ciśnienia w przewodzie głównym
    // *************************************************************************************************
    void TMoverParameters::UpdatePipePressure(double dt) {
            if (PipePress > 1.0) {
                Pipe->Flow(-(PipePress)*AirLeakRate * dt);
                Pipe->Act();
        }

        const double LBDelay = 100;
        const double kL = 0.5;
        // double dV;
        // TMoverParameters *c; // T_MoverParameters
        double temp;
        // int b;

        PipePress = Pipe->P();
        //  PPP:=PipePress;

        dpMainValve = 0;

            if (BrakeCtrlPosNo > 1) {

                    if ((EngineType != TEngineType::ElectricInductionMotor)) {
                        double lbpa = LocalBrakePosA;
                            if ((EIMCtrlType > 0) && (UniCtrlIntegratedLocalBrakeCtrl)) {
                                lbpa = std::max(0.0, -eimic_real);
                        }
                            if (SpeedCtrlUnit.Parking) {
                                lbpa = std::max(lbpa, StopBrakeDecc);
                        }
                        dpLocalValve = LocHandle->GetPF(std::max(lbpa, LocalBrakePosAEIM), Hamulec->GetBCP(),
                                                        ScndPipePress, dt, 0);
                } else
                    dpLocalValve = LocHandle->GetPF(LocalBrakePosAEIM, Hamulec->GetBCP(), ScndPipePress, dt, 0);

                LockPipe = PipePress < (LockPipe ? LockPipeOff : LockPipeOn);
                bool lock_new =
                        (LockPipe && !UnlockPipe && (BrakeCtrlPosR > HandleUnlock)) ||
                        ((EmergencyCutsOffHandle) && (EmergencyValveFlow > 0)); // new simple codition based on .fiz
                bool lock_old = ((BrakeHandle == TBrakeHandle::FV4a) // old complex condition based on assumptions
                                 && ((PipePress < 2.75) && ((Hamulec->GetStatus() & b_rls) == 0)) &&
                                 (BrakeSubsystem == TBrakeSubSystem::ss_LSt) && (TrainType != dt_EZT) && (!UnlockPipe));

                    if ((lock_old) || (lock_new)) {
                        temp = PipePress + 0.00001;
                    } else {
                        temp = ScndPipePress;
                    }
                Handle->SetReductor(BrakeCtrlPos2);

                    if (((BrakeOpModes & bom_PS) == 0) || ((CabOccupied != 0) && (BrakeOpModeFlag != bom_PS))) {

                            if ((BrakeOpModeFlag < bom_EP) || ((Handle->GetPos(bh_EB) - 0.5) < BrakeCtrlPosR) ||
                                ((BrakeHandle != TBrakeHandle::MHZ_EN57) && (BrakeHandle != TBrakeHandle::MHZ_K8P))) {
                                double pos = BrakeCtrlPosR;
                                    if (SpeedCtrlUnit.IsActive && SpeedCtrlUnit.BrakeIntervention &&
                                        !SpeedCtrlUnit.Standby && (BrakeCtrlPos != Handle->GetPos(bh_EB))) {
                                        pos = Handle->GetPos(bh_NP);
                                        if (SpeedCtrlUnit.BrakeInterventionBraking)
                                            pos = Handle->GetPos(bh_FB);
                                        if (SpeedCtrlUnit.BrakeInterventionUnbraking)
                                            pos = Handle->GetPos(bh_RP);
                                }
                                dpMainValve = Handle->GetPF(pos, PipePress, temp, dt, EqvtPipePress);
                            } else {
                                dpMainValve = Handle->GetPF(0, PipePress, temp, dt, EqvtPipePress);
                            }
                    } else if (BrakeCtrlPos == Handle->GetPos(bh_EB)) {
                        dpMainValve = Handle->GetPF(BrakeCtrlPosR, PipePress, temp, dt, EqvtPipePress);
                }

                if (dpMainValve < 0) // && (PipePressureVal > 0.01)           //50
                        if (Compressor > ScndPipePress) {
                            CompressedVolume = CompressedVolume + dpMainValve / 1500.0;
                            Pipe2->Flow(dpMainValve / 3.0);
                    } else
                        Pipe2->Flow(dpMainValve);
        }

        // ulepszony hamulec bezp.
        EmergencyValveFlow = 0.0;

        auto const lowvoltagepower{Power24vIsAvailable || Power110vIsAvailable};

            if (((true == RadioStopFlag) || (true == AlarmChainFlag) ||
                 ((true == EIMCtrlEmergency) && (LocalBrakePosA >= 1.0)) || SecuritySystem.is_braking()) ||
                ((SpringBrakeDriveEmergencyVel >= 0) && (Vel > SpringBrakeDriveEmergencyVel) && (SpringBrake.IsActive))
                /*
                // NOTE: disabled because 32 is 'load destroyed' flag, what does this have to do with emergency brake?
                // (if it's supposed to be broken coupler, such event sets alarmchainflag instead when appropriate)
                || ( true == TestFlag( EngDmgFlag, 32 ) )
                */
                || ((0 == CabActive) && (InactiveCabFlag & activation::emergencybrake)) ||
                ((SpringBrakeDriveEmergencyVel >= 0) && (Vel > SpringBrakeDriveEmergencyVel) &&
                 (SpringBrake.IsActive))) {
                EmergencyValveFlow = PF(0, PipePress, 0.15) * dt;
        }
        dpMainValve += EmergencyValveFlow;

        // 0.2*Spg
        Pipe->Flow(-dpMainValve);
        Pipe->Flow(-(PipePress) * 0.001 * dt);
        //      if Heating then
        //        Pipe.Flow(PF(PipePress, 0, d2A(7)) * dt);
        //      if ConverterFlag then
        //        Pipe.Flow(PF(PipePress, 0, d2A(12)) * dt);
        dpMainValve = dpMainValve / (Dim.L * Spg * 20);

        CntrlPipePress = Hamulec->GetVRP(); // ciśnienie komory wstępnej rozdzielacza

        //      if (Hamulec is typeid(TWest)) return 0;

            switch (BrakeValve) {

                case TBrakeValve::K:
                    case TBrakeValve::W: {

                            if (BrakeLocHandle != TBrakeHandle::NoHandle) {
                                LocBrakePress = LocHandle->GetCP();

                                //(Hamulec as TWest).SetLBP(LocBrakePress);
                                Hamulec->SetLBP(LocBrakePress);
                        }
                        if (MBPM < 2)
                            //(Hamulec as TWest).PLC(MaxBrakePress[LoadFlag])
                            Hamulec->PLC(MaxBrakePress[LoadFlag]);
                        else
                            //(Hamulec as TWest).PLC(TotalMass);
                            Hamulec->PLC(TotalMass - Mred);
                        break;
                    }

                case TBrakeValve::LSt:
                    case TBrakeValve::EStED: {

                        LocBrakePress = LocHandle->GetCP();
                        for (int b = 0; b < 2; b++)
                            if (((TrainType & (dt_ET41 | dt_ET42)) != 0) &&
                                (Couplers[b].Connected != NULL)) // nie podoba mi się to rozwiązanie, chyba trzeba
                                                                 // dodać jakiś wpis do fizyki na to
                                if (((Couplers[b].Connected->TrainType & (dt_ET41 | dt_ET42)) != 0) &&
                                    ((Couplers[b].CouplingFlag & 36) == 36))
                                    LocBrakePress = std::max(Couplers[b].Connected->LocHandle->GetCP(), LocBrakePress);

                        // if ((DynamicBrakeFlag) && (EngineType == ElectricInductionMotor))
                        //{
                        //     //if (Vel > 10)
                        //     //    LocBrakePress = 0;
                        //     //else if (Vel > 5)
                        //     //    LocBrakePress = (10 - Vel) / 5 * LocBrakePress;
                        // }

                        //(Hamulec as TLSt).SetLBP(LocBrakePress);
                        Hamulec->SetLBP(LocBrakePress);
                        if ((BrakeValve == TBrakeValve::EStED))
                            if (MBPM < 2)
                                Hamulec->PLC(MaxBrakePress[LoadFlag]);
                            else
                                Hamulec->PLC(TotalMass - Mred);
                        break;
                    }

                    case TBrakeValve::CV1_L_TR: {
                        LocBrakePress = LocHandle->GetCP();
                        //(Hamulec as TCV1L_TR).SetLBP(LocBrakePress);
                        Hamulec->SetLBP(LocBrakePress);
                        break;
                    }

                case TBrakeValve::EP2:
                    case TBrakeValve::EP1: {
                        Hamulec->PLC(TotalMass - Mred);
                        break;
                    }
                case TBrakeValve::ESt3AL2:
                case TBrakeValve::NESt3:
                case TBrakeValve::ESt4:
                    case TBrakeValve::ESt3: {
                        if (MBPM < 2)
                            //(Hamulec as TNESt3).PLC(MaxBrakePress[LoadFlag])
                            Hamulec->PLC(MaxBrakePress[LoadFlag]);
                        else
                            //(Hamulec as TNESt3).PLC(TotalMass);
                            Hamulec->PLC(TotalMass - Mred);
                        LocBrakePress = LocHandle->GetCP();
                        //(Hamulec as TNESt3).SetLBP(LocBrakePress);
                        Hamulec->SetLBP(LocBrakePress);
                        break;
                    }
                    case TBrakeValve::KE: {
                        LocBrakePress = LocHandle->GetCP();
                        //(Hamulec as TKE).SetLBP(LocBrakePress);
                        Hamulec->SetLBP(LocBrakePress);
                        if (MBPM < 2)
                            //(Hamulec as TKE).PLC(MaxBrakePress[LoadFlag])
                            Hamulec->PLC(MaxBrakePress[LoadFlag]);
                        else
                            //(Hamulec as TKE).PLC(TotalMass);
                            Hamulec->PLC(TotalMass - Mred);
                        break;
                    }
                    default: {
                        // unsupported brake valve type, we should never land here
                        //            ErrorLog( "Unsupported brake valve type (" + std::to_string( BrakeValve ) + ") in
                        //            " + TypeName );
                        //            ::PostQuitMessage( 0 );
                        break;
                    }
            } // switch

            if (((BrakeHandle == TBrakeHandle::FVel6) || (BrakeHandle == TBrakeHandle::FVE408)) && (CabOccupied != 0)) {
                if ((Power24vIsAvailable) && (DirActive != 0) && (EpFuse)) // tu powinien byc jeszcze bezpiecznik EP i
                                                                           // baterie - temp = (Handle as TFVel6).GetCP
                    EpForce = Handle->GetEP();
                else
                    EpForce = 0.0;

                DynamicBrakeEMUStatus = (EpForce > 0.001 ? Power110vIsAvailable : true);

                double temp1 = EpForce;
                if ((DCEMUED_EP_max_Vel > 0.001) && (Vel > DCEMUED_EP_max_Vel) && (DynamicBrakeEMUStatus))
                    temp1 = 0;
                if ((DCEMUED_EP_min_Im > 0.001) && (fabs(Im) > DCEMUED_EP_min_Im) && (DynamicBrakeEMUStatus))
                    temp1 = 0;
                Hamulec->SetEPS(temp1);
                TUHEX_StageActual = EpForce;
                TUHEX_Active = TUHEX_StageActual > 0;
                // Ra 2014-11: na tym się wysypuje, ale nie wiem, w jakich warunkach
                SendCtrlToNext("Brake", EpForce, CabActive);
        }

        Pipe->Act();
        PipePress = Pipe->P();
        if ((Hamulec->GetBrakeStatus() & b_dmg) == b_dmg) // jesli hamulec wyłączony
            temp = 0.0;                                   // odetnij
        else
            temp = 1.0; // połącz
        Pipe->Flow(temp * Hamulec->GetPF(temp * PipePress, dt, Vel) + GetDVc(dt));

        if (ASBType == 128)
            Hamulec->ASB(int(SlippingWheels && (Vel > 1)) * (1 + 2 * int(nrot_eps < -0.01)));

        dpPipe = 0;

        // yB: jednokrokowe liczenie tego wszystkiego
        Pipe->Act();
        PipePress = Pipe->P();

        dpMainValve = dpMainValve / (100.0 * dt); // normalizacja po czasie do syczenia;

            if (PipePress < -1.0) {
                PipePress = -1.0;
                Pipe->CreatePress(-1.0);
                Pipe->Act();
        }

        if (CompressedVolume < 0.0)
            CompressedVolume = 0.0;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Aktualizacja ciśnienia w przewodzie zasilającym
    // *************************************************************************************************
    void TMoverParameters::UpdateScndPipePressure(double dt) {
            if (ScndPipePress > 1.0) {
                Pipe2->Flow(-(ScndPipePress)*AirLeakRate * dt);
                Pipe2->Act();
        }

        const double Spz = 0.5067;
        TMoverParameters *c;
        double dv1, dv2, dV;

        UpdateSpringBrake(dt);

        dv1 = 0;
        dv2 = 0;

        // sprzeg 1
        if (Couplers[0].Connected != NULL)
                if (TestFlag(Couplers[0].CouplingFlag, ctrain_scndpneumatic)) {
                    c = Couplers[0].Connected; // skrot
                    dv1 = 0.5 * dt * PF(ScndPipePress, c->ScndPipePress, Spz * 0.75);
                    if (dv1 * dv1 > 0.00000000000001)
                        c->switch_physics(true);
                    c->Pipe2->Flow(-dv1);
            }
        // sprzeg 2
        if (Couplers[1].Connected != NULL)
                if (TestFlag(Couplers[1].CouplingFlag, ctrain_scndpneumatic)) {
                    c = Couplers[1].Connected; // skrot
                    dv2 = 0.5 * dt * PF(ScndPipePress, c->ScndPipePress, Spz * 0.75);
                    if (dv2 * dv2 > 0.00000000000001)
                        c->switch_physics(true);
                    c->Pipe2->Flow(-dv2);
            }
        if ((Couplers[1].Connected != NULL) && (Couplers[0].Connected != NULL))
                if ((TestFlag(Couplers[0].CouplingFlag, ctrain_scndpneumatic)) &&
                    (TestFlag(Couplers[1].CouplingFlag, ctrain_scndpneumatic))) {
                    dV = 0.00025 * dt *
                         PF(Couplers[0].Connected->ScndPipePress, Couplers[1].Connected->ScndPipePress, Spz * 0.25);
                    Couplers[0].Connected->Pipe2->Flow(+dV);
                    Couplers[1].Connected->Pipe2->Flow(-dV);
            }

        Pipe2->Flow(Hamulec->GetHPFlow(ScndPipePress, dt));
            // NOTE: condition disabled to allow the air flow from the main hose to the main tank as well
            if (/* ( ( Compressor > ScndPipePress ) && ( */ VeselVolume >
                0.0 /* ) ) || ( TrainType == dt_EZT ) || ( TrainType == dt_DMU ) */) {
                dV = PF(Compressor, ScndPipePress, Spz) * dt;
                CompressedVolume += dV / 1000.0;
                Pipe2->Flow(-dV);
        }
        Pipe2->Flow(dv1 + dv2);
        Pipe2->Act();
        ScndPipePress = Pipe2->P();

            if (ScndPipePress < -1) {
                ScndPipePress = -1;
                Pipe2->CreatePress(-1);
                Pipe2->Act();
        }
    }


    // *************************************************************************************************
    // yB: 20190906
    // Aktualizacja ciśnienia w hamulcu sprezynowym
    // *************************************************************************************************
    void TMoverParameters::UpdateSpringBrake(double dt) {
        double BP = SpringBrake.PNBrakeConnection ? BrakePress : 0;
        double MSP = SpringBrake.ShuttOff ? 0 : SpringBrake.MaxSetPressure;
            if (!SpringBrake.Activate) {
                double desired_press = std::min(std::max(MSP, BP), Pipe2->P());
                double dv = PF(desired_press, SpringBrake.SBP, SpringBrake.ValveOffArea);
                SpringBrake.Cylinder->Flow(-dv);
                Pipe2->Flow(std::max(dv, 0.0));
            } else {
                double dv = PF(BP, SpringBrake.SBP, SpringBrake.ValveOnArea);
                SpringBrake.Cylinder->Flow(-dv);
            }
        if (SpringBrake.SBP > SpringBrake.ResetPressure)
            SpringBrake.IsReady = true;

        SpringBrake.IsActive =
                SpringBrake.SBP < (SpringBrake.IsActive ? SpringBrake.PressureOff : SpringBrake.PressureOn);

        SpringBrake.Release = false;
        SpringBrake.Cylinder->Act();
        SpringBrake.SBP = SpringBrake.Cylinder->P();
    }

    // *************************************************************************************************
    // Q: 20160715
    // oblicza i zwraca przepływ powietrza pomiędzy pojazdami
    // *************************************************************************************************
    double TMoverParameters::GetDVc(double dt) {
        // T_MoverParameters *c;
        TMoverParameters *c;
        double dv1, dv2; // , dV;

        dv1 = 0;
        dv2 = 0;
        // sprzeg 1
        if (Couplers[0].Connected != NULL)
                if (TestFlag(Couplers[0].CouplingFlag, ctrain_pneumatic)) { //*0.85
                    c = Couplers[0].Connected; // skrot           //0.08           //e/D * L/D = e/D^2 * L
                    dv1 = 0.5 * dt * PF(PipePress, c->PipePress, (Spg) / (1.0 + 0.015 / Spg * Dim.L));
                    if (dv1 * dv1 > 0.00000000000001)
                        c->switch_physics(true);
                    c->Pipe->Flow(-dv1);
            }
        // sprzeg 2
        if (Couplers[1].Connected != NULL)
                if (TestFlag(Couplers[1].CouplingFlag, ctrain_pneumatic)) {
                    c = Couplers[1].Connected; // skrot
                    dv2 = 0.5 * dt * PF(PipePress, c->PipePress, (Spg) / (1.0 + 0.015 / Spg * Dim.L));
                    if (dv2 * dv2 > 0.00000000000001)
                        c->switch_physics(true);
                    c->Pipe->Flow(-dv2);
            }
        // if ((Couplers[1].Connected != NULL) && (Couplers[0].Connected != NULL))
        //     if ((TestFlag(Couplers[0].CouplingFlag, ctrain_pneumatic)) &&
        //         (TestFlag(Couplers[1].CouplingFlag, ctrain_pneumatic)))
        //     {
        //         dV = 0.05 * dt * PF(Couplers[0].Connected->PipePress, Couplers[1].Connected->PipePress,
        //                             (Spg * 0.85) / (1 + 0.03 * Dim.L)) *
        //              0; // ktoś mi powie jaki jest sens tego bloku jeśli przepływ mnożony przez zero?
        //         Couplers[0].Connected->Pipe->Flow(+dV);
        //         Couplers[1].Connected->Pipe->Flow(-dV);
        //     }
        //  suma
        return dv2 + dv1;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Obliczenie stałych potrzebnych do dalszych obliczeń
    // *************************************************************************************************
    void TMoverParameters::ComputeConstans(void) {
        double BearingF, RollF, HideModifier;
        double Curvature; // Ra 2014-07: odwrotność promienia

        TotalMassxg = TotalMass * g; // TotalMass*g
        BearingF = DamageFlag & dtrain_bearing > 0 ? 2.0 : 0;

        HideModifier = 0; // int(Couplers[0].CouplingFlag>0)+int(Couplers[1].CouplingFlag>0);

        if (BearingType == 0)
            RollF = 0.05; // slizgowe
        else
            RollF = 0.015; // toczne
        RollF += BearingF / 200.0;

        //    if (NPoweredAxles > 0)
        //     RollF = RollF * 1.5;    //dodatkowe lozyska silnikow

            if (NPoweredAxles > 0) // drobna optymalka
            {
                RollF += 0.025;
                //       if (Ft * Ft < 1)
                //         HideModifier = HideModifier - 3;
        }
        Ff = TotalMassxg * (BearingF + RollF * V * V / 10.0) / 1000.0;
        // dorobic liczenie temperatury lozyska!
        FrictConst1 = (TotalMassxg * RollF) / 10000.0;
        // drag calculation
        {
            // NOTE: draft effect of previous vehicle is simplified and doesn't have much to do with reality
            auto const *previousvehicle{Couplers[(V >= 0.0 ? end::front : end::rear)].Connected};
            auto dragarea{Dim.W * Dim.H};
                if (previousvehicle) {
                    dragarea = std::max(0.0, dragarea - (0.85 * previousvehicle->Dim.W * previousvehicle->Dim.H));
            }
            FrictConst1 += Cx * dragarea;
        }

            if (CategoryFlag & 1) {
                Curvature = (RunningShape.R == 0.0 ? // zero oznacza nieskończony promień
                                     0.0
                                                   : 1.0 / std::fabs(RunningShape.R));
            } else {
                // vehicles other than trains don't experience friction against the rail on curves
                Curvature = 0.0;
            }
        // opór składu na łuku (youBy): +(500*TrackW/R)*TotalMassxg*0.001 do FrictConst2s/d
        FrictConst2s =
                (TotalMassxg * ((500.0 * TrackW * Curvature) + 2.5 - HideModifier + 2 * BearingF / dtrain_bearing)) *
                0.001;
        FrictConst2d =
                (TotalMassxg * ((500.0 * TrackW * Curvature) + 2.0 - HideModifier + BearingF / dtrain_bearing)) * 0.001;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Oblicza masę ładunku
    // *************************************************************************************************
    void TMoverParameters::ComputeMass() {
        // Ra: na razie tak, ale nie wszędzie masy wirujące się wliczają
        TotalMass = Mass + Mred;

            if (LoadAmount == 0) {
                return;
        }

        // include weight of carried load
        auto loadtypeunitweight{0.f};

            if (ToLower(LoadQuantity) == "tonns") {
                loadtypeunitweight = 1000;
            } else {
                auto const lookup{simulation::Weights.find(LoadType.name)};
                loadtypeunitweight = (lookup != simulation::Weights.end() ? lookup->second
                                                                          : 1000.f); // legacy default unit weight value
            }

        TotalMass += LoadAmount * loadtypeunitweight;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Obliczanie wypadkowej siły z wszystkich działających sił
    // *************************************************************************************************
    // TBD, TODO: move some of the calculations out of the method, they're relevant to more than just force calculations
    void TMoverParameters::ComputeTotalForce(double dt) {

        Vel = std::fabs(V) * 3.6; // prędkość w km/h

        // McZapkie-031103: sprawdzanie czy warto liczyc fizyke i inne updaty
        // ABu 300105: cos tu mieszalem , dziala teraz troche lepiej, wiec zostawiam
        {
            auto const vehicleisactive{
                    (CabActive != 0) || (Vel > 0.0001) || (std::fabs(AccS) > 0.0001) || (LastSwitchingTime < 5) ||
                    (TrainType == dt_EZT) || (TrainType == dt_DMU)};

            auto const movingvehicleahead{
                    (Neighbours[end::front].vehicle != nullptr) &&
                    ((Neighbours[end::front].vehicle->Vel > 0.0001) ||
                     (std::fabs(Neighbours[end::front].vehicle->AccS) > 0.0001))};

            auto const movingvehiclebehind{
                    (Neighbours[end::rear].vehicle != nullptr) &&
                    ((Neighbours[end::rear].vehicle->Vel > 0.0001) ||
                     (std::fabs(Neighbours[end::rear].vehicle->AccS) > 0.0001))};

            auto const calculatephysics{vehicleisactive || movingvehicleahead || movingvehiclebehind};

            switch_physics(calculatephysics);
        }

            if (false == PhysicActivation) {
                return;
        }

        // juz zoptymalizowane:
        FStand = FrictionForce(); // siła oporów ruchu
            if (true == TestFlag(DamageFlag, dtrain_out)) {
                // HACK: crude way to reduce speed after derailment
                // TBD, TODO: more accurate approach?
                FStand *= 1e20;
        }
        double old_nrot = fabs(nrot);
        nrot = v2n(); // przeliczenie prędkości liniowej na obrotową

            if ((true == TestFlag(BrakeMethod, bp_MHS)) && (PipePress < 3.0) // ustawione na sztywno na 3 bar
                && (Vel > 45) && (true == TestFlag(BrakeDelayFlag, bdelay_M))) {
                // doliczenie hamowania hamulcem szynowym
                FStand += TrackBrakeForce;
        }
        // w charakterystykach jest wartość siły hamowania zamiast nacisku

        LastSwitchingTime += dt;
            if (EngineType == TEngineType::ElectricSeriesMotor) {
                LastRelayTime += dt;
        }

            if (EngineType == TEngineType::ElectricSeriesMotor) // potem ulepszyc! pantogtrafy!
            { // Ra 2014-03: uwzględnienie kierunku jazdy w napięciu na silnikach, a powinien być zdefiniowany nawrotnik
                EngineVoltage = (Mains ? EnginePowerSourceVoltage() : 0.0);
                    if (CabActive == 0) {
                        EngineVoltage *= DirActive;
                    } else {
                        EngineVoltage *= DirAbsolute; // DirActive*CabActive;
                    }
            } // bo nie dzialalo
            else {
                EngineVoltage = (Power > 1.0 ? std::max(GetTrainsetHighVoltage(), PantographVoltage) : 0.0);
            }

        FTrain = (Power > 0 ? TractionForce(dt) : 0);
        double FT_factor = 1.0;
            if (EngineType == TEngineType::ElectricInductionMotor && InvertersRatio > 0.0) {
                FT_factor = 1.0 / InvertersRatio;
                FTrain *= FT_factor;
        }

        Fb = BrakeForce(RunningTrack);
        // poslizg
        auto Fwheels{FTrain - Fb * Sign(V)};
            if ((Vel > 0.1) // crude trap, to prevent braked stationary vehicles from passing fb > mass * adhesive test
                && (std::fabs(Fwheels) > TotalMassxg * Adhesive(RunningTrack.friction))) {
                SlippingWheels = true;
        }
        double temp_nrot = nrot;
            if (true == SlippingWheels) {

                temp_nrot = ComputeRotatingWheel(Fwheels - Sign(nrot * M_PI * WheelDiameter - V) *
                                                                   Adhesive(RunningTrack.friction) * TotalMassxg,
                                                 dt, nrot);
                    if (Sign(nrot * M_PI * WheelDiameter - V) * Sign(temp_nrot * M_PI * WheelDiameter - V) < 0) {
                        SlippingWheels = false;
                        temp_nrot = V / M_PI / WheelDiameter;
                }
        }
            if (true == SlippingWheels) {
                Fwheels = Sign(temp_nrot * M_PI * WheelDiameter - V) * TotalMassxg * Adhesive(RunningTrack.friction);
                    if (Fwheels * Sign(V) > 0) {
                        FTrain = Fwheels + Fb * Sign(V);
                    } else if (FTrain * Sign(V) > 0) {
                        Fb = FTrain * Sign(V) - Fwheels * Sign(V);
                    } else {
                        double factor = (FTrain - Fb * Sign(V) != 0 ? Fwheels / (FTrain - Fb * Sign(V)) : 1.0);
                        Fb *= factor;
                        FTrain *= factor;
                    }
                    if (std::fabs(nrot) < 0.1) {
                        WheelFlat = sqrt(square(WheelFlat) + fabs(Fwheels) / NAxles * Vel * 0.000002);
                }

                nrot = temp_nrot;
        }
        nrot_eps = (fabs(nrot) - (old_nrot)) / dt;
        // doliczenie sił z innych pojazdów
        for (int end = end::front; end <= end::rear; ++end) {
                if (Neighbours[end].vehicle != nullptr) {
                    Couplers[end].CForce = CouplerForce(end, dt);
                    FTrain += Couplers[end].CForce;
                } else {
                    Couplers[end].CForce = 0;
                }
        }

        FStand += Fb;
        // doliczenie składowej stycznej grawitacji
        FTrain /= FT_factor;
        FTrain += TotalMassxg * RunningShape.dHtrack;
        //! niejawne przypisanie zmiennej!
        FTotal = FTrain - Sign(V) * FStand;
    }

    double TMoverParameters::BrakeForceR(double ratio, double velocity) {
        double press = 0;
            if (MBPM > 2) {
                press = MaxBrakePress[1] +
                        (MaxBrakePress[3] - MaxBrakePress[1]) * std::min(1.0, (TotalMass - Mass) / (MBPM - Mass));
            } else {
                    if (MaxBrakePress[1] > 0.1) {
                        press = MaxBrakePress[LoadFlag];
                    } else {
                        press = MaxBrakePress[3];
                        if (DynamicBrakeType == dbrake_automatic)
                            ratio = ratio + (1.5 - ratio) * std::min(1.0, Vel * 0.02);
                        if ((BrakeDelayFlag & bdelay_R) && (BrakeMethod % 128 != bp_Cosid) &&
                            (BrakeMethod % 128 != bp_D1) && (BrakeMethod % 128 != bp_D2) && (Power < 1) &&
                            (velocity < 40))
                            ratio = ratio / 2;
                            if ((TrainType == dt_DMU) && (velocity < 30.0)) {
                                ratio -= 0.3;
                        }
                    }
            }
        return BrakeForceP(press * ratio, velocity);
    }

    double TMoverParameters::BrakeForceP(double press, double velocity) {
        double BFP = 0;
        double K = (((press * P2FTrans) - BrakeCylSpring) * BrakeCylMult[0] - BrakeSlckAdj) * BrakeRigEff;
        K *= static_cast<double>(BrakeCylNo) / (NAxles * std::max(1, NBpA));
        BFP = Hamulec->GetFC(velocity, K) * K * (NAxles * std::max(1, NBpA)) * 1000;
        return BFP;
    }

    // *************************************************************************************************
    // Q: 20160713
    // oblicza siłę na styku koła i szyny
    // *************************************************************************************************
    double TMoverParameters::BrakeForce(TTrackParam const &Track) {

        double K{0}, Fb{0}, sm{0};

            switch (LocalBrake) {
                    case TLocalBrake::ManualBrake: {
                        K = MaxBrakeForce * ManualBrakeRatio();
                        break;
                    }
                    case TLocalBrake::HydraulicBrake: {
                        K = MaxBrakeForce * LocalBrakeRatio();
                        break;
                    }
                    default: {
                        break;
                    }
            }

            if (MBrake == true) {
                K = MaxBrakeForce * ManualBrakeRatio();
        }

        if (SpringBrake.IsReady)
            K += std::max(0.0, SpringBrake.MinForcePressure - SpringBrake.Cylinder->P()) * SpringBrake.MaxBrakeForce;

        u = ((BrakePress * P2FTrans) - BrakeCylSpring) * BrakeCylMult[0] - BrakeSlckAdj;
        if (u * BrakeRigEff > Ntotal) // histereza na nacisku klockow
            Ntotal = u * BrakeRigEff;
            else {
                u = ((BrakePress * P2FTrans) - BrakeCylSpring) * BrakeCylMult[0] - BrakeSlckAdj;
                if (u * (2.0 - BrakeRigEff) < Ntotal) // histereza na nacisku klockow
                    Ntotal = u * (2.0 - BrakeRigEff);
            }

        auto const NBrakeAxles{NAxles};

            if (NBrakeAxles * NBpA > 0) {
                if (Ntotal > 0)                                                                   // nie luz
                    K += Ntotal;                                                                  // w kN
                K *= static_cast<double>(BrakeCylNo) / (NBrakeAxles * static_cast<double>(NBpA)); // w kN na os
        }
            if ((BrakeSystem == TBrakeSystem::Pneumatic) || (BrakeSystem == TBrakeSystem::ElectroPneumatic)) {
                u = Hamulec->GetFC(Vel, K);
                UnitBrakeForce = u * K * 1000.0; // sila na jeden klocek w N
        } else
            UnitBrakeForce = K * 1000.0;
        //     if (LocalBrake=ManualBrake)or(MBrake=true)) and (BrakePress<0.3) then
        //      Fb:=UnitBrakeForce*NBpA {ham. reczny dziala na jedna os}
        //     else  //yB: to nie do konca ma sens, ponieważ ręczny w wagonie działa na jeden cylinder
        //     hamulcowy/wózek, dlatego potrzebne są oddzielnie liczone osie
        Fb = UnitBrakeForce * NBrakeAxles * std::max(1, NBpA);

        //  u:=((BrakePress*P2FTrans)-BrakeCylSpring*BrakeCylMult[BCMFlag]/BrakeCylNo-0.83*BrakeSlckAdj/(BrakeCylNo))*BrakeCylNo;
        // {  end; }
        return Fb;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Obliczanie siły tarcia
    // *************************************************************************************************
    double TMoverParameters::FrictionForce() const {
        double FF = 0;
        // ABu 240205: chyba juz ekstremalnie zoptymalizowana funkcja liczaca sily tarcia
        if (fabs(V) > 0.01)
            FF = (FrictConst1 * V * V) + FrictConst2d;
        else
            FF = (FrictConst1 * V * V) + FrictConst2s;
        return FF;
    }


    // *************************************************************************************************
    // Q: 20160713
    // Oblicza przyczepność
    // *************************************************************************************************
    double TMoverParameters::Adhesive(double staticfriction) const {
        double adhesion = 0.0;
        const double adh_factor = 0.25; // współczynnik określający, jak bardzo spada tarcie przy poślizgu
        const double slipfactor = 0.33; // współczynnik określający, jak szybko spada tarcie przy poślizgu
        const double sandfactor = 1.25; // współczynnik określający, jak mocno pomaga piasek
        /*
        // ABu: male przerobki, tylko czy to da jakikolwiek skutek w FPS?
        //     w kazdym razie zaciemni kod na pewno :)
        if (SlippingWheels == false)
        {
        if (SandDose)
        adhesion = (Max0R(staticfriction * (100.0 + Vel) / ((50.0 + Vel) * 11.0), 0.048)) *
        (11.0 - 2.0 * Random(0.0, 1.0));
        else
        adhesion = (staticfriction * (100.0 + Vel) / ((50.0 + Vel) * 10.0)) *
        (11.0 - 2.0 * Random(0.0, 1.0));
        }
        else
        {
        if (SandDose)
        adhesion = (0.048) * (11.0 - 2.0 * Random(0.0, 1.0));
        else
        adhesion = (staticfriction * 0.02) * (11.0 - 2.0 * Random(0.0, 1.0));
        }
        //  WriteLog(FloatToStr(adhesive));      // tutaj jest na poziomie 0.2 - 0.3
        return adhesion;

        //wersja druga
        if( true == SlippingWheels ) {

        if( true == SandDose ) { adhesion = 0.48; }
        else                   { adhesion = staticfriction * 0.2; }
        }
        else {

        if( true == SandDose ) { adhesion = std::max( staticfriction * ( 100.0 + Vel ) / ( 50.0 + Vel ) * 1.1, 0.48 ); }
        else                   { adhesion = staticfriction * ( 100.0 + Vel ) / ( 50.0 + Vel ); }
        }
        //    adhesion *= ( 0.9 + 0.2 * Random() );
        */
        // wersja3 by youBy - uwzględnia naturalne mikropoślizgi i wpływ piasecznicy, usuwa losowość z pojazdu
        double Vwheels = nrot * M_PI * WheelDiameter; // predkosc liniowa koła wynikająca z obrotowej
        double deltaV = V - Vwheels;                  // poślizg - różnica prędkości w punkcie styku koła i szyny
        deltaV = std::max(0.0, std::fabs(deltaV) - 0.25); // mikropoślizgi do ok. 0,25 m/s nie zrywają przyczepności
        Vwheels = std::fabs(Vwheels);
        adhesion = staticfriction * (28 + Vwheels) / (14 + Vwheels) *
                   ((SandDose ? sandfactor : 1) - (1 - adh_factor) * (deltaV / (deltaV + slipfactor)));

        return adhesion;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Obliczanie sił dzialających na sprzęgach
    // *************************************************************************************************
    double TMoverParameters::CouplerForce(int const End, double dt) {

        auto &coupler{Couplers[End]};
        auto *othervehicle{Neighbours[End].vehicle};
        auto const otherend{Neighbours[End].vehicle_end};
        auto &othercoupler{othervehicle->Couplers[otherend]};

        auto const othervehiclemove{(othervehicle->dMoveLen * DirPatch(End, otherend))};
        auto const initialdistance{Neighbours[End].distance}; // odległość od sprzęgu sąsiada
        auto const distancedelta{(End == end::front ? othervehiclemove - dMoveLen : dMoveLen - othervehiclemove)};

        auto const newdistance{initialdistance + 10.0 * distancedelta};

        auto const dV{V - (othervehicle->V * DirPatch(End, otherend))};
        auto const absdV{std::fabs(dV)};

            // potentially generate sounds on clash or stretch
            if ((newdistance < 0.0) && (coupler.Dist > newdistance) && (dV < -0.1) &&
                (false == coupler.has_adapter())) { // HACK: with adapter present we presume buffers won't clash
                                                    // 090503: dzwieki pracy zderzakow
                SetFlag(coupler.sounds, (absdV > 5.0 ? (sound::bufferclash | sound::loud) : sound::bufferclash));
            } else if ((coupler.CouplingFlag != coupling::faux) && (newdistance > 0.001) && (coupler.Dist <= 0.001) &&
                       (absdV > 0.005) && (Vel > 1.0)) {
                // 090503: dzwieki pracy sprzegu
                SetFlag(coupler.sounds,
                        (absdV > 0.035 ? (sound::couplerstretch | sound::loud) : sound::couplerstretch));
        }

        coupler.CheckCollision = false;
        coupler.Dist = 0.0;

        double CF{0.0};

            if ((coupler.CouplingFlag == coupling::faux) &&
                (initialdistance > 0.05)) { // arbitrary distance
                                            // potentially reset auto coupling lock
                coupler.AutomaticCouplingAllowed = true;
        }

            if ((coupler.CouplingFlag != coupling::faux) || (initialdistance < 0)) {

                coupler.Dist = clamp(newdistance, (coupler.has_adapter() ? 0 : -coupler.DmaxB), coupler.DmaxC);

                double BetaAvg = 0;
                double Fmax = 0;

                    if (coupler.CouplingFlag == coupling::faux) {

                        BetaAvg = coupler.beta;
                        Fmax = (coupler.FmaxC + coupler.FmaxB) * CouplerTune;
                    } else {
                        // usrednij bo wspolny sprzeg
                        BetaAvg = 0.5 * (coupler.beta + othercoupler.beta);
                        Fmax = 0.5 * (coupler.FmaxC + coupler.FmaxB + othercoupler.FmaxC + othercoupler.FmaxB) *
                               CouplerTune;
                    }
                auto const distDelta{std::fabs(newdistance) -
                                     std::fabs(coupler.Dist)}; // McZapkie-191103: poprawka na histereze

                    if (newdistance > 0) {

                            if (distDelta > 0) {
                                CF = (-(coupler.SpringKC + othercoupler.SpringKC) * coupler.Dist / 2.0) * DirF(End) -
                                     Fmax * dV * BetaAvg;
                            } else {
                                CF = (-(coupler.SpringKC + othercoupler.SpringKC) * coupler.Dist / 2.0) * DirF(End) *
                                             BetaAvg -
                                     Fmax * dV * BetaAvg;
                            }
                            // liczenie sily ze sprezystosci sprzegu
                            if (newdistance > (coupler.DmaxC + othercoupler.DmaxC)) {
                                // zderzenie
                                coupler.CheckCollision = true;
                        }
                            if (std::fabs(CF) > coupler.FmaxC) {
                                // coupler is stretched with excessive force, may break
                                coupler.stretch_duration += dt;
                                    // give coupler 1 sec of leeway to account for simulation glitches, before checking
                                    // whether it breaks (arbitrary) chance to break grows from 10-100% over 10 sec
                                    // period
                                    if (Global_crash_damage && (coupler.stretch_duration > 1.f) &&
                                        (Random() < (coupler.stretch_duration * 0.1f * dt))) {
                                        damage_coupler(End);
                                }
                            } else {
                                coupler.stretch_duration = 0.f;
                            }
                }
                    if (newdistance < 0) {

                            if (distDelta > 0) {
                                CF = (-(coupler.SpringKB + othercoupler.SpringKB) * coupler.Dist / 2.0) * DirF(End) -
                                     Fmax * dV * BetaAvg;
                            } else {
                                CF = (-(coupler.SpringKB + othercoupler.SpringKB) * coupler.Dist / 2.0) * DirF(End) *
                                             BetaAvg -
                                     Fmax * dV * BetaAvg;
                            }
                        // liczenie sily ze sprezystosci zderzaka
                        auto const collisiondistance{
                                ((coupler.has_adapter() || othercoupler.has_adapter())
                                         ? std::min(coupler.DmaxB, othercoupler.DmaxB)
                                         : // HACK: only take into account buffering ability of automatic coupler
                                         coupler.DmaxB + othercoupler.DmaxB)};
                            if (-newdistance > collisiondistance) {
                                // zderzenie
                                coupler.CheckCollision = true;
                        }
                            if (-newdistance >= std::min(collisiondistance, dEpsilon)) {
                                    if ((coupler.type() == TCouplerType::Automatic) &&
                                        (coupler.type() == othercoupler.type()) &&
                                        (coupler.CouplingFlag == coupling::faux) &&
                                        (coupler.AutomaticCouplingAllowed && othercoupler.AutomaticCouplingAllowed)) {
                                        // sprzeganie wagonow z samoczynnymi sprzegami
                                        auto couplingtype{coupler.AutomaticCouplingFlag &
                                                          othercoupler.AutomaticCouplingFlag};
                                            // potentially exclude incompatible control coupling
                                            if (coupler.control_type != othercoupler.control_type) {
                                                couplingtype &= ~(coupling::control);
                                        }

                                            if (Attach(End, otherend, othervehicle, couplingtype)) {
                                                // HACK: we're reusing sound enum to mark whether vehicle was connected
                                                // to another
                                                SetFlag(AIFlag, sound::attachcoupler);
                                                coupler.AutomaticCouplingAllowed = false;
                                                othercoupler.AutomaticCouplingAllowed = false;
                                        }
                                }
                        }
                }
        }

            if (coupler.CouplingFlag != coupling::faux) {
                // uzgadnianie prawa Newtona
                othervehicle->Couplers[1 - End].CForce = -CF;
        }

        return CF;
    }

    // *************************************************************************************************
    // Q: 20160714
    // oblicza sile trakcyjna lokomotywy (dla elektrowozu tez calkowity prad)
    // *************************************************************************************************
    double TMoverParameters::TractionForce(double dt) {
        double PosRatio, dmoment, dtrans, tmp;

        Ft = 0;
        dtrans = 0;
        dmoment = 0;
            // youBy
            switch (EngineType) {
                    case TEngineType::DieselElectric: {
                            if ((true == Mains) && (true == FuelPump.is_active)) {

                                if (EIMCtrlType > 0) // sterowanie cyfrowe
                                    tmp = (DElist[0].RPM +
                                           ((DElist[MainCtrlPosNo].RPM - DElist[0].RPM) * std::max(0.0, eimic_real))) /
                                          60.0;
                                else
                                    tmp = DElist[(ControlPressureSwitch ? MainCtrlNoPowerPos() : MainCtrlPos)].RPM /
                                          60.0;

                                    if ((true == HeatingAllow) && (HeatingPower > 0) && (EngineHeatingRPM > 0)) {
                                        // bump engine revolutions up if needed, when heating is on
                                        tmp = std::max(tmp, std::min(EngineMaxRPM(), EngineHeatingRPM) / 60.0);
                                }
                                // NOTE: fake dizel_fill calculation for the sake of smoke emitter which uses this
                                // parameter to determine smoke opacity
                                dizel_fill = clamp(0.2 + 0.35 * (tmp - enrot) +
                                                           0.5 * (std::fabs(Im) / DElist[MainCtrlPosNo].Imax),
                                                   0.05, 1.0);
                            } else {
                                tmp = 0.0;
                                dizel_fill = 0.0;
                            }

                            if (enrot != tmp) {
                                enrot = clamp(
                                        enrot + (dt / dizel_AIM) *
                                                        (enrot < tmp
                                                                 ? 1.0
                                                                 : -1.0 * dizel_RevolutionsDecreaseRate), // NOTE:
                                                                                                          // revolutions
                                                                                                          // typically
                                                                                                          // drop faster
                                                                                                          // than they
                                                                                                          // rise
                                        0.0, std::max(tmp, enrot));
                                    if (std::fabs(tmp - enrot) < 0.001) {
                                        enrot = tmp;
                                }
                        }
                        break;
                    }
                    case TEngineType::DieselEngine: {
                        if (ShuntMode) // dodatkowa przekładnia np. dla 2Ls150
                            dtrans = AnPos * Transmision.Ratio * MotorParam[ScndCtrlActualPos].mIsat;
                        else
                            dtrans = Transmision.Ratio * MotorParam[ScndCtrlActualPos].mIsat;

                        dmoment = dizel_Momentum(dizel_fill, dtrans * nrot * DirActive, dt); // oblicza tez enrot
                        break;
                    }
                    default: {
                        enrot = Transmision.Ratio * nrot;
                        break;
                    }
            }

        eAngle += enrot * dt;
        if (eAngle > M_PI * 2.0)
            eAngle = std::fmod(eAngle, M_PI * 2.0);
            /*
               while (eAngle > M_PI * 2.0)
            // eAngle = Pirazy2 - eAngle; <- ABu: a nie czasem tak, jak nizej?
            eAngle -= M_PI * 2.0;
            */
            // hunter-091012: przeniesione z if DirActive<>0 (zeby po zejsciu z kierunku dalej spadala predkosc
            // wentylatorow) wentylatory rozruchowe TBD, TODO: move this to update, it doesn't exactly have much to do
            // with traction
            switch (EngineType) {

                    case TEngineType::ElectricSeriesMotor: {
                            if (true == Mains) {
                                    switch (RVentType) {

                                            case 1: { // manual
                                                    if ((true == RVentForceOn) ||
                                                        ((DirActive != 0) &&
                                                         (RList[MainCtrlActualPos].R > RVentCutOff))) {
                                                        RventRot += (RVentnmax - RventRot) * RVentSpeed * dt;
                                                    } else {
                                                        RventRot *= std::max(0.0, 1.0 - RVentSpeed * dt);
                                                    }
                                                break;
                                            }

                                            case 2: { // automatic
                                                auto const motorcurrent{std::min<double>(ImaxHi, std::fabs(Im))};
                                                    if ((std::fabs(Itot) > RVentMinI) &&
                                                        (RList[MainCtrlActualPos].R > RVentCutOff)) {

                                                        RventRot +=
                                                                (RVentnmax *
                                                                         std::min(1.0, ((motorcurrent / NPoweredAxles) /
                                                                                        RVentMinI)) *
                                                                         motorcurrent / ImaxLo -
                                                                 RventRot) *
                                                                RVentSpeed * dt;
                                                    } else if ((DynamicBrakeType == dbrake_automatic) &&
                                                               (true == DynamicBrakeFlag)) {
                                                        RventRot += (RVentnmax * motorcurrent / ImaxLo - RventRot) *
                                                                    RVentSpeed * dt;
                                                    } else if (RVentForceOn) {
                                                        RventRot += (RVentnmax - RventRot) * RVentSpeed * dt;
                                                    } else {
                                                        RventRot *= std::max(0.0, 1.0 - RVentSpeed * dt);
                                                    }
                                                break;
                                            }

                                            default: {
                                                break;
                                            }
                                    } // rventtype
                            } // mains
                            else {
                                RventRot *= std::max(0.0, 1.0 - RVentSpeed * dt);
                            }
                        break;
                    }

                    case TEngineType::DieselElectric: {
                            // NOTE: for this type RventRot is the speed of motor blowers; we also update radiator fans
                            // while at it
                            if (true == Mains) {
                                // TBD, TODO: currently ignores RVentType, fix this?
                                RventRot += clamp(enrot - RventRot, -100.0, 50.0) * dt;
                                dizel_heat.rpmw += clamp(dizel_heat.rpmwz - dizel_heat.rpmw, -100.f, 50.f) * dt;
                                dizel_heat.rpmw2 += clamp(dizel_heat.rpmwz2 - dizel_heat.rpmw2, -100.f, 50.f) * dt;
                            } else {
                                RventRot *= std::max(0.0, 1.0 - RVentSpeed * dt);
                                dizel_heat.rpmw *= std::max(0.0, 1.0 - dizel_heat.rpmw * dt);
                                dizel_heat.rpmw2 *= std::max(0.0, 1.0 - dizel_heat.rpmw2 * dt);
                            }
                        break;
                    }

                    case TEngineType::DieselEngine: {
                            // NOTE: we update only radiator fans, as vehicles with diesel engine don't have other
                            // ventilators
                            if (true == Mains) {
                                dizel_heat.rpmw += clamp(dizel_heat.rpmwz - dizel_heat.rpmw, -100.f, 50.f) * dt;
                                dizel_heat.rpmw2 += clamp(dizel_heat.rpmwz2 - dizel_heat.rpmw2, -100.f, 50.f) * dt;
                            } else {
                                dizel_heat.rpmw *= std::max(0.0, 1.0 - dizel_heat.rpmw * dt);
                                dizel_heat.rpmw2 *= std::max(0.0, 1.0 - dizel_heat.rpmw2 * dt);
                            }
                        break;
                    }

                    default: {
                        break;
                    }
            }

            switch (EngineType) {
                    case TEngineType::Dumb: {
                        PosRatio = (MainCtrlPos + ScndCtrlPos) / (MainCtrlPosNo + ScndCtrlPosNo + 0.01);
                        EnginePower = /*1000.0 **/ Power * PosRatio;
                        break;
                    }
                    case TEngineType::DieselEngine: {
                        dizel_Power = Mm * enrot * (2.0 * M_PI / 1000.0);
                        EnginePower = (dizel_Mstand + Mm) * enrot * (2.0 * M_PI / 1000.0);
                            if (MainCtrlPowerPos() > 1) {
                                // dodatkowe opory z powodu sprezarki}
                                //                dmoment -= dizel_Mstand * ( 0.2 * enrot / dizel_nmax ); //yB: skąd to
                                //                w ogóle się bierze?!
                        }
                        break;
                    }
                    case TEngineType::DieselElectric: {
                        EnginePower = 0; // the actual calculation is done in two steps later in the method
                        break;
                    }
                    default: {
                        break;
                    }
            }

            switch (EngineType) {

                    case TEngineType::ElectricSeriesMotor: {
                        // update the state of voltage relays
                        auto const voltage{std::max(GetTrainsetHighVoltage(), PantographVoltage)};
                        NoVoltRelay = (EnginePowerSource.SourceType != TPowerSource::CurrentCollector) ||
                                      (voltage >= EnginePowerSource.CollectorParameters.MinV);
                        OvervoltageRelay = (EnginePowerSource.SourceType != TPowerSource::CurrentCollector) ||
                                           (voltage <= EnginePowerSource.CollectorParameters.MaxV) ||
                                           (false == EnginePowerSource.CollectorParameters.OVP);
                        // wywalanie szybkiego z powodu niewłaściwego napięcia
                        EventFlag |= ((true == Mains) && ((false == NoVoltRelay) || (false == OvervoltageRelay)) &&
                                      (MainSwitch(false, (TrainType == dt_EZT
                                                                  ? range_t::unit
                                                                  : range_t::local)))); // TODO: check whether we need
                                                                                        // to send this EMU-wide
                        break;
                    }

                    case TEngineType::ElectricInductionMotor: {
                            // TODO: check if we can use instead the code for electricseriesmotor
                            if ((Mains)) {
                                    // nie wchodzić w funkcję bez potrzeby
                                    if ((std::max(GetTrainsetHighVoltage(), PantographVoltage) <
                                         EnginePowerSource.CollectorParameters.MinV) ||
                                        (std::max(GetTrainsetHighVoltage(), PantographVoltage) >
                                         EnginePowerSource.CollectorParameters.MaxV + 200)) {
                                        MainSwitch(false, (TrainType == dt_EZT
                                                                   ? range_t::unit
                                                                   : range_t::local)); // TODO: check whether we need to
                                                                                       // send this EMU-wide
                                }
                        }
                        break;
                    }

                    case TEngineType::DieselElectric: {
                        // TODO: move this to the auto relay check when the electric engine code paths are unified
                        StLinFlag |= ((Mains) && (false == StLinFlag) && (MainCtrlPowerPos() == 1));
                        StLinFlag &= MotorConnectorsCheck();
                        StLinFlag &= (MainCtrlPowerPos() > 0);

                        break;
                    }

                    default: {
                        break;
                    }
            }

        if (DirActive != 0)
                switch (EngineType) {
                        case TEngineType::Dumb: {
                                if (Mains && (CabActive != 0)) {
                                        if (Vel > 0.1) {
                                            Ft = std::min(1000.0 * Power / std::fabs(V), Ftmax) * PosRatio;
                                        } else {
                                            Ft = Ftmax * PosRatio;
                                        }
                                    Ft = Ft * DirAbsolute; // DirActive*CabActive;
                                } else {
                                    Ft = 0;
                                }
                            break;
                        } // Dumb

                        case TEngineType::WheelsDriven: {
                            if (EnginePowerSource.SourceType == TPowerSource::InternalSource)
                                if (EnginePowerSource.PowerType == TPowerType::BioPower)
                                    Ft = Sign(sin(eAngle)) * PulseForce * Transmision.Ratio;
                            PulseForceTimer = PulseForceTimer + dt;
                                if (PulseForceTimer > CtrlDelay) {
                                    PulseForce = 0;
                                    if (PulseForceCount > 0)
                                        PulseForceCount--;
                            }
                            EnginePower = Ft * (1.0 + Vel);
                            break;
                        } // WheelsDriven

                        case TEngineType::ElectricSeriesMotor: {
                            //        enrot:=Transmision.Ratio*nrot;
                            // yB: szereg dwoch sekcji w ET42
                            if ((TrainType == dt_ET42) && (Imax == ImaxHi))
                                EngineVoltage = EngineVoltage / 2.0;
                            Mm = Momentum(Current(enrot, EngineVoltage)); // oblicza tez prad p/slinik

                                if (TrainType == dt_ET42) {
                                    if (Imax == ImaxHi)
                                        EngineVoltage = EngineVoltage * 2;
                                    if ((DynamicBrakeFlag) && (fabs(Im) > 300)) // przeiesione do mover.cpp
                                        FuseOff();
                            }
                                if ((DynamicBrakeType == dbrake_automatic) && (DynamicBrakeFlag)) {
                                        if (TUHEX_Stages > 0) // hamowanie wielostopniowe, nadpisuje wartości domyślne
                                        {
                                            if (Vel > 100)
                                                TUHEX_StageActual = std::min(TUHEX_StageActual, 1);
                                                switch (TUHEX_StageActual) {
                                                    case 1:
                                                        TUHEX_Sum = TUHEX_Sum1;
                                                        DynamicBrakeRes = DynamicBrakeRes1;
                                                        break;
                                                    case 2:
                                                        TUHEX_Sum = TUHEX_Sum2;
                                                        DynamicBrakeRes = DynamicBrakeRes1;
                                                        break;
                                                    case 3:
                                                        TUHEX_Sum = TUHEX_Sum3;
                                                        if ((Vadd > 0.99 * TUHEX_MaxIw) &&
                                                            (DynamicBrakeRes == DynamicBrakeRes1))
                                                            TUHEX_ResChange = true;
                                                            if (TUHEX_ResChange && Vadd < 0.5 * TUHEX_MaxIw) {
                                                                TUHEX_ResChange = false;
                                                                DynamicBrakeRes = DynamicBrakeRes2;
                                                        }
                                                        break;
                                                    default:
                                                        DynamicBrakeRes = DynamicBrakeRes1;
                                                        TUHEX_Sum = 0;
                                                        break;
                                                }
                                    }
                                        if (((Vadd + fabs(Im)) > TUHEX_Sum + TUHEX_Diff) ||
                                            (Hamulec->GetEDBCP() < 0.25) || (TUHEX_ResChange) ||
                                            (TUHEX_StageActual == 0 && TUHEX_Stages > 0)) {
                                            Vadd -= 500.0 * dt;
                                                if (Vadd < TUHEX_MinIw) {
                                                    Vadd = 0;
                                                    DynamicBrakeFlag = false;
                                            }
                                        } else if ((DynamicBrakeFlag) && ((Vadd + fabs(Im)) < TUHEX_Sum - TUHEX_Diff)) {
                                            Vadd += 70.0 * dt;
                                            Vadd = Min0R(Max0R(Vadd, TUHEX_MinIw), TUHEX_MaxIw);
                                    }
                                    if (Vadd > 0)
                                        Mm = MomentumF(Im, Vadd, 0);
                            }

                            if ((TrainType == dt_ET22) && (DelayCtrlFlag)) // szarpanie przy zmianie układu w byku
                                Mm = Mm * RList[MainCtrlActualPos].Bn /
                                     (RList[MainCtrlActualPos].Bn +
                                      1); // zrobione w momencie, żeby nie dawac elektryki w przeliczaniu sił

                            if (fabs(Im) > Imax)
                                Vhyp += dt; //*(fabs(Im) / Imax - 0.9) * 10; // zwieksz czas oddzialywania na PN
                            else
                                Vhyp = 0;
                            if (Vhyp > CtrlDelay / 2) // jesli czas oddzialywania przekroczony
                                FuseOff();            // wywalanie bezpiecznika z powodu przetezenia silnikow

                            if (((DynamicBrakeType == dbrake_automatic) || (DynamicBrakeType == dbrake_switch)) &&
                                (DynamicBrakeFlag))
                                Itot = Im * 2; // 2x2 silniki w EP09
                            else if ((TrainType == dt_EZT) && (Imin == IminLo) &&
                                     (ScndS)) // yBARC - boczniki na szeregu poprawnie
                                Itot = Im;
                            else
                                Itot = Im * RList[MainCtrlActualPos].Bn; // prad silnika * ilosc galezi
                            Mw = Mm * Transmision.Ratio * Transmision.Efficiency;
                            Fw = Mw * 2.0 / WheelDiameter;
                            Ft = Fw * NPoweredAxles; // sila trakcyjna
                            break;
                        }

                        case TEngineType::DieselEngine: {
                            Mw = dmoment * dtrans *
                                 Transmision.Efficiency; // dmoment i dtrans policzone przy okazji enginerotation
                            if ((hydro_R) && (hydro_R_Placement == 0))
                                Mw -= dizel_MomentumRetarder(nrot * Transmision.Ratio, dt) * Transmision.Ratio *
                                      Transmision.Efficiency;
                            Fw = Mw * 2.0 / WheelDiameter / NPoweredAxles;
                            Ft = Fw * NPoweredAxles; // sila trakcyjna
                            Ft = Ft * DirAbsolute;   // DirActive*CabActive;
                            break;
                        }

                        case TEngineType::DieselElectric: // youBy
                        {
                            //       tmpV:=V*CabActive*DirActive;
                            auto const tmpV{nrot * Pirazy2 * 0.5 * WheelDiameter * DirAbsolute}; //*CabActive*DirActive;
                            auto tempUmax = 0.0;
                            auto tempImax = 0.0;
                            auto tempPmax = 0.0;
                                // jazda manewrowa
                                if (EIMCtrlType > 0) // sterowanie cyfrowe
                                {
                                    auto eimic_positive = std::max(0.0, eimic_real);
                                    auto const rpmratio{EngineRPMRatio()};
                                    tempImax = DElist[MainCtrlPosNo].Imax * eimic_positive;
                                    tempUmax = DElist[MainCtrlPosNo].Umax * std::min(eimic_positive, rpmratio);
                                    tempPmax = DElist[MainCtrlPosNo].GenPower * std::min(eimic_positive, rpmratio);
                                    tmp = tempPmax;
                                        if (true == StLinFlag) {

                                                if (tmpV < (Vhyp * tempPmax / DElist[MainCtrlPosNo].GenPower)) {
                                                    // czy na czesci prostej, czy na hiperboli
                                                    Ft = (Ftmax - ((Ftmax - 1000.0 * DElist[MainCtrlPosNo].GenPower /
                                                                                    (Vhyp + Vadd)) *
                                                                   (tmpV / Vhyp) / PowerCorRatio)) *
                                                         eimic_positive; // posratio - bo sila jakos tam sie rozklada
                                                } else {
                                                    // na hiperboli
                                                    // 1.107 - wspolczynnik sredniej nadwyzki Ft w symku nad
                                                    // charakterystyka
                                                    Ft = 1000.0 * tempPmax / (tmpV + Vadd) /
                                                         PowerCorRatio; // tu jest zawarty stosunek mocy
                                                }
                                    } else
                                        Ft = 0; // jak nastawnik na zero, to sila tez zero

                                    PosRatio = tempPmax / DElist[MainCtrlPosNo].GenPower;
                                } else if (true == ShuntMode) {
                                        if (true == StLinFlag) {
                                            EngineVoltage = (SST[MainCtrlPos].Umax * AnPos) +
                                                            (SST[MainCtrlPos].Umin * (1.0 - AnPos));
                                            // NOTE: very crude way to approximate power generated at current rpm
                                            // instead of instant top output NOTE, TODO: doesn't take into account
                                            // potentially increased revolutions if heating is on, fix it
                                            tmp = EngineRPMRatio() * (SST[MainCtrlPos].Pmax * AnPos) +
                                                  (SST[MainCtrlPos].Pmin * (1.0 - AnPos));
                                            Ft = tmp * 1000.0 / (fabs(tmpV) + 1.6);
                                        } else {
                                            EngineVoltage = 0;
                                            Ft = 0;
                                        }
                                    PosRatio = 1;
                                } else // jazda ciapongowa
                                {
                                    auto power = Power;
                                    tempImax = DElist[MainCtrlPos].Imax;
                                    tempUmax = DElist[MainCtrlPos].Umax;
                                    tempPmax = DElist[MainCtrlPos].GenPower;
                                        if (true == Heating) {
                                            power -= HeatingPower;
                                    }
                                        if (power < 0.0) {
                                            power = 0.0;
                                    }
                                    // NOTE: very crude way to approximate power generated at current rpm instead of
                                    // instant top output NOTE, TODO: doesn't take into account potentially increased
                                    // revolutions if heating is on, fix it
                                    auto const currentgenpower{
                                            (DElist[MainCtrlPos].RPM > 0
                                                     ? DElist[MainCtrlPos].GenPower *
                                                               (60.0 * enrot / DElist[MainCtrlPos].RPM)
                                                     : 0.0)};

                                    tmp = std::min(power, currentgenpower);

                                    PosRatio = currentgenpower / DElist[MainCtrlPosNo].GenPower;
                                        // stosunek mocy teraz do mocy max
                                        // NOTE: Mains in this context is working diesel engine
                                        if (true == StLinFlag) {

                                                if (tmpV < (Vhyp * power / DElist[MainCtrlPosNo].GenPower)) {
                                                    // czy na czesci prostej, czy na hiperboli
                                                    Ft = (Ftmax - ((Ftmax - 1000.0 * DElist[MainCtrlPosNo].GenPower /
                                                                                    (Vhyp + Vadd)) *
                                                                   (tmpV / Vhyp) / PowerCorRatio)) *
                                                         PosRatio; // posratio - bo sila jakos tam sie rozklada
                                                } else {
                                                    // na hiperboli
                                                    // 1.107 - wspolczynnik sredniej nadwyzki Ft w symku nad
                                                    // charakterystyka
                                                    Ft = 1000.0 * tmp / (tmpV + Vadd) /
                                                         PowerCorRatio; // tu jest zawarty stosunek mocy
                                                }
                                    } else
                                        Ft = 0; // jak nastawnik na zero, to sila tez zero

                                    PosRatio = tmp / DElist[MainCtrlPosNo].GenPower;
                                }

                            if (FuseFlag)
                                Ft = 0;
                            else
                                Ft = Ft * DirAbsolute;     // DirActive * CabActive; //zwrot sily i jej wartosc
                            Fw = Ft / NPoweredAxles;       // sila na obwodzie kola
                            Mw = Fw * WheelDiameter / 2.0; // moment na osi kola
                            Mm = Mw / Transmision.Ratio;   // moment silnika trakcyjnego

                            // with MotorParam[ScndCtrlPos] do
                            if (fabs(Mm) > MotorParam[ScndCtrlPos].fi)
                                Im = NPoweredAxles *
                                     fabs(fabs(Mm) / MotorParam[ScndCtrlPos].mfi + MotorParam[ScndCtrlPos].mIsat);
                            else
                                Im = NPoweredAxles * sqrt(fabs(Mm * MotorParam[ScndCtrlPos].Isat));

                                if (ShuntMode) {
                                    EnginePower = EngineVoltage * Im / 1000.0;
                                        if (EnginePower > tmp) {
                                            EnginePower = tmp;
                                            EngineVoltage = EnginePower * 1000.0 / Im;
                                    }
                                        if (EnginePower < tmp) {
                                            Ft *= EnginePower / tmp;
                                    }
                                } else {
                                        if (fabs(Im) >
                                            tempImax) { // nie ma nadmiarowego, tylko Imax i zwarcie na pradnicy
                                            Ft = Ft / Im * tempImax;
                                            Im = tempImax;
                                    }

                                        if (Im > 0) {
                                                // jak pod obciazeniem
                                                if (true == Flat) {
                                                    // ograniczenie napiecia w pradnicy - plaszczak u gory
                                                    EngineVoltage = 1000.0 * tmp / std::fabs(Im);
                                                } else {
                                                    auto tempMCP = EIMCtrlType > 0 ? 1 + 99 * std::max(1.0, eimic_real)
                                                                                   : MainCtrlPos;
                                                    auto tempMCPN = EIMCtrlType > 0 ? 100 : MainCtrlPosNo;
                                                    // charakterystyka pradnicy obcowzbudnej (elipsa) - twierdzenie
                                                    // Pitagorasa
                                                    EngineVoltage =
                                                            std::sqrt(std::fabs(square(tempUmax) -
                                                                                square(tempUmax * Im / tempImax))) *
                                                                    (tempMCP - 1) +
                                                            (1.0 - Im / tempImax) * tempUmax * (tempMCPN - tempMCP);
                                                    EngineVoltage /= (tempMCPN - 1);
                                                    EngineVoltage = clamp(EngineVoltage, Im * 0.05,
                                                                          (1000.0 * tmp / std::fabs(Im)));
                                                }
                                    }

                                        if ((EngineVoltage > tempUmax) || (Im == 0)) {
                                            // gdy wychodzi za duze napiecie albo przy biegu jalowym (jest cos takiego?)
                                            EngineVoltage = tempUmax * (ConverterFlag ? 1 : 0);
                                    }

                                    EnginePower = EngineVoltage * Im / 1000.0;
                                    /*
                                    // power curve drop
                                    // NOTE: disabled for the time being due to side-effects
                                    if( ( tmpV > 1 ) && ( EnginePower < tmp ) ) {
                                    Ft = interpolate(
                                    Ft, EnginePower / tmp,
                                    clamp( tmpV - 1.0, 0.0, 1.0 ) );
                                    }
                                    */
                                }

                            if ((Imax > 1) && (Im > Imax))
                                FuseOff();
                            if (FuseFlag)
                                EngineVoltage = 0;

                                // przekazniki bocznikowania, kazdy inny dla kazdej pozycji
                                if ((false == StLinFlag) || (ShuntMode)) {
                                    ScndCtrlPos = 0;
                                } else {
                                        if (AutoRelayFlag) {

                                            auto const shuntfieldstate{ScndCtrlPos};

                                                switch (RelayType) {

                                                        case 0: {

                                                                if ((ScndCtrlPos < ScndCtrlPosNo) &&
                                                                    (Im <= (MPTRelay[ScndCtrlPos].Iup * PosRatio))) {
                                                                    ++ScndCtrlPos;
                                                            }
                                                                if ((ScndCtrlPos > 0) &&
                                                                    (Im >= (MPTRelay[ScndCtrlPos].Idown * PosRatio))) {
                                                                    --ScndCtrlPos;
                                                            }
                                                            break;
                                                        }
                                                        case 1: {

                                                                if ((ScndCtrlPos < ScndCtrlPosNo) &&
                                                                    (MPTRelay[ScndCtrlPos].Iup < Vel)) {
                                                                    ++ScndCtrlPos;
                                                            }
                                                                if ((ScndCtrlPos > 0) &&
                                                                    (MPTRelay[ScndCtrlPos].Idown > Vel)) {
                                                                    --ScndCtrlPos;
                                                            }
                                                            break;
                                                        }
                                                        case 2: {

                                                                if ((ScndCtrlPos < ScndCtrlPosNo) &&
                                                                    (MPTRelay[ScndCtrlPos].Iup < Vel) &&
                                                                    (EnginePower < (tmp * 0.99))) {
                                                                    ++ScndCtrlPos;
                                                            }
                                                                if ((ScndCtrlPos > 0) &&
                                                                    (MPTRelay[ScndCtrlPos].Idown < Im)) {
                                                                    --ScndCtrlPos;
                                                            }
                                                            break;
                                                        }
                                                        case 41: {
                                                                if ((ScndCtrlPos < ScndCtrlPosNo) &&
                                                                    (MainCtrlPos == MainCtrlPosNo) &&
                                                                    (tmpV * 3.6 > MPTRelay[ScndCtrlPos].Iup)) {
                                                                    ++ScndCtrlPos;
                                                                    enrot = enrot * 0.73;
                                                            }
                                                                if ((ScndCtrlPos > 0) &&
                                                                    (Im > MPTRelay[ScndCtrlPos].Idown)) {
                                                                    --ScndCtrlPos;
                                                            }
                                                            break;
                                                        }
                                                        case 45: {
                                                                if ((ScndCtrlPos < ScndCtrlPosNo) &&
                                                                    (MainCtrlPos >= 11)) {

                                                                        if (Im < MPTRelay[ScndCtrlPos].Iup) {
                                                                            ++ScndCtrlPos;
                                                                    }
                                                                        // check for cases where the speed drops below
                                                                        // threshold for level 2 or 3
                                                                        if ((ScndCtrlPos > 1) &&
                                                                            (Vel < MPTRelay[ScndCtrlPos - 1].Idown)) {
                                                                            --ScndCtrlPos;
                                                                    }
                                                            }
                                                                // malenie
                                                                if ((ScndCtrlPos > 0) && (MainCtrlPos < 11)) {

                                                                        if (ScndCtrlPos == 1) {
                                                                                if (Im >
                                                                                    MPTRelay[ScndCtrlPos - 1].Idown) {
                                                                                    --ScndCtrlPos;
                                                                            }
                                                                        } else {
                                                                                if (Vel < MPTRelay[ScndCtrlPos].Idown) {
                                                                                    --ScndCtrlPos;
                                                                            }
                                                                        }
                                                            }
                                                                // 3rd level drops with master controller at position
                                                                // lower than 10...
                                                                if (MainCtrlPos < 11) {
                                                                    ScndCtrlPos = std::min(2, ScndCtrlPos);
                                                            }
                                                                // ...and below position 7 field shunt drops altogether
                                                                if (MainCtrlPos < 8) {
                                                                    ScndCtrlPos = 0;
                                                            }
                                                            /*
                                                            // crude woodward approximation; difference between rpm for
                                                            consecutive positions is ~5%
                                                            // so we get full throttle until ~half way between desired
                                                            and previous position, or zero on rpm reduction auto const
                                                            woodward { clamp( ( DElist[ MainCtrlPos ].RPM / ( enrot
                                                            * 60.0 ) - 1.0 ) * 50.0, 0.0, 1.0 ) };
                                                            */
                                                            break;
                                                        }
                                                        case 46: {
                                                                // wzrastanie
                                                                if ((MainCtrlPos >= 12) &&
                                                                    (ScndCtrlPos < ScndCtrlPosNo)) {
                                                                        if ((ScndCtrlPos) % 2 == 0) {
                                                                                if ((MPTRelay[ScndCtrlPos].Iup > Im)) {
                                                                                    ++ScndCtrlPos;
                                                                            }
                                                                        } else {
                                                                                if ((MPTRelay[ScndCtrlPos - 1].Iup >
                                                                                     Im) &&
                                                                                    (MPTRelay[ScndCtrlPos].Iup < Vel)) {
                                                                                    ++ScndCtrlPos;
                                                                            }
                                                                        }
                                                            }
                                                                // malenie
                                                                if ((MainCtrlPos < 12) && (ScndCtrlPos > 0)) {
                                                                        if (Vel < 50.0) {
                                                                                // above 50 km/h already active shunt
                                                                                // field can be maintained until lower
                                                                                // controller setting
                                                                                if ((ScndCtrlPos) % 2 == 0) {
                                                                                        if ((MPTRelay[ScndCtrlPos]
                                                                                                     .Idown < Im)) {
                                                                                            --ScndCtrlPos;
                                                                                    }
                                                                                } else {
                                                                                        if ((MPTRelay[ScndCtrlPos + 1]
                                                                                                     .Idown < Im) &&
                                                                                            (MPTRelay[ScndCtrlPos]
                                                                                                     .Idown > Vel)) {
                                                                                            --ScndCtrlPos;
                                                                                    }
                                                                                }
                                                                    }
                                                            }
                                                                if (MainCtrlPos < 11) {
                                                                    ScndCtrlPos = std::min(2, ScndCtrlPos);
                                                            }
                                                                if (MainCtrlPos < 8) {
                                                                    ScndCtrlPos = 0;
                                                            }
                                                            break;
                                                        }
                                                        default: {
                                                            break;
                                                        }
                                                } // switch RelayType

                                                if (ScndCtrlPos != shuntfieldstate) {
                                                    SetFlag(SoundFlag, (sound::relay | sound::shuntfield));
                                            }
                                    }
                                }
                            break;
                        } // DieselElectric

                        case TEngineType::ElectricInductionMotor: {
                                if (true == Mains && !SecuritySystem.is_engine_blocked()) {
                                    double ActiveInverters = 0.0;
                                        for (auto &inv: Inverters) {
                                            if (inv.IsActive)
                                                ActiveInverters += 1.0;
                                        }
                                    InvertersRatio = ActiveInverters / (double)InvertersNo;
                                        if (EIM_Pmax_Table.size() > 1) {
                                            eimc[eimc_p_Pmax] = TableInterpolation(EIM_Pmax_Table, Vel);
                                    }
                                        // tempomat
                                        if (ScndCtrlPosNo == 4 && SpeedCtrlTypeTime) {
                                            SpeedCtrlUnit.IsActive = (SpeedCtrlValue > 0);
                                                switch (ScndCtrlPos) {
                                                    case 0:
                                                        NewSpeed = 0;
                                                        SpeedCtrlValue = 0;
                                                        SpeedCtrlTimer = 10;
                                                        break;
                                                    case 1:
                                                            if (SpeedCtrlTimer > SpeedCtrlDelay) {
                                                                SpeedCtrlTimer = 0;
                                                                NewSpeed -= 10;
                                                                if (NewSpeed < 0)
                                                                    NewSpeed = 0;
                                                        } else
                                                            SpeedCtrlTimer += dt;
                                                        break;
                                                    case 2:
                                                        SpeedCtrlTimer = 10;
                                                        SpeedCtrlValue = NewSpeed;
                                                        break;
                                                    case 3:
                                                            if (SpeedCtrlTimer > SpeedCtrlDelay) {
                                                                SpeedCtrlTimer = 0;
                                                                NewSpeed += 10;
                                                                if (NewSpeed > Vmax)
                                                                    NewSpeed = Vmax;
                                                        } else
                                                            SpeedCtrlTimer += dt;
                                                        break;
                                                    case 4:
                                                        NewSpeed = Vmax;
                                                        SpeedCtrlValue = Vmax;
                                                        SpeedCtrlTimer = 10;
                                                        break;
                                                }
                                        } else if (ScndCtrlPosNo > 1) {
                                                if (ScndCtrlPos != NewSpeed) {

                                                    SpeedCtrlTimer = 0;
                                                    NewSpeed = ScndCtrlPos;
                                                } else {
                                                    SpeedCtrlTimer += dt;
                                                        if (SpeedCtrlTimer > SpeedCtrlDelay) {
                                                            int NewSCAP =
                                                                    (float)ScndCtrlPos / (float)ScndCtrlPosNo * Vmax;
                                                                if (NewSCAP != SpeedCtrlValue) {
                                                                    SpeedCtrlValue = NewSCAP;
                                                                    //								SendCtrlToNext("SpeedCntrl",
                                                                    // SpeedCtrlValue, CabActive);
                                                            }
                                                    }
                                                }
                                            SpeedCtrlUnit.IsActive = (SpeedCtrlValue > 0);
                                    }
                                    double edBCP = Hamulec->GetEDBCP();
                                    auto const localbrakeactive{(CabOccupied != 0) && (LocHandle->GetCP() > 0.25)};
                                        if ((false == Doors.instances[side::left].is_closed) ||
                                            (false == Doors.instances[side::right].is_closed) ||
                                            (Doors.permit_needed && (Doors.instances[side::left].open_permit ||
                                                                     Doors.instances[side::right].open_permit))) {
                                            DynamicBrakeFlag = true;
                                    } else if (((edBCP < 0.25) && (false == localbrakeactive) && (AnPos < 0.01)) ||
                                               ((edBCP < 0.25) && (ShuntModeAllow) && (LocalBrakePosA < 0.01)))
                                        DynamicBrakeFlag = false;
                                    else if ((((BrakePress > 0.25) && (edBCP > 0.25) || localbrakeactive)) ||
                                             (AnPos > 0.02))
                                        DynamicBrakeFlag = true;
                                    edBCP = Hamulec->GetEDBCP() * eimc[eimc_p_abed]; // stala napedu
                                        if ((DynamicBrakeFlag)) {
                                                // ustalanie współczynnika blendingu do luzowania hamulca PN
                                                if (eimv[eimv_Fmax] * Sign(V) * DirAbsolute < -1) {
                                                    PosRatio = -Sign(V) * DirAbsolute * eimv[eimv_Fr] /
                                                               (eimc[eimc_p_Fh] *
                                                                Max0R(edBCP, Max0R(0.01, Hamulec->GetEDBCP())) /
                                                                MaxBrakePress[0]);
                                                    PosRatio = clamp(PosRatio, 0.0, 1.0);
                                                } else {
                                                    PosRatio = 0;
                                                }
                                            PosRatio = Round(20.0 * PosRatio) / 20.0; // stopniowanie PN/ED
                                            if (PosRatio < 19.5 / 20.0)
                                                PosRatio *= 0.9;
                                            Hamulec->SetED(Max0R(
                                                    0.0, std::min(PosRatio,
                                                                  1.0))); // ustalenie stopnia zmniejszenia ciśnienia
                                                                          //  ustalanie siły hamowania ED
                                                if ((Hamulec->GetEDBCP() > 0.25) && (eimc[eimc_p_abed] < 0.001) ||
                                                    (ActiveInverters < InvertersNo)) // jeśli PN wyłącza ED
                                                {
                                                    PosRatio = 0;
                                                    eimv[eimv_Fzad] = 0;
                                                } else {
                                                    PosRatio = -std::max(std::min(edBCP * 1.0 / MaxBrakePress[0], 1.0),
                                                                         AnPos) *
                                                               std::max(0.0, std::min(1.0, (Vel - eimc[eimc_p_Vh0]) /
                                                                                                   (eimc[eimc_p_Vh1] -
                                                                                                    eimc[eimc_p_Vh0])));
                                                    eimv[eimv_Fzad] = -std::min(
                                                            1.0, std::max(LocalBrakeRatio(), edBCP / MaxBrakePress[0]));
                                                }
                                            tmp = 5;
                                        } else {
                                            PosRatio = Max0R(eimic_real, 0);
                                            eimv[eimv_Fzad] = PosRatio;
                                            if ((Flat) && (eimc[eimc_p_F0] * eimv[eimv_Fful] > 0))
                                                PosRatio = Min0R(PosRatio * eimc[eimc_p_F0] / eimv[eimv_Fful], 1);
                                            /*                    if (ScndCtrlActualPos > 0) //speed control
                                                                  if (Vmax < 250)
                                                                  PosRatio = Min0R(PosRatio, Max0R(-1, 0.5 *
                                               (ScndCtrlActualPos - Vel))); else PosRatio = Min0R(PosRatio, Max0R(-1,
                                               0.5 * (ScndCtrlActualPos * 2 - Vel))); */
                                            // PosRatio = 1.0 * (PosRatio * 0 + 1) * PosRatio; // 1 * 1 * PosRatio =
                                            // PosRatio
                                            Hamulec->SetED(0);
                                            //           (Hamulec as TLSt).SetLBP(LocBrakePress);
                                            if ((PosRatio > eimv_pr))
                                                tmp = 4;
                                            else
                                                tmp = 4; // szybkie malenie, powolne wzrastanie
                                        }
                                    dmoment = eimv[eimv_Fful];
                                        // NOTE: the commands to operate the sandbox are likely to conflict with other
                                        // similar ai decisions
                                        // TODO: gather these in single place so they can be resolved together
                                        if ((SlippingWheels)) {
                                            PosRatio = 0;
                                            tmp = 10;
                                            SandboxAuto(true, range_t::unit);
                                        } // przeciwposlizg
                                        else {
                                            // switch sandbox off
                                            SandboxAuto(false, range_t::unit);
                                        }
                                        if (ActiveInverters == 0.0) {
                                            PosRatio = 0;
                                            eimv_pr = 0;
                                    }

                                    eimv_pr += Max0R(Min0R(PosRatio - eimv_pr, 0.02), -0.02) * 12 *
                                               (tmp /*2{+4*byte(PosRatio<eimv_pr)*/) *
                                               dt; // wartość zadana/procent czegoś

                                    if ((DynamicBrakeFlag))
                                        tmp = eimc[eimc_f_Uzh];
                                    else
                                        tmp = eimc[eimc_f_Uzmax];

                                    auto f_cfu{DynamicBrakeFlag ? eimc[eimc_f_cfuH] : eimc[eimc_f_cfu]};

                                    eimv[eimv_Uzsmax] = Min0R(EngineVoltage - eimc[eimc_f_DU], tmp);
                                    eimv[eimv_fkr] = eimv[eimv_Uzsmax] / f_cfu;
                                        if ((eimv_pr < 0)) {
                                            eimv[eimv_Pmax] = eimc[eimc_p_Ph];
                                        } else {
                                            eimv[eimv_Pmax] = std::min(eimc[eimc_p_Pmax],
                                                                       0.001 * EngineVoltage *
                                                                               (eimc[eimc_p_Imax] - eimc[eimc_f_I0]) *
                                                                               Pirazy2 * eimc[eimc_s_cim] /
                                                                               eimc[eimc_s_p] / eimc[eimc_s_cfu]);
                                        }
                                    eimv[eimv_FMAXMAX] =
                                            0.001 *
                                            square(std::min(1.0,
                                                            eimv[eimv_fkr] /
                                                                    std::max(fabs(enrot) * eimc[eimc_s_p] +
                                                                                     eimc[eimc_s_dfmax] * eimv[eimv_ks],
                                                                             eimc[eimc_s_dfmax])) *
                                                   f_cfu / eimc[eimc_s_cfu]) *
                                            (eimc[eimc_s_dfmax] * eimc[eimc_s_dfic] * eimc[eimc_s_cim]) *
                                            Transmision.Ratio * NPoweredAxles * 2.0 / WheelDiameter;
                                        if ((eimv_pr < 0)) {
                                            eimv[eimv_Fful] =
                                                    std::min(eimc[eimc_p_Ph] * 3.6 / (Vel != 0.0 ? Vel : 0.001),
                                                             std::min(eimc[eimc_p_Fh], eimv[eimv_FMAXMAX]));
                                            eimv[eimv_Fmax] =
                                                    -Sign(V) *
                                                    (DirAbsolute)*std::min(
                                                            eimc[eimc_p_Ph] * 3.6 / (Vel != 0.0 ? Vel : 0.001),
                                                            std::min(-eimc[eimc_p_Fh] * eimv_pr, eimv[eimv_FMAXMAX]));
                                            double pr = eimv_pr;
                                            if (EIMCLogForce)
                                                pr = -log(1 - 4 * pr) / log(5);
                                            eimv[eimv_Fr] =
                                                    -Sign(V) *
                                                    (DirAbsolute)*std::min(
                                                            eimc[eimc_p_Ph] * 3.6 / (Vel != 0.0 ? Vel : 0.001),
                                                            std::min(-eimc[eimc_p_Fh] * pr, eimv[eimv_FMAXMAX]));
                                            if (InvertersRatio < 1.0)
                                                eimv[eimv_Fful] = 0;
                                            //*Min0R(1,(Vel-eimc[eimc_p_Vh0])/(eimc[eimc_p_Vh1]-eimc[eimc_p_Vh0]))
                                        } else {
                                            eimv[eimv_Fful] = Min0R(Min0R(3.6 * eimv[eimv_Pmax] / Max0R(Vel, 1),
                                                                          eimc[eimc_p_F0] - Vel * eimc[eimc_p_a1]),
                                                                    eimv[eimv_FMAXMAX]);
                                            //           if(not Flat)then
                                            eimv[eimv_Fmax] = eimv[eimv_Fful] * eimv_pr;
                                            //           else
                                            //             eimv[eimv_Fmax]:=Min0R(eimc[eimc_p_F0]*eimv_pr,eimv[eimv_Fful]);
                                            double pr = eimv_pr;
                                            if (EIMCLogForce)
                                                pr = log(1 + 4 * pr) / log(5);
                                            eimv[eimv_Fr] = eimv[eimv_Fful] * pr;
                                        }
                                        for (auto &inv: Inverters) {
                                            inv.Request = inv.IsActive ? eimv_pr : 0.0;
                                            inv.Error = inv.Failure_Const || (inv.Failure_Drive && inv.Request != 0);
                                            inv.IsActive = inv.Activate && !inv.Error;
                                        }
                                    eimv[eimv_ks] = eimv[eimv_Fr] / eimv[eimv_FMAXMAX];
                                    eimv[eimv_df] = eimv[eimv_ks] * eimc[eimc_s_dfmax];
                                    eimv[eimv_fp] =
                                            DirAbsolute * enrot * eimc[eimc_s_p] +
                                            eimv[eimv_df]; // do przemyslenia dzialanie pp z tmpV
                                                           //         eimv[eimv_U]:=Max0R(eimv[eimv_Uzsmax],Min0R(eimc[eimc_f_cfu]*eimv[eimv_fp],eimv[eimv_Uzsmax]));
                                                           //         eimv[eimv_pole]:=eimv[eimv_U]/(eimv[eimv_fp]*eimc[eimc_s_cfu]);
                                    if ((fabs(eimv[eimv_fp]) <= eimv[eimv_fkr]))
                                        eimv[eimv_pole] = f_cfu / eimc[eimc_s_cfu];
                                    else
                                        eimv[eimv_pole] = eimv[eimv_Uzsmax] / eimc[eimc_s_cfu] / fabs(eimv[eimv_fp]);
                                    eimv[eimv_U] = eimv[eimv_pole] * eimv[eimv_fp] * eimc[eimc_s_cfu];
                                    eimv[eimv_Ic] = (eimv[eimv_fp] - DirAbsolute * enrot * eimc[eimc_s_p]) *
                                                    eimc[eimc_s_dfic] * eimv[eimv_pole];
                                    eimv[eimv_If] = eimv[eimv_Ic] * eimc[eimc_s_icif];
                                    eimv[eimv_M] = eimv[eimv_pole] * eimv[eimv_Ic] * eimc[eimc_s_cim];
                                    eimv[eimv_Ipoj] = (eimv[eimv_Ic] * NPoweredAxles * InvertersRatio * eimv[eimv_U]) /
                                                              (EngineVoltage - eimc[eimc_f_DU]) +
                                                      eimc[eimc_f_I0];
                                    eimv[eimv_Pm] = DirActive * eimv[eimv_M] * NPoweredAxles * InvertersRatio * enrot *
                                                    Pirazy2 / 1000;
                                    eimv[eimv_Pe] = eimv[eimv_Ipoj] * EngineVoltage / 1000;
                                    eimv[eimv_eta] = eimv[eimv_Pm] / eimv[eimv_Pe];

                                    Im = eimv[eimv_If];
                                    if ((eimv[eimv_Ipoj] >= 0))
                                        Vadd *= (1.0 - 2.0 * dt);
                                    else if ((std::fabs(EngineVoltage) < EnginePowerSource.CollectorParameters.MaxV))
                                        Vadd *= (1.0 - dt);
                                    else
                                        Vadd = std::max(Vadd * (1.0 - 0.2 * dt),
                                                        0.007 * (std::fabs(EngineVoltage) -
                                                                 (EnginePowerSource.CollectorParameters.MaxV - 100)));
                                    Itot = eimv[eimv_Ipoj] * (0.01 + std::min(0.99, 0.99 - Vadd));

                                    EnginePower = fabs(eimv[eimv_Ic] * eimv[eimv_U] * NPoweredAxles) / 1000;
                                    // power inverters
                                    auto const tmpV{std::fabs(eimv[eimv_fp])};

                                        if ((RlistSize > 0) && ((std::fabs(eimv[eimv_If]) > 1.0) && (tmpV > 0.0001))) {

                                            int i = 0;
                                                while ((i < RlistSize - 1) && (DElist[i + 1].RPM < tmpV)) {
                                                    ++i;
                                                }
                                            InverterFrequency =
                                                    (tmpV - DElist[i].RPM) /
                                                            std::max(1.0, (DElist[i + 1].RPM - DElist[i].RPM)) *
                                                            (DElist[i + 1].GenPower - DElist[i].GenPower) +
                                                    DElist[i].GenPower;
                                        } else {
                                            InverterFrequency = 0.0;
                                        }

                                    Mm = eimv[eimv_M] * DirAbsolute;
                                    Mw = Mm * Transmision.Ratio * Transmision.Efficiency;
                                    Fw = Mw * 2.0 / WheelDiameter;
                                    Ft = Fw * NPoweredAxles * InvertersRatio;
                                    eimv[eimv_Fr] = DirAbsolute * Ft / 1000;
                                } // mains
                                else {
                                        for (auto &inv: Inverters) {
                                            inv.Freal = 0.0;
                                            inv.IsActive = false;
                                        }
                                    Im = 0.0;
                                    Mm = 0.0;
                                    Mw = 0.0;
                                    Fw = 0.0;
                                    Ft = 0.0;
                                    Itot = 0.0;
                                    eimv_pr = 0.0;
                                    EnginePower = 0.0;
                                    {
                                        for (int i = 0; i < 21; ++i)
                                            eimv[i] = 0.0;
                                    }
                                    Hamulec->SetED(0.0);
                                    InverterFrequency = 0.0; //(Hamulec as TLSt).SetLBP(LocBrakePress);
                                }
                            break;
                        } // ElectricInductionMotor

                    case TEngineType::None:
                        default: {
                            break;
                        }
                } // case EngineType

            switch (EngineType) {
                    case TEngineType::DieselElectric: {
                        // rough approximation of extra effort to overcome friction etc
                        EnginePower += EngineRPMRatio() * 0.15 * DElist[MainCtrlPosNo].GenPower;
                        break;
                    }
                    default: {
                        break;
                    }
            }


        return Ft;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Obliczenie predkości obrotowej kół???
    // *************************************************************************************************
    double TMoverParameters::ComputeRotatingWheel(double WForce, double dt, double n) const {
        double newn = 0, eps = 0;
        if ((n == 0) && (WForce * Sign(V) < 0))
            newn = 0;
            else {
                eps = WForce * WheelDiameter / (2.0 * AxleInertialMoment);
                newn = n + eps * dt;
                if ((newn * n <= 0) && (eps * n < 0))
                    newn = 0;
            }
        return newn;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Sprawdzenie bezpiecznika nadmiarowego
    // *************************************************************************************************
    bool TMoverParameters::FuseFlagCheck(void) const {
        bool FFC;

        FFC = false;
        if (Power > 0.01)
            FFC = FuseFlag;
        else // pobor pradu jezeli niema mocy
            for (int b = 0; b < 2; b++)
                if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                    if (Couplers[b].Connected->Power > 0.01)
                        FFC = Couplers[b].Connected->FuseFlagCheck();

        return FFC;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Załączenie bezpiecznika nadmiarowego
    // *************************************************************************************************
    bool TMoverParameters::FuseOn(range_t const Notify) {
        auto const result{RelayReset((relay_t::maincircuitground | relay_t::tractionnmotoroverload), Notify)};

        return result;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Wyłączenie bezpiecznika nadmiarowego
    // *************************************************************************************************
    void TMoverParameters::FuseOff(void) {
            if (!FuseFlag) {
                FuseFlag = true;
                EventFlag = true;
                SetFlag(SoundFlag, sound::relay | sound::loud);
        }
    }

    // resets relays assigned to specified customizable reset button
    bool TMoverParameters::UniversalResetButton(int const Button, range_t const Notify) {

        auto const lowvoltagepower{Power24vIsAvailable || Power110vIsAvailable};
            if (false == lowvoltagepower) {
                return false;
        }

        auto const relays{UniversalResetButtonFlag[Button]};
            if (relays == 0) {
                return false;
        }

        auto const result{RelayReset(relays, Notify)};

        return result;
    }

    // resets state of specified relays
    bool TMoverParameters::RelayReset(int const Relays, range_t const Notify) {

        auto const lowvoltagepower{Power24vIsAvailable || Power110vIsAvailable};
        bool reset{false};

            if (TestFlag(Relays, relay_t::maincircuitground)) {
                    if (((EngineType == TEngineType::ElectricSeriesMotor) ||
                         (EngineType == TEngineType::DieselElectric)) &&
                        ((GroundRelayStart == start_t::manual) ||
                         (GroundRelayStart == start_t::manualwithautofallback)) &&
                        (IsMainCtrlNoPowerPos()) && (ScndCtrlPos == 0) && (DirActive != 0) &&
                        (!TestFlag(EngDmgFlag, 1))) {
                        // NOTE: true means the relay is operational
                        reset |= (!GroundRelay && lowvoltagepower);
                        GroundRelay |= lowvoltagepower;
                }
        }

            if (TestFlag(Relays, relay_t::tractionnmotoroverload)) {
                    if (((EngineType == TEngineType::ElectricSeriesMotor) ||
                         (EngineType == TEngineType::DieselElectric)) &&
                        (IsMainCtrlNoPowerPos()) && (ScndCtrlPos == 0) && (DirActive != 0) &&
                        (!TestFlag(EngDmgFlag, 1))) {
                        // NOTE: false means the relay is operational
                        // TODO: cleanup, flip the FuseFlag code to match other relays
                        // TODO: check whether the power is required, TBD, TODO: make it configurable?
                        reset |= (FuseFlag && lowvoltagepower);
                        FuseFlag &= !lowvoltagepower;
                }
        }

            if (TestFlag(Relays, relay_t::primaryconverteroverload)) {
                    if ((ConverterOverloadRelayStart == start_t::manual)
                        //         && ( false == Mains )
                        && (false == ConverterAllow)) {
                        // NOTE: false means the relay is operational
                        // TODO: cleanup, flip the FuseFlag code to match other relays
                        // TODO: check whether the power is required, TBD, TODO: make it configurable?
                        reset |= (ConvOvldFlag && lowvoltagepower);
                        ConvOvldFlag &= !lowvoltagepower;
                }
        }

            if (reset) {
                SetFlag(SoundFlag, sound::relay | sound::loud);
        }

            if (Notify != range_t::local) {
                SendCtrlToNext("RelayReset", Relays, CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return reset;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Przeliczenie prędkości liniowej na obrotową
    // *************************************************************************************************
    double TMoverParameters::v2n(void) {
        // przelicza predkosc liniowa na obrotowa
        const double dmgn = 0.5;
        double n, deltan = 0;

        n = V / (M_PI * WheelDiameter); // predkosc obrotowa wynikajaca z liniowej [obr/s]
        deltan = n - nrot;              //"pochodna" prędkości obrotowej
            /* if (SlippingWheels)
               if (std::fabs(deltan) < 0.001)
               SlippingWheels = false; // wygaszenie poslizgu */ //poslizg jest w innym miejscu wygaszany też
            if (SlippingWheels) // nie ma zwiazku z predkoscia liniowa V
            {                   // McZapkie-221103: uszkodzenia kol podczas poslizgu
                if (deltan > dmgn)
                    if (FuzzyLogic(deltan, dmgn, p_slippdmg))
                        if (SetFlag(DamageFlag, dtrain_wheelwear)) // podkucie
                            EventFlag = true;
                if (deltan < -dmgn)
                    if (FuzzyLogic(-deltan, dmgn, p_slippdmg))
                        if (SetFlag(DamageFlag, dtrain_thinwheel)) // wycieranie sie obreczy
                            EventFlag = true;
                n = nrot; // predkosc obrotowa nie zalezy od predkosci liniowej
        }
        return n;
    }

    // *************************************************************************************************
    // Q: 20160714
    // Oblicza moment siły wytwarzany przez silnik
    // *************************************************************************************************
    double TMoverParameters::Momentum(double I) {
        // liczy moment sily wytwarzany przez silnik elektryczny}
        int SP;

        SP = ScndCtrlActualPos;
        if (ScndInMain)
            if (!(RList[MainCtrlActualPos].ScndAct == 255))
                SP = RList[MainCtrlActualPos].ScndAct;

        //     Momentum:=mfi*I*(1-1.0/(Abs(I)/mIsat+1));
        return (MotorParam[SP].mfi * I * (fabs(I) / (fabs(I) + MotorParam[SP].mIsat) - MotorParam[SP].mfi0));
    }

    // *************************************************************************************************
    // Q: 20160714
    // Oblicza moment siły do sterowania wzbudzeniem
    // *************************************************************************************************
    double TMoverParameters::MomentumF(double I, double Iw, int SCP) {
        // umozliwia dokladne sterowanie wzbudzeniem

        return (MotorParam[SCP].mfi * I *
                Max0R(fabs(Iw) / (fabs(Iw) + MotorParam[SCP].mIsat) - MotorParam[SCP].mfi0, 0));
    }

    // *************************************************************************************************
    // Q: 20160713
    // Odłączenie uszkodzonych silników
    // *************************************************************************************************
    bool TMoverParameters::CutOffEngine(void) {
        bool COE = false; // Ra: wartość domyślna, sprawdzić to trzeba
            if ((NPoweredAxles > 0) && (CabActive == 0) && (EngineType == TEngineType::ElectricSeriesMotor)) {
                    if (SetFlag(DamageFlag, -dtrain_engine)) {
                        NPoweredAxles = NPoweredAxles / 2; // bylo div czyli mod?
                        COE = true;
                }
        }
        return COE;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Przełączenie wysoki / niski prąd rozruchu
    // *************************************************************************************************
    bool TMoverParameters::MaxCurrentSwitch(bool State, range_t const Notify) {
        auto const initialstate{MotorOverloadRelayHighThreshold};

        MotorOverloadRelayHighThreshold = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("MaxCurrentSwitch", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return State != initialstate;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Przełączenie wysoki / niski prąd rozruchu automatycznego
    // *************************************************************************************************
    bool TMoverParameters::MinCurrentSwitch(bool State) {
        bool MCS = false;
            if (((EngineType == TEngineType::ElectricSeriesMotor) && (IminHi > IminLo)) ||
                ((TrainType == dt_EZT) && (EngineType != TEngineType::ElectricInductionMotor))) {

                    if (State && (Imin == IminLo)) {
                        Imin = IminHi;
                        MCS = true;
                        if (CabActive != 0)
                            SendCtrlToNext("MinCurrentSwitch", 1, CabActive);
                }
                    if ((!State) && (Imin == IminHi)) {
                        Imin = IminLo;
                        MCS = true;
                        if (CabActive != 0)
                            SendCtrlToNext("MinCurrentSwitch", 0, CabActive);
                }
        }
        return MCS;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Sprawdzenie wskaźnika jazdy na oporach
    // *************************************************************************************************
    bool TMoverParameters::ResistorsFlagCheck(void) const {
        bool RFC = false;

        if (Power > 0.01)
            RFC = ResistorsFlag;
            else // pobor pradu jezeli niema mocy
            {
                for (int b = 0; b < 2; b++)
                    if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                        if (Couplers[b].Connected->Power > 0.01)
                            RFC = Couplers[b].Connected->ResistorsFlagCheck();
            }
        return RFC;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Włączenie / wyłączenie automatycznego rozruchu
    // *************************************************************************************************
    bool TMoverParameters::AutoRelaySwitch(bool State) {
        bool ARS;
            if ((AutoRelayType == 2) && (AutoRelayFlag != State)) {
                AutoRelayFlag = State;
                ARS = true;
                SendCtrlToNext("AutoRelaySwitch", int(State), CabActive);
        } else
            ARS = false;

        return ARS;
    }

    // *************************************************************************************************
    // Q: 20160724
    // Sprawdzenie warunków pracy automatycznego rozruchu
    // *************************************************************************************************

    bool TMoverParameters::AutoRelayCheck(void) {
        bool OK = false; // b:int;
        bool ARC = false;

        auto const motorconnectorsoff{false == MotorConnectorsCheck()};

            // Ra 2014-06: dla SN61 nie działa prawidłowo
            // yBARC - rozlaczenie stycznikow liniowych
            if ((motorconnectorsoff) || (HasCamshaft ? IsMainCtrlActualNoPowerPos() : IsMainCtrlNoPowerPos())) {
                StLinFlag = false;
                OK = false;
                    if (false == DynamicBrakeFlag) {
                        Im = 0;
                        Itot = 0;
                        ResistorsFlag = false;
                }
        }

        // sprawdzenie wszystkich warunkow (AutoRelayFlag, AutoSwitch, Im<Imin)
        auto const ARFASI2{
                ((false == AutoRelayFlag) || ((MotorParam[ScndCtrlActualPos].AutoSwitch) && (fabs(Im) < Imin)))};
        auto const ARFASI{((false == AutoRelayFlag) || ((RList[MainCtrlActualPos].AutoSwitch) && (fabs(Im) < Imin)) ||
                           ((!RList[MainCtrlActualPos].AutoSwitch) && (RList[MainCtrlActualPos].Relay < MainCtrlPos)))};
        // brak PSR                   na tej pozycji działa PSR i prąd poniżej progu
        // na tej pozycji nie działa PSR i pozycja walu ponizej
        //                         chodzi w tym wszystkim o to, żeby można było zatrzymać rozruch na
        //                         jakiejś pozycji wpisując Autoswitch=0 i wymuszać
        //                         przejście dalej przez danie nastawnika na dalszą pozycję - tak to do
        //                         tej pory działało i na tym się opiera fizyka ET22-2k
        {
                if (StLinFlag) {
                        if ((RList[MainCtrlActualPos].R == 0) && ((ScndCtrlActualPos > 0) || (ScndCtrlPos > 0)) &&
                            (!(CoupledCtrl) ||
                             (RList[MainCtrlActualPos].Relay == MainCtrlPos))) { // zmieniaj scndctrlactualpos
                                                                                 // scnd bez samoczynnego rozruchu
                                if (ScndCtrlActualPos < ScndCtrlPos) {
                                        if ((LastRelayTime > CtrlDelay) && (ARFASI2)) {
                                            ++ScndCtrlActualPos;
                                            SetFlag(SoundFlag, sound::shuntfield);
                                            OK = true;
                                    }
                                } else if (ScndCtrlActualPos > ScndCtrlPos) {
                                        if ((LastRelayTime > CtrlDownDelay) && (TrainType != dt_EZT)) {
                                            --ScndCtrlActualPos;
                                            SetFlag(SoundFlag, sound::shuntfield);
                                            OK = true;
                                    }
                            } else
                                OK = false;
                        } else { // zmieniaj mainctrlactualpos
                                if ((DirActive < 0) && (TrainType != dt_PseudoDiesel)) {
                                        if (RList[MainCtrlActualPos + 1].Bn > BackwardsBranchesAllowed) {
                                            return false; // nie poprawiamy przy konwersji
                                                          // return ARC;// bbylo exit; //Ra: to powoduje, że EN57 nie
                                                          // wyłącza się przy IminLo
                                    }
                            }
                                // main bez samoczynnego rozruchu
                                if ((MainCtrlActualPos < (sizeof(RList) / sizeof(TScheme) -
                                                          1)) // crude guard against running out of current fixed table
                                    && ((RList[MainCtrlActualPos].Relay < MainCtrlPos) ||
                                        ((RList[MainCtrlActualPos + 1].Relay == MainCtrlPos) &&
                                         (MainCtrlActualPos < RlistSize)) ||
                                        ((TrainType == dt_ET22) && (DelayCtrlFlag)))) {

                                        // prevent switch to parallel mode if motor overload relay is set to high
                                        // threshold mode
                                        if ((IsMotorOverloadRelayHighThresholdOn()) &&
                                            (RList[MainCtrlActualPos + 1].Bn > 1)) {
                                            return false;
                                    }

                                        if ((RList[MainCtrlPos].R == 0) && (MainCtrlPos > 0) &&
                                            (MainCtrlPos != MainCtrlPosNo) && (FastSerialCircuit == 1)) {
                                            // szybkie wchodzenie na bezoporowa (303E)
                                            // MainCtrlActualPos:=MainCtrlPos; //hunter-111012:
                                            ++MainCtrlActualPos;
                                                if (MainCtrlPos - MainCtrlActualPos == 1) {
                                                    // HACK: ensure we play only single sound of basic relays for entire
                                                    // trasition; return false for all but last step despite
                                                    // configuration change, to prevent playback of the basic relay
                                                    // sound TBD, TODO: move the basic sound event here and enable it
                                                    // with call parameter
                                                    OK = true;
                                            }
                                                if (RList[MainCtrlActualPos].R == 0) {
                                                    SetFlag(SoundFlag, sound::parallel | sound::loud);
                                                    OK = true;
                                            }
                                        } else if ((LastRelayTime > CtrlDelay) && (ARFASI)) {
                                                // WriteLog("LRT = " + FloatToStr(LastRelayTime) + ", " +
                                                // FloatToStr(CtrlDelay));
                                                if ((TrainType == dt_ET22) && (MainCtrlPos > 1) &&
                                                    ((RList[MainCtrlActualPos].Bn < RList[MainCtrlActualPos + 1].Bn) ||
                                                     (DelayCtrlFlag))) {
                                                        // et22 z walem grupowym
                                                        if (!DelayCtrlFlag) // najpierw przejscie
                                                        {
                                                            ++MainCtrlActualPos;
                                                            DelayCtrlFlag = true; // tryb przejscia
                                                            OK = true;
                                                        } else if (LastRelayTime > 4 * CtrlDelay) // przejscie
                                                        {

                                                            DelayCtrlFlag = false;
                                                            OK = true;
                                                    }
                                                    /*
                                                       else
                                                       ;
                                                       */
                                                } else // nie ET22 z wałem grupowym
                                                {
                                                    ++MainCtrlActualPos;
                                                    OK = true;
                                                }
                                                //---------
                                                // hunter-111211: poprawki
                                                if (MainCtrlActualPos > 0) {
                                                        if ((RList[MainCtrlActualPos].R == 0) &&
                                                            (MainCtrlActualPos != MainCtrlPosNo)) {
                                                            // wejscie na bezoporowa
                                                            SetFlag(SoundFlag, sound::parallel | sound::loud);
                                                        } else if ((RList[MainCtrlActualPos].R > 0) &&
                                                                   (RList[MainCtrlActualPos - 1].R == 0)) {
                                                            // wejscie na drugi uklad
                                                            SetFlag(SoundFlag, sound::parallel);
                                                    }
                                            }
                                    }
                                } else if (RList[MainCtrlActualPos].Relay > MainCtrlPos) {
                                        if ((RList[MainCtrlPos].R == 0) && (MainCtrlPos > 0) &&
                                            (!(MainCtrlPos == MainCtrlPosNo)) && (FastSerialCircuit == 1)) {
                                            // szybkie wchodzenie na bezoporowa (303E)
                                            // MainCtrlActualPos:=MainCtrlPos; //hunter-111012:
                                            --MainCtrlActualPos;
                                            OK = true;
                                                if (RList[MainCtrlActualPos].R == 0) {
                                                    SetFlag(SoundFlag, sound::parallel);
                                            }
                                        } else if (LastRelayTime > CtrlDownDelay) {
                                                if (TrainType != dt_EZT) // tutaj powinien być tryb sterowania wałem
                                                {
                                                    --MainCtrlActualPos;
                                                    OK = true;
                                            }
                                            if (MainCtrlActualPos > 0) // hunter-111211: poprawki
                                                    if (RList[MainCtrlActualPos].R == 0) {
                                                        // dzwieki schodzenia z bezoporowej}
                                                        SetFlag(SoundFlag, sound::parallel);
                                                }
                                    }
                                } else if ((RList[MainCtrlActualPos].R > 0) && (ScndCtrlActualPos > 0)) {
                                        if (LastRelayTime > CtrlDownDelay) {
                                            --ScndCtrlActualPos; // boczniki nie dzialaja na poz. oporowych
                                            SetFlag(SoundFlag, sound::shuntfield);
                                            OK = true;
                                    }
                            } else
                                OK = false;
                        }
                } else // not StLinFlag
                {
                    OK = false;
                        // ybARC - zalaczenie stycznikow liniowych
                        if ((false == motorconnectorsoff) && (MainCtrlActualPos == 0) &&
                            ((TrainType == dt_EZT || HasCamshaft) ? MainCtrlPowerPos() > 0 : MainCtrlPowerPos() == 1)) {

                            DelayCtrlFlag = true;
                                if (LastRelayTime >= InitialCtrlDelay) {
                                    StLinFlag = true;
                                    MainCtrlActualPos = 1;
                                    DelayCtrlFlag = false;
                                    SetFlag(SoundFlag, sound::relay | sound::loud);
                                    OK = true;
                            }
                        } else {
                            DelayCtrlFlag = false;
                        }

                        if ((false == StLinFlag) && ((MainCtrlActualPos > 0) || (ScndCtrlActualPos > 0))) {

                                if (CoupledCtrl) {

                                        if (TrainType == dt_EZT) {
                                                // EN57 wal jednokierunkowy calosciowy
                                                if (MainCtrlActualPos == 1) {

                                                    MainCtrlActualPos = 0;
                                                    OK = true;
                                                } else {

                                                        if (LastRelayTime > CtrlDownDelay) {

                                                                if (MainCtrlActualPos < RlistSize) {
                                                                    // dojdz do konca
                                                                    ++MainCtrlActualPos;
                                                                } else if (ScndCtrlActualPos < ScndCtrlPosNo) {
                                                                    // potem boki
                                                                    ++ScndCtrlActualPos;
                                                                    SetFlag(SoundFlag, sound::shuntfield);
                                                                } else {
                                                                    // i sie przewroc na koniec
                                                                    MainCtrlActualPos = 0;
                                                                    ScndCtrlActualPos = 0;
                                                                }
                                                            OK = true;
                                                    }
                                                }
                                        } else {
                                                // wal kulakowy dwukierunkowy
                                                if (LastRelayTime > CtrlDownDelay) {
                                                        if (ScndCtrlActualPos > 0) {
                                                            --ScndCtrlActualPos;
                                                            SetFlag(SoundFlag, sound::shuntfield);
                                                        } else {
                                                            --MainCtrlActualPos;
                                                        }
                                                    OK = true;
                                            }
                                        }
                                } else if (HasCamshaft) {
                                        // wal kulakowy dwukierunkowy
                                        if (LastRelayTime > CtrlDownDelay) {
                                                if (MainCtrlActualPos > 0) {
                                                    --MainCtrlActualPos;
                                            }
                                            ScndCtrlActualPos = 0;
                                            OK = true;
                                    }
                                } else {
                                    MainCtrlActualPos = 0;
                                    ScndCtrlActualPos = 0;
                                    OK = true;
                                }
                    }
                }
            if (OK)
                LastRelayTime = 0;

            return OK;
        }
    }

    bool TMoverParameters::MotorConnectorsCheck() {

        // hunter-111211: wylacznik cisnieniowy
        ControlPressureSwitch = ((HasControlPressureSwitch) && ((BrakePress > 2.0) || (PipePress < 3.6)));

            if (true == ControlPressureSwitch) {
                return false;
        }

        auto const connectorsoff{(false == Mains) || (true == FuseFlag) || (true == StLinSwitchOff) ||
                                 (DirActive == 0)};

        return (false == connectorsoff);
    }

    bool TMoverParameters::OperatePantographsValve(operation_t const State, range_t const Notify) {

            if ((EnginePowerSource.SourceType == TPowerSource::CurrentCollector) &&
                (EnginePowerSource.CollectorParameters.CollectorsNo > 0)) {

                auto &valve{PantsValve};

                    switch (State) {
                            case operation_t::none: {
                                valve.is_enabled = false;
                                valve.is_disabled = false;
                                break;
                            }
                            case operation_t::enable: {
                                valve.is_enabled = true;
                                valve.is_disabled = false;
                                break;
                            }
                            case operation_t::disable: {
                                valve.is_enabled = false;
                                valve.is_disabled = true;
                                break;
                            }
                            case operation_t::enable_on: {
                                valve.is_enabled = true;
                                break;
                            }
                            case operation_t::enable_off: {
                                valve.is_enabled = false;
                                break;
                            }
                            case operation_t::disable_on: {
                                valve.is_disabled = true;
                                break;
                            }
                            case operation_t::disable_off: {
                                valve.is_disabled = false;
                                break;
                            }
                    }
        }

            if (Notify != range_t::local) {
                SendCtrlToNext("PantsValve", static_cast<double>(State), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return true;
    }

    bool TMoverParameters::OperatePantographValve(end const End, operation_t const State, range_t const Notify) {

            if ((EnginePowerSource.SourceType == TPowerSource::CurrentCollector) &&
                (EnginePowerSource.CollectorParameters.CollectorsNo > 0)) {

                auto &valve{Pantographs[End].valve};

                    switch (State) {
                            case operation_t::none: {
                                valve.is_enabled = false;
                                valve.is_disabled = false;
                                break;
                            }
                            case operation_t::enable: {
                                valve.is_enabled = true;
                                valve.is_disabled = false;
                                break;
                            }
                            case operation_t::disable: {
                                valve.is_enabled = false;
                                valve.is_disabled = true;
                                break;
                            }
                            case operation_t::enable_on: {
                                valve.is_enabled = true;
                                break;
                            }
                            case operation_t::enable_off: {
                                valve.is_enabled = false;
                                break;
                            }
                            case operation_t::disable_on: {
                                valve.is_disabled = true;
                                break;
                            }
                            case operation_t::disable_off: {
                                valve.is_disabled = false;
                                break;
                            }
                    }
        }

            if (Notify != range_t::local) {
                SendCtrlToNext("PantValve",
                               // HACK: pack the state, pantograph index and sender cab into 8-bit value
                               // with high bit storing front/rear pantograph, and 7th bit storing sender cab
                               static_cast<double>(0x80 * (End == end::front ? 0 : 1) +
                                                   0x40 * (CabActive != -1 ? 1 : 0) + static_cast<int>(State)),
                               CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return true;
    }

    bool TMoverParameters::DropAllPantographs(bool const State, range_t const Notify) {

        auto const initialstate{PantAllDown};

        PantAllDown = State;

            if (Notify != range_t::local) {
                SendCtrlToNext("PantAllDown", (State ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return State != initialstate;
    }

    void TMoverParameters::CheckEIMIC(double dt) {
        double offset = EIMCtrlAdditionalZeros ? 1.0 : 0.0;
        double multiplier = (EIMCtrlEmergency ? 1.0 : 0.0) + offset;
            switch (EIMCtrlType) {
                case 0:
                    eimic = (LocalBrakeRatio() > 0.01 ? -LocalBrakeRatio()
                             : eimic_analog > 0.01    ? eimic_analog
                                                      : (double)MainCtrlPos / (double)MainCtrlPosNo);
                        if (EIMCtrlAdditionalZeros || EIMCtrlEmergency) {
                            if (eimic > 0.001)
                                eimic = std::max(0.002,
                                                 eimic * (double)MainCtrlPosNo / ((double)MainCtrlPosNo - offset) -
                                                         offset / ((double)MainCtrlPosNo - offset));
                            if ((eimic < -0.001) && (BrakeHandle != TBrakeHandle::MHZ_EN57))
                                eimic = std::min(-0.002, eimic * (double)LocalBrakePosNo /
                                                                         ((double)LocalBrakePosNo - multiplier) +
                                                                 offset / ((double)LocalBrakePosNo - multiplier));
                    }
                    if ((eimic > 0.001) && (SpeedCtrlUnit.IsActive))
                        eimic = std::max(eimic, SpeedCtrlUnit.MinPower);
                    break;
                case 1:
                        switch (MainCtrlPos) {
                            case 0:                                          // B+
                                eimic -= clamp(1.0 + eimic, 0.0, dt * 0.14); // odejmuj do -1
                                break;
                            case 1:                                          // B
                                eimic -= clamp(0.0 + eimic, 0.0, dt * 0.14); // odejmuj do 0
                                break;
                            case 2:                                          // B-
                            case 3:                                          // 0
                            case 4:                                          // T-
                                eimic -= clamp(0.0 + eimic, 0.0, dt * 0.14); // odejmuj do 0
                                eimic += clamp(0.0 - eimic, 0.0, dt * 0.14); // dodawaj do 0
                                break;
                            case 5:                                          // T
                                eimic += clamp(0.0 - eimic, 0.0, dt * 0.14); // dodawaj do 0
                                break;
                            case 6:                                          // T+
                                eimic += clamp(1.0 - eimic, 0.0, dt * 0.14); // dodawaj do 1
                                break;
                            case 7:                                          // TMax
                                eimic += clamp(1.0 - eimic, 0.0, dt * 0.14); // dodawaj do 1, max
                                break;
                        }
                    if (MainCtrlPos >= 3 && eimic < 0)
                        eimic = 0;
                    if (MainCtrlPos <= 3 && eimic > 0)
                        eimic = 0;
                    break;
                case 2:
                        if ((MainCtrlActualPos != MainCtrlPos) || (LastRelayTime > InitialCtrlDelay)) {
                            double delta = (MainCtrlActualPos == MainCtrlPos ? dt * CtrlDelay : 0.01);
                                switch (MainCtrlPos) {
                                    case 0:
                                    case 1:
                                        eimic -= clamp(1.0 + eimic, 0.0, delta); // odejmuj do -1
                                        if (eimic > 0)
                                            eimic = 0;
                                        break;
                                    case 2:
                                        eimic -= clamp(0.0 + eimic, 0.0, delta); // odejmuj do 0
                                        break;
                                    case 3:
                                        eimic += clamp(0.0 - eimic, 0.0, delta); // dodawaj do 0
                                        break;
                                    case 4:
                                        eimic += clamp(1.0 - eimic, 0.0, delta); // dodawaj do 1
                                        if (eimic < 0)
                                            eimic = 0;
                                        break;
                                }
                    }
                    if (MainCtrlActualPos == MainCtrlPos)
                        LastRelayTime += dt;
                        else {
                            LastRelayTime = 0;
                            MainCtrlActualPos = MainCtrlPos;
                        }
                    break;
                case 3:
                        if (UniCtrlIntegratedBrakePNCtrl) {
                                if ((UniCtrlList[MainCtrlPos].mode != BrakeCtrlPos) &&
                                    (MainCtrlActualPos ==
                                     MainCtrlPos)) // there was no move of controller, but brake only
                                {
                                    if (BrakeCtrlPos < UniCtrlList[MainCtrlPosNo].mode)
                                        BrakeLevelSet(UniCtrlList[MainCtrlPosNo].mode); // bottom clamping
                                    if (BrakeCtrlPos > UniCtrlList[0].mode)
                                        BrakeLevelSet(UniCtrlList[0].mode); // top clamping
                                        if (IsCabMaster()) {
                                            while (BrakeCtrlPos > UniCtrlList[MainCtrlPos].mode)
                                                DecMainCtrl(1); // find nearest position
                                            while (BrakeCtrlPos < UniCtrlList[MainCtrlPos].mode)
                                                IncMainCtrl(1); // find nearest position
                                    }
                            } else // controller was moved
                                BrakeLevelSet(UniCtrlList[MainCtrlPos].mode);
                    }

                        if ((MainCtrlActualPos != MainCtrlPos) || (LastRelayTime > InitialCtrlDelay)) {
                            eimic -= clamp(-UniCtrlList[MainCtrlPos].SetCtrlVal + eimic, 0.0,
                                           (MainCtrlActualPos == MainCtrlPos
                                                    ? dt * UniCtrlList[MainCtrlPos].SpeedDown
                                                    : sign(UniCtrlList[MainCtrlPos].SpeedDown) * 0.01)); // odejmuj do X
                            eimic += clamp(UniCtrlList[MainCtrlPos].SetCtrlVal - eimic, 0.0,
                                           (MainCtrlActualPos == MainCtrlPos
                                                    ? dt * UniCtrlList[MainCtrlPos].SpeedUp
                                                    : sign(UniCtrlList[MainCtrlPos].SpeedUp) * 0.01)); // dodawaj do X
                            eimic = clamp(eimic, UniCtrlList[MainCtrlPos].MinCtrlVal,
                                          UniCtrlList[MainCtrlPos].MaxCtrlVal);
                    }
                    if (MainCtrlActualPos == MainCtrlPos)
                        LastRelayTime += dt;
                        else {
                            LastRelayTime = 0;
                            MainCtrlActualPos = MainCtrlPos;
                        }
                    if (Hamulec->GetEDBCP() > 0.3 && eimic < 0 &&
                        !UniCtrlIntegratedLocalBrakeCtrl) // when braking with pneumatic brake
                        eimic = 0;                        // shut off retarder
                        if ((UniCtrlIntegratedBrakeCtrl == false) && (UniCtrlIntegratedLocalBrakeCtrl == false)) {
                            eimic = (LocalBrakeRatio() > 0.01 ? -LocalBrakeRatio() : eimic);
                    }
            }
            if (LocHandleTimeTraxx) {
                    if (LocalBrakeRatio() < 0.05) // pozycja 0
                    {
                        eim_localbrake -= dt * 0.17; // zmniejszanie
                }

                    if (LocalBrakeRatio() > 0.15) // pozycja 2
                    {
                        eim_localbrake += dt * 0.17; // wzrastanie
                        eim_localbrake = std::max(eim_localbrake, BrakePress / MaxBrakePress[0]);
                    } else {
                        if (eim_localbrake < Hamulec->GetEDBCP() / MaxBrakePress[0])
                            eim_localbrake = 0;
                    }
                eim_localbrake = clamp(eim_localbrake, 0.0, 1.0);
                if (eim_localbrake > 0.04 && eimic > 0)
                    eimic = 0;
        }

        auto const eimicpowerenabled{((true == Mains) || (Power == 0.0)) &&
                                     (!SpringBrake.IsActive || !SpringBrakeCutsOffDrive) && (!LockPipe) &&
                                     (DirAbsolute != 0)};
        auto const eimicdoorenabled{(SpringBrake.IsActive && ReleaseParkingBySpringBrakeWhenDoorIsOpen)};
        double eimic_max = 0.0;
            if ((Doors.instances[side::left].open_permit == false) &&
                (Doors.instances[side::right].open_permit == false)) {
                    if (eimicpowerenabled) {
                        eimic_max = 1.0;
                    } else {
                        eimic_max = 0.001;
                    }
            } else {
                    if (eimicdoorenabled) {
                        eimic_max = 0.001;
                }
            }
        eimic = clamp(eimic, -1.0, eimicpowerenabled ? eimic_max : 0.0);
    }

    void TMoverParameters::CheckSpeedCtrl(double dt) {
            if (EIMCtrlType == 0) {
                SpeedCtrlUnit.DesiredPower = std::max(eimic, 0.0);
        }
        double accfactor = SpeedCtrlUnit.DesiredPower;
            if (EIMCtrlType >= 2) {
                    if (MainCtrlPos < MainCtrlPosNo - 2) {
                        SpeedCtrlUnit.Standby = true;
                }
                    if (MainCtrlPos > MainCtrlPosNo - 1) {
                        SpeedCtrlUnit.Standby = false;
                }
                    if (!SpeedCtrlUnit.BrakeIntervention) {
                        if ((Hamulec->GetEDBCP() > 0.4) || (PipePress < (HighPipePress - 0.2)))
                            SpeedCtrlUnit.Standby = true;
                }
                    if ((EIMCtrlType >= 3) && (UniCtrlList[MainCtrlPos].SpeedUp <= 0)) {
                        accfactor = 0.0;
                        eimicSpeedCtrl = 0;
                }
        }
            if (SpeedCtrlUnit.IsActive) { // speed control
                    if (true) {
                            if ((!SpeedCtrlUnit.Standby)) {
                                    if (SpeedCtrlUnit.ManualStateOverride) {
                                        if (eimic > 0.0009)
                                            eimic = 1.0;
                                }
                                double error = (std::max(SpeedCtrlValue + SpeedCtrlUnit.Offset, 0.0) - Vel);
                                double factorP = error > 0 ? SpeedCtrlUnit.FactorPpos : SpeedCtrlUnit.FactorPneg;
                                double eSCP = clamp(factorP * error, -1.2, 1.0); // P module
                                bool retarder_not_work = (EngineType != TEngineType::DieselEngine) ||
                                                         (Vel < SpeedCtrlUnit.BrakeInterventionVel);
                                    if (eSCP < -1.0) {
                                        SpeedCtrlUnit.BrakeInterventionBraking =
                                                (eSCP < -1.1) && retarder_not_work &&
                                                (eimicSpeedCtrl < -0.99 * SpeedCtrlUnit.DesiredPower);
                                        eSCP = -1.0;
                                }
                                SpeedCtrlUnit.BrakeInterventionUnbraking = (eSCP > 0.0) || (Vel == 0.0);
                                    if (fabs(eSCP) < 0.999) {
                                        // TODO: check how to disable integral part when braking in smart way
                                        // double factorI = eimicSpeedCtrlIntegral >= 0 ? SpeedCtrlUnit.FactorIpos :
                                        // SpeedCtrlUnit.FactorIneg;
                                        double factorI = eimicSpeedCtrlIntegral >= 0 ? SpeedCtrlUnit.FactorIpos
                                                                                     : SpeedCtrlUnit.FactorIneg;
                                        eimicSpeedCtrlIntegral = clamp(eimicSpeedCtrlIntegral + factorI * eSCP * dt,
                                                                       -1.0 + eSCP, 1.0 - eSCP);
                                    } else {
                                        eimicSpeedCtrlIntegral = 0;
                                    }
                                auto const DesiredeimicSpeedCtrl{
                                        clamp(eimicSpeedCtrlIntegral + eSCP, -SpeedCtrlUnit.DesiredPower, accfactor)};
                                eimicSpeedCtrl =
                                        clamp(DesiredeimicSpeedCtrl, eimicSpeedCtrl - SpeedCtrlUnit.PowerDownSpeed * dt,
                                              eimicSpeedCtrl + SpeedCtrlUnit.PowerUpSpeed * dt);
                                    if (Vel < SpeedCtrlUnit.FullPowerVelocity) {
                                        eimicSpeedCtrl = std::min(eimicSpeedCtrl, SpeedCtrlUnit.InitialPower);
                                }
                                    if ((Vel < SpeedCtrlUnit.StartVelocity) && (MainCtrlPos < MainCtrlPosNo)) {
                                        eimicSpeedCtrl = 0;
                                        eimic = 0;
                                }
                            } else {
                                eimicSpeedCtrl = 0;
                                eimicSpeedCtrlIntegral = 0;
                            }
                        SpeedCtrlUnit.Parking =
                                (Vel == 0.0) && (eimic <= 0) && (EngineType != TEngineType::ElectricInductionMotor);
                        SendCtrlToNext("SpeedCtrlUnit.Parking", SpeedCtrlUnit.Parking, CabActive);

                    } else {
                        if (Vmax < 250)
                            eimicSpeedCtrl = clamp(0.5 * (SpeedCtrlValue - Vel), -1.0, 1.0);
                        else
                            eimicSpeedCtrl = clamp(0.5 * (SpeedCtrlValue * 2 - Vel), -1.0, 1.0);
                    }
                    if (((SpeedCtrlAutoTurnOffFlag & 2) == 2) && (Hamulec->GetEDBCP() > 0.25)) {
                        DecScndCtrl(2);
                        SpeedCtrlUnit.IsActive = false;
                }
            } else {
                eimicSpeedCtrl = 1;
                eimicSpeedCtrlIntegral = 0;
                SpeedCtrlUnit.Parking = false;
                SendCtrlToNext("SpeedCtrlUnit.Parking", SpeedCtrlUnit.Parking, CabActive);
            }
    }

    void TMoverParameters::SpeedCtrlButton(int button) {
            if ((SpeedCtrl) && (ScndCtrlPos > 0)) {
                SpeedCtrlValue = SpeedCtrlButtons[button];
        }
    }

    void TMoverParameters::SpeedCtrlInc() {
            if ((SpeedCtrl) && (ScndCtrlPos > 0)) {
                double x = floor(SpeedCtrlValue / SpeedCtrlUnit.VelocityStep) + 1.0;
                SpeedCtrlValue = std::min(x * SpeedCtrlUnit.VelocityStep, SpeedCtrlUnit.MaxVelocity);
        }
    }

    void TMoverParameters::SpeedCtrlDec() {
            if ((SpeedCtrl) && (ScndCtrlPos > 0)) {
                double x = ceil(SpeedCtrlValue / SpeedCtrlUnit.VelocityStep) - 1.0;
                SpeedCtrlValue = std::max(x * SpeedCtrlUnit.VelocityStep, SpeedCtrlUnit.MinVelocity);
        }
    }

    bool TMoverParameters::SpeedCtrlPowerInc() {
        if (!(SpeedCtrl && ScndCtrlPos > 0))
            return false;

        if (SpeedCtrlUnit.DesiredPower == SpeedCtrlUnit.MaxPower)
            return false;

        SpeedCtrlUnit.DesiredPower =
                std::min(SpeedCtrlUnit.DesiredPower + SpeedCtrlUnit.PowerStep, SpeedCtrlUnit.MaxPower);
        return true;
    }

    bool TMoverParameters::SpeedCtrlPowerDec() {
        if (!(SpeedCtrl && ScndCtrlPos > 0))
            return false;

        if (SpeedCtrlUnit.DesiredPower == SpeedCtrlUnit.MinPower)
            return false;

        SpeedCtrlUnit.DesiredPower =
                std::max(SpeedCtrlUnit.DesiredPower - SpeedCtrlUnit.PowerStep, SpeedCtrlUnit.MinPower);
        return true;
    }

    // *************************************************************************************************
    // Q: 20160715
    // Zmienia parametr do którego dąży sprzęgło
    // *************************************************************************************************
    bool TMoverParameters::dizel_EngageSwitch(double state) {
            if ((EngineType == TEngineType::DieselEngine) && (state <= 1) && (state >= 0) &&
                (state != dizel_engagestate)) {
                dizel_engagestate = state;
                return true;
        } else
            return false;
    }

    // *************************************************************************************************
    // Q: 20160715
    // Zmienia parametr do którego dąży sprzęgło
    // *************************************************************************************************
    bool TMoverParameters::dizel_EngageChange(double dt) {
        double engagespeed = 0; // OK:boolean;
        bool DEC;

        DEC = false;
        if (dizel_engage - dizel_engagestate > 0)
            engagespeed = engagedownspeed;
        else
            engagespeed = engageupspeed;
        if (dt > 0.2)
            dt = 0.1;
            if (fabs(dizel_engage - dizel_engagestate) < 0.11) {
                    if (dizel_engage != dizel_engagestate) {
                        DEC = true;
                        dizel_engage = dizel_engagestate;
                }
                // else OK:=false; //już jest false
            } else {
                dizel_engage = dizel_engage + engagespeed * dt * (dizel_engagestate - dizel_engage);
                // OK:=false;
            }
        // dizel_EngageChange:=OK;
        return DEC;
    }

    // *************************************************************************************************
    // Q: 20160715
    // Automatyczna zmiana biegów gdy prędkość przekroczy widełki
    // *************************************************************************************************
    bool TMoverParameters::dizel_AutoGearCheck(void) {
        auto OK{false};

        auto const VelUp{(MotorParam[ScndCtrlActualPos].mfi0 != 0.0
                                  ? MotorParam[ScndCtrlActualPos].mfi0 +
                                            (MotorParam[ScndCtrlActualPos].mfi - MotorParam[ScndCtrlActualPos].mfi0) *
                                                    std::max(0.0, eimic_real)
                                  : MotorParam[ScndCtrlActualPos].mfi)};

        auto const VelDown{((MotorParam[ScndCtrlActualPos].fi0 != 0.0) && (eimic_real <= 0.0)
                                    ? MotorParam[ScndCtrlActualPos].fi0
                                    : MotorParam[ScndCtrlActualPos].fi)};

            if (MotorParam[ScndCtrlActualPos].AutoSwitch && Mains) {
                    if ((RList[MainCtrlPos].Mn == 0) && (!hydro_TC)) {
                        if (dizel_engagestate > 0)
                            dizel_EngageSwitch(0);
                        if ((IsMainCtrlNoPowerPos()) && (ScndCtrlActualPos > 0))
                            dizel_automaticgearstatus = -1;
                    } else {
                            if (MotorParam[ScndCtrlActualPos].AutoSwitch &&
                                (dizel_automaticgearstatus == 0)) // sprawdz czy zmienic biegi
                            {
                                    if (Vel > VelUp) {
                                            // shift up
                                            if (ScndCtrlActualPos < ScndCtrlPosNo) {
                                                dizel_automaticgearstatus = 1;
                                                OK = true;
                                        }
                                    } else if (Vel < VelDown) {
                                            // shift down
                                            if (ScndCtrlActualPos > 0) {
                                                dizel_automaticgearstatus = -1;
                                                OK = true;
                                        }
                                }
                        }
                    }
                    if ((dizel_engage < 0.1) && (dizel_automaticgearstatus != 0)) {
                        if (dizel_automaticgearstatus == 1)
                            ScndCtrlActualPos++;
                        else
                            ScndCtrlActualPos--;
                        dizel_automaticgearstatus = 0;
                        dizel_EngageSwitch(1.0);
                        OK = true;
                }
        }

            if (Mains) {
                    if (EIMCtrlType > 0) // sterowanie komputerowe
                    {
                            if (dizel_automaticgearstatus == 0) {
                                if ((hydro_TC && hydro_TC_Fill > 0.01) || (eimic_real > 0.005))
                                    dizel_EngageSwitch(1.0);
                                else if (Vel > hydro_R_EngageVel && hydro_R && hydro_R_Fill > 0.01)
                                    dizel_EngageSwitch(0.5);
                                else
                                    dizel_EngageSwitch(0.0);
                        } else
                            dizel_EngageSwitch(0.0);
                } else if (dizel_automaticgearstatus == 0) // ustaw cisnienie w silowniku sprzegla}
                        switch (RList[MainCtrlPos].Mn) {
                            case 1:
                                dizel_EngageSwitch(0.5);
                                break;
                            case 2:
                                dizel_EngageSwitch(1.0);
                                break;
                            case 3:
                                if (Vel > dizel_minVelfullengage)
                                    dizel_EngageSwitch(1.0);
                                else
                                    dizel_EngageSwitch(0.5);
                                break;
                            case 4:
                                if (Vel > dizel_minVelfullengage)
                                    dizel_EngageSwitch(1.0);
                                else
                                    dizel_EngageSwitch(0.66);
                                break;
                            case 5:
                                if (Vel > dizel_minVelfullengage)
                                    dizel_EngageSwitch(1.0);
                                else
                                    dizel_EngageSwitch(0.35 * (1 + RList[MainCtrlPos].R) * RList[MainCtrlPos].R);
                                break;
                            default:
                                if (hydro_TC && hydro_TC_Fill > 0.01)
                                    dizel_EngageSwitch(1.0);
                                else
                                    dizel_EngageSwitch(0.0);
                        }
                else
                    dizel_EngageSwitch(0.0);
                if (!(MotorParam[ScndCtrlActualPos].mIsat > 0))
                    dizel_EngageSwitch(0.0); // wylacz sprzeglo na pozycjach neutralnych
                if (!AutoRelayFlag)
                    ScndCtrlActualPos = ScndCtrlPos;
        }
        return OK;
    }

    // performs diesel engine startup procedure; potentially clears startup switch; returns: true if the engine can be
    // started, false otherwise
    bool TMoverParameters::dizel_StartupCheck() {

        auto engineisready{true}; // make inital optimistic presumption, then watch the reality crush it

            // test the fuel pump
            // TODO: add fuel pressure check
            if ((false == FuelPump.is_active) ||
                ((EngineType == TEngineType::DieselEngine) && (RList[MainCtrlPos].R == 0.0))) {
                engineisready = false;
                //        if( FuelPump.start_type == start_t::manual ) {
                // with manual pump control startup procedure is done only once per starter switch press
                dizel_startup = false;
                //        }
        }
            // test the oil pump
            if ((false == OilPump.is_active) || (OilPump.pressure < OilPump.pressure_minimum)) {
                engineisready = false;
                    if (OilPump.start_type == start_t::manual) {
                        // with manual pump control startup procedure is done only once per starter switch press
                        dizel_startup = false;
                }
        }
            // test the water circuits and water temperature
            if (true == dizel_heat.PA) {
                engineisready = false;
                // TBD, TODO: reset startup procedure depending on pump and heater control mode
                dizel_startup = false;
        }

        return engineisready;
    }

    // *************************************************************************************************
    // Q: 20160715
    // Aktualizacja stanu silnika
    // *************************************************************************************************
    bool TMoverParameters::dizel_Update(double dt) {

        WaterPumpCheck(dt);
        WaterHeaterCheck(dt);
        OilPumpCheck(dt);
        FuelPumpCheck(dt);
            if ((true == dizel_startup) && (true == dizel_StartupCheck())) {
                dizel_ignition = true;
        }

            if ((true == dizel_ignition) && (LastSwitchingTime >= InitialCtrlDelay)) {

                dizel_startup = false;
                dizel_ignition = false;
                // TODO: split engine and main circuit state indicator in two separate flags
                LastSwitchingTime = 0;
                Mains = true;
                dizel_spinup = true;
                enrot = std::max(
                        enrot,
                        0.35 * ( // TODO: dac zaleznie od temperatury i baterii
                                       EngineType == TEngineType::DieselEngine ? dizel_nmin : DElist[0].RPM / 60.0));
        }

        dizel_spinup = (dizel_spinup && Mains &&
                        (enrot < 0.95 * (EngineType == TEngineType::DieselEngine ? dizel_nmin : DElist[0].RPM / 60.0)));

            if ((true == Mains) && (false == FuelPump.is_active)) {
                // knock out the engine if the fuel pump isn't feeding it
                // TBD, TODO: grace period before the engine is starved for fuel and knocked out
                MainSwitch(false);
        }

        bool DU{false};

            if (EngineType == TEngineType::DieselEngine) {
                dizel_EngageChange(dt);
                DU = dizel_AutoGearCheck();
                double const fillspeed{2};
                dizel_fill = dizel_fill + fillspeed * dt * (dizel_fillcheck(MainCtrlPos, dt) - dizel_fill);
        }

        dizel_Heat(dt);

        return DU;
    }

    // *************************************************************************************************
    // Q: 20160715
    // oblicza napelnienie, uzwglednia regulator obrotow
    // *************************************************************************************************
    double TMoverParameters::dizel_fillcheck(int mcp, double dt) {
        auto realfill{0.0};

            if ((true == Mains) && (MainCtrlPosNo > 0) && (true == FuelPump.is_active)) {

                    if ((true == dizel_ignition) && (LastSwitchingTime >= 0.9 * InitialCtrlDelay)) {
                        // wzbogacenie przy rozruchu
                        // NOTE: ignition flag is reset before this code is executed
                        // TODO: sort this out
                        realfill = 1;
                    } else {
                            // napelnienie zalezne od MainCtrlPos
                            if (EIMCtrlType > 0) {
                                realfill = std::max(0.0, std::min(eimic_real, 1 - MotorParam[ScndCtrlActualPos].Isat));
                                    if (eimic_real > 0.005 && !hydro_TC_Lockup) {
                                        dizel_nreg_min =
                                                std::min(dizel_nreg_min + 2.5 * dt,
                                                         dizel_nmin_hdrive + eimic_real * dizel_nmin_hdrive_factor);
                                    } else {
                                        if (Vel < hydro_R_EngageVel && hydro_R && hydro_R_Fill > 0.01)
                                            dizel_nreg_min = std::min(dizel_nreg_min + 5.0 * dt, dizel_nmin_retarder);
                                        else
                                            dizel_nreg_min = dizel_nmin;
                                    }
                                    if (dizel_vel2nmax_Table.size() > 0 && !hydro_TC_Lockup) {
                                        dizel_nreg_max = std::min(std::min(dizel_nreg_max, enrot) + dizel_nreg_acc * dt,
                                                                  TableInterpolation(dizel_vel2nmax_Table, Vel));
                                    } else {
                                        dizel_nreg_max = dizel_nmax;
                                    }
                            } else {
                                dizel_nreg_max = dizel_nmax;
                                realfill = RList[mcp].R;
                            }
                    }
                    if (dizel_nmax_cutoff > 0) {
                        auto nreg{0.0};
                        if (EIMCtrlType > 0)
                            nreg = (eimic_real > 0.005 ? dizel_nreg_max : dizel_nmin);
                        else
                                switch (RList[MainCtrlPos].Mn) {
                                    case 0:
                                    case 1:
                                        nreg = dizel_nmin;
                                        break;
                                    case 2:
                                        if ((dizel_automaticgearstatus == 0) &&
                                            (true /*(!hydro_TC) || (dizel_engage>dizel_fill)*/))
                                            nreg = dizel_nreg_max;
                                        else
                                            nreg = dizel_nmin;
                                        break;
                                    case 3:
                                        if ((dizel_automaticgearstatus == 0) && (Vel > dizel_minVelfullengage))
                                            nreg = dizel_nreg_max;
                                        else
                                            nreg = dizel_nmin;
                                        break;
                                    case 4:
                                        if ((dizel_automaticgearstatus == 0) && (Vel > dizel_minVelfullengage))
                                            nreg = dizel_nmax;
                                        else
                                            nreg = dizel_nmin * 0.75 + dizel_nreg_max * 0.25;
                                        break;
                                    case 5:
                                        if (Vel > dizel_minVelfullengage)
                                            nreg = dizel_nreg_max;
                                        else
                                            nreg = dizel_nmin + 0.8 * (dizel_nreg_max - dizel_nmin) * RList[mcp].R;
                                        break;
                                    default:
                                        realfill = 0; // sluczaj
                                        break;
                                }
                        if (enrot > nreg) // nad predkoscia regulatora zeruj dawke
                            realfill = 0;
                        if (enrot < nreg) // pod predkoscia regulatora dawka zadana
                            realfill = realfill;
                        if ((enrot < dizel_nreg_min) &&
                            (RList[mcp].R > 0.001)) // jesli ponizej biegu jalowego i niezerowa dawka, to dawaj pelna
                            realfill = 1;
                }
        }

        return clamp(realfill, 0.0, 1.0);
    }

    // *************************************************************************************************
    // Q: 20160715
    // Oblicza moment siły wytwarzany przez silnik spalinowy
    // *************************************************************************************************
    double TMoverParameters::dizel_Momentum(double dizel_fill, double n,
                                            double dt) { // liczy moment sily wytwarzany przez silnik spalinowy}
        double Moment = 0, enMoment = 0, gearMoment = 0, eps = 0, newn = 0, friction = 0, neps = 0;
        double TorqueH = 0, TorqueL = 0, TorqueC = 0;
        n = n * CabActive;
        if ((MotorParam[ScndCtrlActualPos].mIsat < 0.001) || (DirActive == 0))
            n = enrot;
        friction = dizel_engagefriction;
        hydro_TC_nIn = enrot;          // wal wejsciowy przetwornika momentu
        hydro_TC_nOut = dizel_n_old;   // wal wyjsciowy przetwornika momentu
        neps = (n - dizel_n_old) / dt; // przyspieszenie katowe walu wejsciowego skrzyni biegow

            if (enrot > 0) {
                    if (dizel_Momentum_Table.size() > 1) {
                        Moment = TableInterpolation(dizel_Momentum_Table, enrot) * dizel_fill - dizel_Mstand;
                    } else {
                        Moment = (dizel_Mmax - (dizel_Mmax - dizel_Mnmax) *
                                                       square((enrot - dizel_nMmax) / (dizel_nMmax - dizel_nmax))) *
                                         dizel_fill -
                                 dizel_Mstand;
                    }
                Mm = Moment;
                dizel_FuelConsumptionActual = dizel_FuelConsumption * enrot * dizel_fill;
                dizel_FuelConsumptedTotal += dizel_FuelConsumptionActual * dt / 3600.0;
                if ((hydro_R) && (hydro_R_Placement == 2))
                    Moment -= dizel_MomentumRetarder(enrot, dt);
            } else {
                Moment = -dizel_Mstand;
            }
            if ((enrot < dizel_nmin / 10.0) && (eAngle < M_PI_2)) {
                // wstrzymywanie przy malych obrotach
                Moment -= dizel_Mstand;
        }
        if (true == dizel_spinup)
            Moment += dizel_Mstand / (0.3 + std::max(0.0, enrot / dizel_nmin)); // rozrusznik

        dizel_Torque = Moment;

            if (hydro_TC) // jesli przetwornik momentu
            {
                // napelnianie przetwornika
                bool IsPower = (EIMCtrlType > 0 ? eimic_real > 0.005 : MainCtrlPowerPos() > 0);
                if ((IsPower) && (Mains) && (enrot > dizel_nmin * 0.9))
                    hydro_TC_Fill += hydro_TC_FillRateInc * dt;
                // oproznianie przetwornika
                if (((!IsPower) && (Vel < dizel_maxVelANS)) || (!Mains) || (enrot < dizel_nmin * 0.8))
                    hydro_TC_Fill -= hydro_TC_FillRateDec * dt;
                // obcinanie zakresu
                hydro_TC_Fill = clamp(hydro_TC_Fill, 0.0, 1.0);

                    // blokowanie sprzegla blokującego
                    if ((Vel > hydro_TC_LockupSpeed) && (Mains) && (enrot > 0.9 * dizel_nmin) && (IsPower)) {
                        hydro_TC_Lockup = true;
                        hydro_TC_LockupRate += hydro_TC_FillRateInc * dt;
                }
                    // luzowanie sprzegla blokujacego
                    if ((Vel < (IsPower ? hydro_TC_LockupSpeed : hydro_TC_UnlockSpeed)) || (!Mains) ||
                        (enrot < 0.8 * dizel_nmin)) {
                        hydro_TC_Lockup = false;
                        hydro_TC_LockupRate -= hydro_TC_FillRateDec * dt;
                }
                // obcinanie zakresu
                hydro_TC_LockupRate = clamp(hydro_TC_LockupRate, 0.0, 1.0);
            } else {
                hydro_TC_Fill = 0.0;
                hydro_TC_LockupRate = 0.0;
            }

        // obliczanie momentow poszczegolnych sprzegiel
        // sprzeglo glowne (skrzynia biegow)
        TorqueC = dizel_engageMaxForce * dizel_engage * dizel_engageDia * friction;

            if (hydro_TC) // jesli hydro
            {
                double HydroTorque = 0;
                HydroTorque += hydro_TC_nIn * hydro_TC_nIn * hydro_TC_TorqueInIn;
                HydroTorque += (hydro_TC_nIn - hydro_TC_nOut) * hydro_TC_TorqueInOut;
                HydroTorque += hydro_TC_nOut * hydro_TC_nOut * hydro_TC_TorqueOutOut;
                double nOut2In = hydro_TC_nOut / std::max(0.01, hydro_TC_nIn);
                    if (hydro_TC_Table.size() > 1) {
                        hydro_TC_TMRatio = TableInterpolation(hydro_TC_Table, nOut2In);
                        hydro_TC_TorqueOut = HydroTorque * hydro_TC_Fill * hydro_TC_TMRatio;
                        hydro_TC_TorqueIn = HydroTorque * hydro_TC_Fill * std::min(1.0, hydro_TC_TMRatio);
                    } else {
                            if (nOut2In < hydro_TC_CouplingPoint) {
                                hydro_TC_TMRatio =
                                        1 + (hydro_TC_TMMax - 1) * square(1 - nOut2In / hydro_TC_CouplingPoint);
                                hydro_TC_TorqueIn = HydroTorque * hydro_TC_Fill;
                                hydro_TC_TorqueOut = HydroTorque * hydro_TC_Fill * hydro_TC_TMRatio;
                            } else {
                                hydro_TC_TMRatio = (1 - nOut2In) / (1 - hydro_TC_CouplingPoint);
                                hydro_TC_TorqueIn = HydroTorque * hydro_TC_Fill * hydro_TC_TMRatio;
                                hydro_TC_TorqueOut = HydroTorque * hydro_TC_Fill * hydro_TC_TMRatio;
                            }
                    }
                TorqueH = hydro_TC_TorqueOut;
                TorqueL = hydro_TC_LockupTorque * hydro_TC_LockupRate;
            } else {
                TorqueH = 0;               // brak przetwornika oznacza brak momentu
                TorqueL = 1 + TorqueC * 2; // zabezpieczenie, polaczenie trwale
            }

            // sprawdzanie dociskow poszczegolnych sprzegiel
            if (fabs(Moment) > Min0R(TorqueC, TorqueL + fabs(hydro_TC_TorqueIn)) ||
                (fabs(dizel_n_old - enrot) > 0.1)) // slizga sie z powodu roznic predkosci albo przekroczenia momentu
            {
                dizel_engagedeltaomega = enrot - dizel_n_old;

                    if (TorqueC > TorqueL) {
                            if (TorqueC > TorqueL + fabs(TorqueH)) {
                                hydro_TC_nOut = n;
                                gearMoment = TorqueL + fabs(TorqueH) * sign(dizel_engagedeltaomega);
                                enMoment = Moment - (TorqueL + fabs(hydro_TC_TorqueIn)) * sign(dizel_engagedeltaomega);
                            } else {
                                hydro_TC_nOut =
                                        enrot - (n - enrot) * (TorqueC - TorqueL) /
                                                        TorqueH; // slizganie proporcjonalne, zeby przetwornik nadrabial
                                gearMoment = TorqueC * sign(dizel_engagedeltaomega);
                                enMoment = Moment - gearMoment;
                            }

                    } else {
                        hydro_TC_nOut = enrot;
                        gearMoment = (TorqueC)*sign(dizel_engagedeltaomega);
                        enMoment = Moment - gearMoment;
                    }
                eps = enMoment / dizel_AIM;
                newn = enrot + eps * dt;
                if (((newn - n) * (enrot - dizel_n_old) < 0) &&
                    (TorqueC > 0.1)) // przejscie przez zero - slizgalo sie i przestało
                    newn = n;
                if ((newn * enrot <= 0) && (eps * enrot < 0)) // przejscie przez zero obrotow
                    newn = 0;
                enrot = newn;
            } else // nie slizga sie (jeszcze)
            {
                dizel_engagedeltaomega = 0;
                gearMoment = Moment;
                enMoment = 0;
                double enrot_min =
                        enrot - (Min0R(TorqueC, TorqueL + fabs(hydro_TC_TorqueIn)) - Moment) / dizel_AIM * dt;
                double enrot_max =
                        enrot + (Min0R(TorqueC, TorqueL + fabs(hydro_TC_TorqueIn)) + Moment) / dizel_AIM * dt;
                enrot = clamp(n, enrot_min, enrot_max);
            }
        if ((hydro_R) && (hydro_R_Placement == 1))
            gearMoment -= dizel_MomentumRetarder(hydro_TC_nOut, dt);


            if ((enrot <= 0) && (false == dizel_spinup)) {
                MainSwitch(false);
                enrot = 0;
        }

        dizel_n_old = n; // obecna predkosc katowa na potrzeby kolejnej klatki

        return gearMoment;
    }

    double TMoverParameters::dizel_MomentumRetarder(double n, double dt) {
        double RetarderRequest = (Mains ? std::max(0.0, -eimic_real) : 0);
        if (hydro_R_WithIndividual)
            RetarderRequest = LocalBrakeRatio();
        if (Vel < hydro_R_MinVel)
            RetarderRequest = 0;
            if ((hydro_R_Placement == 2) && (enrot < dizel_nmin)) {
                RetarderRequest = 0;
        }

        hydro_R_ClutchActive = (!hydro_R_Clutch) || (RetarderRequest > 0);
            if ((!hydro_R_Clutch) || ((hydro_R_ClutchActive) && (hydro_R_ClutchSpeed == 0))) {
                hydro_R_n = n * 60;
            } else if (hydro_R_ClutchActive) {
                hydro_R_n = sign(n) * std::min(std::fabs(hydro_R_n + hydro_R_ClutchSpeed * dt), std::fabs(n * 60));
            } else {
                hydro_R_n = 0;
            }
        n = hydro_R_n / 60.f;

            if (hydro_R_Fill < RetarderRequest) // gdy zadane hamowanie
            {
                hydro_R_Fill = std::min(hydro_R_Fill + hydro_R_FillRateInc * dt, RetarderRequest);
            } else {
                hydro_R_Fill = std::max(hydro_R_Fill - hydro_R_FillRateDec * dt, RetarderRequest);
            }

        double Moment = hydro_R_MaxTorque;
        double pwr = Moment * std::fabs(n) * M_PI * 2 * 0.001;
        if (pwr > hydro_R_MaxPower)
            Moment = Moment * hydro_R_MaxPower / pwr;
        double moment_in = n * n * hydro_R_TorqueInIn;
        Moment = std::min(moment_in, Moment * hydro_R_Fill);

        hydro_R_Torque = Moment;

        return Moment;
    }

    // sets component temperatures to specified value
    void TMoverParameters::dizel_HeatSet(float const Value) {

        dizel_heat.Te = // TODO: don't include ambient temperature, pull it from environment data instead
                dizel_heat.Ts = dizel_heat.To = dizel_heat.Tsr = dizel_heat.Twy = dizel_heat.Tsr2 = dizel_heat.Twy2 =
                        dizel_heat.temperatura1 = dizel_heat.temperatura2 = Value;
    }

    // calculates diesel engine temperature and heat transfers
    // adapted from scripts written by adamst
    // NOTE: originally executed twice per second
    void TMoverParameters::dizel_Heat(double const dt) {

        auto const qs{44700.0};
        auto const Cs{11000.0};
        auto const Cw{4.189};
        auto const Co{1.885};
        auto const gwmin{400.0};
        auto const gwmax{4000.0};
        auto const gwmin2{400.0};
        auto const gwmax2{4000.0};

        dizel_heat.Te = Global_AirTemperature;

        auto const engineon{(Mains ? 1 : 0)};
        auto const engineoff{(Mains ? 0 : 1)};
        auto const rpm{enrot * 60};
        // TODO: calculate this once and cache for further use, instead of doing it repeatedly all over the place
        auto const revolutionsfactor{EngineRPMRatio()};
        auto const waterpump{WaterPump.is_active ? 1 : 0};

        auto const gw = engineon * interpolate(gwmin, gwmax, revolutionsfactor) + waterpump * 1000 + engineoff * 200;
        auto const gw2 = engineon * interpolate(gwmin2, gwmax2, revolutionsfactor) + waterpump * 1000 + engineoff * 200;
        auto const gwO = interpolate(gwmin, gwmax, revolutionsfactor);

        dizel_heat.water.is_cold = ((dizel_heat.water.config.temp_min > 0) &&
                                    (dizel_heat.temperatura1 < dizel_heat.water.config.temp_min - (Mains ? 5 : 0)));
        dizel_heat.water.is_hot =
                ((dizel_heat.water.config.temp_max > 0) &&
                 (dizel_heat.temperatura1 > dizel_heat.water.config.temp_max - (dizel_heat.water.is_hot ? 8 : 0)));
        dizel_heat.water_aux.is_cold =
                ((dizel_heat.water_aux.config.temp_min > 0) &&
                 (dizel_heat.temperatura2 < dizel_heat.water_aux.config.temp_min - (Mains ? 5 : 0)));
        dizel_heat.water_aux.is_hot = ((dizel_heat.water_aux.config.temp_max > 0) &&
                                       (dizel_heat.temperatura2 >
                                        dizel_heat.water_aux.config.temp_max - (dizel_heat.water_aux.is_hot ? 8 : 0)));
        dizel_heat.oil.is_cold = ((dizel_heat.oil.config.temp_min > 0) &&
                                  (dizel_heat.To < dizel_heat.oil.config.temp_min - (Mains ? 5 : 0)));
        dizel_heat.oil.is_hot = ((dizel_heat.oil.config.temp_max > 0) &&
                                 (dizel_heat.To > dizel_heat.oil.config.temp_max - (dizel_heat.oil.is_hot ? 8 : 0)));

        auto const PT = ((false == dizel_heat.water.is_cold) && (false == dizel_heat.water.is_hot) &&
                         (false == dizel_heat.water_aux.is_cold) && (false == dizel_heat.water_aux.is_hot) &&
                         (false == dizel_heat.oil.is_cold) &&
                         (false == dizel_heat.oil.is_hot) /* && ( false == awaria_termostatow ) */) /* || PTp */;
        auto const PPT = (false == PT) /* && ( false == PPTp ) */;
        dizel_heat.PA = (/* ( ( !zamkniecie or niedomkniecie ) and !WBD ) || */
                         PPT /* || nurnik || ( woda < 7 ) */) /* && ( !PAp ) */;

        // engine heat transfers
        auto const Ge{engineon * (0.21 * dizel_heat.powerfactor * EnginePower + 12) / 3600};
        // TODO: replace fixed heating power cost with more accurate calculation
        auto const obciazenie{engineon *
                              ((dizel_heat.powerfactor * EnginePower / 950) + (Heating ? HeatingPower : 0) + 70)};
        auto const Qd{qs * Ge - obciazenie};
        // silnik oddaje czesc ciepla do wody chlodzacej, a takze pewna niewielka czesc do otoczenia, modyfikowane przez
        // okienko
        auto const Qs{(Qd - (dizel_heat.kfs * (dizel_heat.Ts - dizel_heat.Tsr)) -
                       (dizel_heat.kfe *
                        /* ( 0.3 + 0.7 * ( dizel_heat.okienko ? 1 : 0 ) ) * */ (dizel_heat.Ts - dizel_heat.Te)))};
        auto const dTss{Qs / Cs};
        dizel_heat.Ts += (dTss * dt);

        // oil heat transfers
        // olej oddaje cieplo do wody gdy krazy przez wymiennik ciepla == wlaczona pompka lub silnik
        auto const dTo{(dizel_heat.auxiliary_water_circuit ? ((dizel_heat.kfo * (dizel_heat.Ts - dizel_heat.To)) -
                                                              (dizel_heat.kfo2 * (dizel_heat.To - dizel_heat.Tsr2))) /
                                                                     (gwO * Co)
                                                           : ((dizel_heat.kfo * (dizel_heat.Ts - dizel_heat.To)) -
                                                              (dizel_heat.kfo2 * (dizel_heat.To - dizel_heat.Tsr))) /
                                                                     (gwO * Co))};
        dizel_heat.To += (dTo * dt);

        // heater
        /*
           if( typ == "SP45" )
           Qp = (float)( podgrzewacz and ( true == WaterPump.is_active ) and ( Twy < 55 ) and ( Twy2 < 55 ) ) * 1000;
           else
           */
        auto const Qp = (((true == WaterHeater.is_active) && (true == WaterPump.is_active) && (dizel_heat.Twy < 60) &&
                          (dizel_heat.Twy2 < 60))
                                 ? 1
                                 : 0) *
                        1000;

        auto const kurek07{1}; // unknown/unimplemented device TBD, TODO: identify and implement?

            if (true == dizel_heat.auxiliary_water_circuit) {
                // auxiliary water circuit setup
                dizel_heat.water_aux.is_warm =
                        ((true == dizel_heat.cooling) ||
                         ((true == Mains) &&
                          (BatteryVoltage >
                           (0.75 * NominalBatteryVoltage)) /* && !bezpompy && !awaria_chlodzenia && !WS10 */
                          && (dizel_heat.water_aux.config.temp_cooling > 0) &&
                          (dizel_heat.temperatura2 >
                           dizel_heat.water_aux.config.temp_cooling - (dizel_heat.water_aux.is_warm ? 8 : 0))));
                auto const PTC2{(dizel_heat.water_aux.is_warm /*or PTC2p*/ ? 1 : 0)};
                dizel_heat.rpmwz2 =
                        PTC2 * (dizel_heat.fan_speed >= 0 ? (rpm * dizel_heat.fan_speed) : (dizel_heat.fan_speed * -1));
                dizel_heat.zaluzje2 = (dizel_heat.water_aux.config.shutters
                                               ? (PTC2 == 1)
                                               : true); // no shutters is an equivalent to having them open
                auto const zaluzje2{(dizel_heat.zaluzje2 ? 1 : 0)};
                // auxiliary water circuit heat transfer values
                auto const kf2{kurek07 * ((dizel_heat.kw * (0.3 + 0.7 * zaluzje2)) * dizel_heat.rpmw2 +
                                          (dizel_heat.kv * (0.3 + 0.7 * zaluzje2) * Vel / 3.6)) +
                               2};
                auto const dTs2{((dizel_heat.kfo2 * (dizel_heat.To - dizel_heat.Tsr2))) / (gw2 * Cw)};
                // przy otwartym kurku B ma³y obieg jest dogrzewany przez du¿y - stosujemy przy korzystaniu z
                // podgrzewacza oraz w zimie
                auto const Qch2{-kf2 * (dizel_heat.Tsr2 - dizel_heat.Te) +
                                (80 * (true == WaterCircuitsLink ? 1 : 0) * (dizel_heat.Twy - dizel_heat.Tsr2))};
                auto const dTch2{Qch2 / (gw2 * Cw)};
                // auxiliary water circuit heat transfers finalization
                // NOTE: since primary circuit doesn't read data from the auxiliary one, we can pretty safely finalize
                // auxiliary updates before touching the primary circuit
                auto const Twe2{dizel_heat.Twy2 + (dTch2 * dt)};
                dizel_heat.Twy2 = Twe2 + (dTs2 * dt);
                dizel_heat.Tsr2 = 0.5 * (dizel_heat.Twy2 + Twe2);
                dizel_heat.temperatura2 = dizel_heat.Twy2;
        }
        // primary water circuit setup
        dizel_heat.water.is_flowing =
                ((dizel_heat.water.config.temp_flow < 0) ||
                 (dizel_heat.temperatura1 > dizel_heat.water.config.temp_flow - (dizel_heat.water.is_flowing ? 5 : 0)));
        auto const obieg{(dizel_heat.water.is_flowing ? 1 : 0)};
        dizel_heat.water.is_warm =
                ((true == dizel_heat.cooling) ||
                 ((true == Mains) &&
                  (BatteryVoltage > (0.75 * NominalBatteryVoltage)) /* && !bezpompy && !awaria_chlodzenia && !WS10 */
                  && (dizel_heat.water.config.temp_cooling > 0) &&
                  (dizel_heat.temperatura1 >
                   dizel_heat.water.config.temp_cooling - (dizel_heat.water.is_warm ? 8 : 0))));
        auto const PTC1{(dizel_heat.water.is_warm /*or PTC1p*/ ? 1 : 0)};
        dizel_heat.rpmwz =
                PTC1 * (dizel_heat.fan_speed >= 0 ? (rpm * dizel_heat.fan_speed) : (dizel_heat.fan_speed * -1));
        dizel_heat.zaluzje1 =
                (dizel_heat.water.config.shutters ? (PTC1 == 1)
                                                  : true); // no shutters is an equivalent to having them open
        auto const zaluzje1{(dizel_heat.zaluzje1 ? 1 : 0)};
        // primary water circuit heat transfer values
        auto const kf{obieg * kurek07 *
                              ((dizel_heat.kw * (0.3 + 0.7 * zaluzje1)) * dizel_heat.rpmw +
                               (dizel_heat.kv * (0.3 + 0.7 * zaluzje1) * Vel / 3.6) + 3) +
                      2};
        auto const dTs{(dizel_heat.auxiliary_water_circuit
                                ? ((dizel_heat.kfs * (dizel_heat.Ts - dizel_heat.Tsr))) / (gw * Cw)
                                : ((dizel_heat.kfs * (dizel_heat.Ts - dizel_heat.Tsr)) +
                                   (dizel_heat.kfo2 * (dizel_heat.To - dizel_heat.Tsr))) /
                                          (gw * Cw))};
        auto const Qch{-kf * (dizel_heat.Tsr - dizel_heat.Te) + Qp};
        auto const dTch{Qch / (gw * Cw)};
        // primary water circuit heat transfers finalization
        auto const Twe{dizel_heat.Twy + (dTch * dt)};
        dizel_heat.Twy = Twe + (dTs * dt);
        dizel_heat.Tsr = 0.5 * (dizel_heat.Twy + Twe);
        dizel_heat.temperatura1 = dizel_heat.Twy;
        /*
           fuelConsumed = fuelConsumed + ( Ge * 0.5 );

           while( fuelConsumed >= 0.83 ) {
           fuelConsumed = fuelConsumed - 0.83;
           fuelQueue.DestroyProductMatching( null, 1 );
           }//if

           if( engineon )
           temp_turbo = temp_turbo + 0.3 * ( t_pozycja );
           if( t_pozycja == 0 and cisnienie > 0.04 )
           temp_turbo = temp_turbo - 1;

           if( temp_turbo > 400 )
           temp_turbo = 400;
           if( temp_turbo < 0 )
           temp_turbo = 0;

           if( temp_turbo > 50 and cisnienie < 0.05 )
           timer_turbo = timer_turbo + 1;

           if( temp_turbo == 0 )
           timer_turbo = 0;

           if( timer_turbo > 360 ) {
           awaria_turbo = true;
           timer_turbo = 400;
           }

           if( Ts < 50 )
           p_odpal = 3;
           if( Ts > 49 and Ts < 76 )
           p_odpal = 4;
           if( Ts > 75 )
           p_odpal = 7;

           stukanie = stukanie or awaria_oleju;

           if( awaria_oleju == true and ilosc_oleju > 0 ) {
           ilosc_oleju = ilosc_oleju - ( 0.002 * rpm / 1500 );
           }
           if( awaria_oleju == true and cisnienie < 0.06 )
           damage = 1;
           */
    }

    bool TMoverParameters::AssignLoad(std::string const &Name, float const Amount) {

            if (Name == "pantstate") {
                    if (EnginePowerSource.SourceType == TPowerSource::CurrentCollector) {
                        // wartość niby "pantstate" - nazwa dla formalności, ważna jest ilość
                        auto const pantographsetup{static_cast<int>(Amount)};
                            if (pantographsetup & (1 << 2)) {
                                DoubleTr = -1;
                        }
                            if (pantographsetup & (1 << 0)) {
                                    if (DoubleTr == 1) {
                                        OperatePantographValve(end::front, operation_t::enable, range_t::local);
                                    } else {
                                        OperatePantographValve(end::rear, operation_t::enable, range_t::local);
                                    }
                        }
                            if (pantographsetup & (1 << 1)) {
                                    if (DoubleTr == 1) {
                                        OperatePantographValve(end::rear, operation_t::enable, range_t::local);
                                    } else {
                                        OperatePantographValve(end::front, operation_t::enable, range_t::local);
                                    }
                        }
                        return true;
                    } else {
                        return false;
                    }
        }

            if (Name.empty()) {
                // empty the vehicle if requested
                LoadTypeChange = (LoadType.name != Name);
                LoadType = load_attributes();
                LoadAmount = 0.f;
                return true;
        }
            // can't mix load types, at least for the time being
            if ((LoadAmount > 0) && (LoadType.name != Name)) {
                return false;
        }

            for (auto const &loadattributes: LoadAttributes) {
                    if (Name == loadattributes.name) {
                        LoadTypeChange = (LoadType.name != Name);
                        LoadType = loadattributes;
                        LoadAmount = clamp(Amount, 0.f, MaxLoad);
                        ComputeMass();
                        return true;
                }
            }
        // didn't find matching load configuration, this type is unsupported
        return false;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Test zakończenia załadunku / rozładunku
    // *************************************************************************************************
    bool TMoverParameters::LoadingDone(double const LSpeed, std::string const &Loadname) {

            if (LSpeed == 0.0) {
                // zerowa prędkość zmiany, to koniec
                LoadStatus = 4;
                return true;
        }

            if (Loadname.empty()) {
                return (LoadStatus >= 4);
        }
            if (Loadname != LoadType.name) {
                return (LoadStatus >= 4);
        }

        // test zakończenia załadunku/rozładunku
        // load exchange speed is reduced if the wagon is overloaded
        auto const loadchange{
                static_cast<float>(std::fabs(LSpeed * LastLoadChangeTime) * (LoadAmount > MaxLoad ? 0.5 : 1.0))};

            if (LSpeed < 0) {
                // gdy rozładunek
                LoadStatus = 2;         // trwa rozładunek (włączenie naliczania czasu)
                    if (loadchange > 0) // jeśli coś przeładowano
                    {
                        LastLoadChangeTime = 0;         // naliczony czas został zużyty
                        LoadAmount -= loadchange;       // zmniejszenie ilości ładunku
                        CommandIn.Value1 -= loadchange; // zmniejszenie ilości do rozładowania
                            if ((LoadAmount <= 0) || (CommandIn.Value1 <= 0)) {
                                // pusto lub rozładowano żądaną ilość
                                LoadStatus = 4;                               // skończony rozładunek
                                LoadAmount = clamp(LoadAmount, 0.f, MaxLoad); // ładunek nie może być ujemny
                        }
                            if (LoadAmount == 0.f) {
                                AssignLoad(""); // jak nic nie ma, to nie ma też nazwy
                        }
                        ComputeMass();
                }
            } else if (LSpeed > 0) {
                // gdy załadunek
                LoadStatus = 1;         // trwa załadunek (włączenie naliczania czasu)
                    if (loadchange > 0) // jeśli coś przeładowano
                    {
                        LastLoadChangeTime = 0;   // naliczony czas został zużyty
                        LoadAmount += loadchange; // zwiększenie ładunku
                        CommandIn.Value1 -= loadchange;
                            if ((LoadAmount >= MaxLoad * (1.0 + OverLoadFactor)) || (CommandIn.Value1 <= 0)) {
                                LoadStatus = 4; // skończony załadunek
                                LoadAmount = std::min<float>(MaxLoad * (1.0 + OverLoadFactor), LoadAmount);
                        }
                        ComputeMass();
                }
        }

        return (LoadStatus >= 4);
    }

    bool TMoverParameters::ChangeDoorPermitPreset(int const Change, range_t const Notify) {

        auto const initialstate{Doors.permit_preset};

            if (false == Doors.permit_presets.empty()) {

                Doors.permit_preset = clamp<int>(Doors.permit_preset + Change, 0, Doors.permit_presets.size() - 1);
                auto const doors{Doors.permit_presets[Doors.permit_preset]};
                auto const permitleft{((doors & 1) != 0)};
                auto const permitright{((doors & 2) != 0)};

                PermitDoors((CabActive > 0 ? side::left : side::right), permitleft, Notify);
                PermitDoors((CabActive > 0 ? side::right : side::left), permitright, Notify);
        }

        return (Doors.permit_preset != initialstate);
    }

    bool TMoverParameters::PermitDoorStep(bool const State, range_t const Notify) {

        auto const initialstate{Doors.step_enabled};

        Doors.step_enabled = State;
            if (Notify != range_t::local) {
                // wysłanie wyłączenia do pozostałych?
                SendCtrlToNext("DoorStep", (State == true ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (Doors.step_enabled != initialstate);
    }

    bool TMoverParameters::PermitDoors(side const Door, bool const State, range_t const Notify) {

        bool const initialstate{Doors.instances[Door].open_permit};

        PermitDoors_(Door, State);

            if (Notify != range_t::local) {

                SendCtrlToNext("DoorPermit",
                               (State ? 1 : -1) // positive: grant, negative: revoke
                                       * (Door == (CabActive > 0 ? side::left : side::right)
                                                  ? // 1=lewe, 2=prawe (swap if reversed)
                                                  1
                                                  : 2),
                               CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (Doors.instances[Door].open_permit != initialstate);
    }

    void TMoverParameters::PermitDoors_(side const Door, bool const State) {

            if ((State) && (State != Doors.instances[Door].open_permit)) {
                SetFlag(SoundFlag, sound::doorpermit);
        }
        Doors.instances[Door].open_permit = State;
    }

    bool TMoverParameters::ChangeDoorControlMode(bool const State, range_t const Notify) {

        auto const initialstate{Doors.remote_only};

        Doors.remote_only = State;
            if (Notify != range_t::local) {
                // wysłanie wyłączenia do pozostałych?
                SendCtrlToNext("DoorMode", (State == true ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

            if (true == State) {
                // when door are put in remote control mode they're automatically open
                // TBD, TODO: make it dependant on config switch?
                OperateDoors(side::left, true);
                OperateDoors(side::right, true);
        }

        return (Doors.remote_only != initialstate);
    }

    bool TMoverParameters::OperateDoors(side const Door, bool const State, range_t const Notify) {

        auto &door{Doors.instances[Door]};
        /*
           if( ( State == true ? door.is_open : door.is_closed ) ) {
        // TBD: should the command be passed to other vehicles regardless of whether it affected the primary target?
        // (for the time being no, methods are often invoked blindly which would lead to commands spam)
        return false;
        }
        */
        bool result{false};

            if (Notify == range_t::local) {
                door.local_open = State;
                door.local_close = (false == State);
                result = true;
            } else {
                    // remote door operation signals require power to propagate
                    if ((Power24vIsAvailable || Power110vIsAvailable)) {
                        door.remote_open = State;
                        door.remote_close = (false == State);
                        result = true;
                }
            }

            if (Notify != range_t::local) {

                SendCtrlToNext((State == true ? "DoorOpen" : "DoorClose"),
                               (Door == (CabActive > 0 ? side::left : side::right)
                                        ? // 1=lewe, 2=prawe (swap if reversed)
                                        1
                                        : 2),
                               CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return result;
    }

    // toggle door lock
    bool TMoverParameters::LockDoors(bool const State, range_t const Notify) {

        auto const initialstate{Doors.lock_enabled};

        Doors.lock_enabled = State;
            if (Notify != range_t::local) {
                // wysłanie wyłączenia do pozostałych?
                SendCtrlToNext("DoorLock", (State == true ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return (Doors.lock_enabled != initialstate);
    }

    // toggles departure warning
    bool TMoverParameters::signal_departure(bool const State, range_t const Notify) {

            if (DepartureSignal == State) {
                // TBD: should the command be passed to other vehicles regardless of whether it affected the primary
                // target?
                return false;
        }

        DepartureSignal = State;
            if (Notify != range_t::local) {
                // wysłanie wyłączenia do pozostałych?
                SendCtrlToNext("DepartureSignal", (State == true ? 1 : 0), CabActive,
                               (Notify == range_t::unit ? coupling::control | coupling::permanent : coupling::control));
        }

        return true;
    }

    // automatic door controller update
    void TMoverParameters::update_doors(double const Deltatime) {

            if (Doors.range == 0.f) {
                return;
        } // HACK: crude way to distinguish vehicles with actual doors

        // NBMX Obsluga drzwi, MC: zuniwersalnione
        auto const localopencontrol{(false == Doors.remote_only) && ((Doors.open_control == control_t::passenger) ||
                                                                     (Doors.open_control == control_t::mixed))};
        auto const remoteopencontrol{(Doors.open_control == control_t::driver) ||
                                     (Doors.open_control == control_t::conductor) ||
                                     (Doors.open_control == control_t::mixed)};
        auto const localclosecontrol{(false == Doors.remote_only) && ((Doors.close_control == control_t::passenger) ||
                                                                      (Doors.close_control == control_t::mixed))};
        auto const remoteclosecontrol{(Doors.close_control == control_t::driver) ||
                                      (Doors.close_control == control_t::conductor) ||
                                      (Doors.close_control == control_t::mixed)};

        Doors.is_locked = (true == Doors.has_lock) && (true == Doors.lock_enabled) && (Vel >= 10.0);

            for (auto &door: Doors.instances) {
                // revoke permit if...
                door.open_permit =
                        (true == door.open_permit) // ...we already have one...
                        && ((false ==
                             Doors.permit_presets.empty()) // ...there's no preset switch controlling permit state...
                            || ((false == Doors.is_locked) // ...and the door lock is engaged...
                                && (false == door.remote_close))); // ...or the door is about to be closed

                door.is_open = (door.position >= Doors.range) &&
                               ((false == Doors.step_enabled) ||
                                (door.step_position >= (Doors.step_range != 0.f ? 1.f : 0.f)));
                door.is_closed = (door.position <= 0.f) && (door.step_position <= 0.f);
                door.is_door_closed = (door.position <= 0.f);

                door.local_open = door.local_open && (false == door.is_open) &&
                                  ((false == Doors.permit_needed) || door.open_permit);
                door.remote_open = (door.remote_open || Doors.remote_only) && (false == door.is_open) &&
                                   ((false == Doors.permit_needed) || door.open_permit);
                door.local_close = door.local_close && (false == door.is_closed) &&
                                   ((false == remoteopencontrol) || (false == door.remote_open));
                door.remote_close = door.remote_close && (false == door.is_closed) &&
                                    ((false == localopencontrol) || (false == door.local_open));

                auto const autoopenrequest{(Doors.open_control == control_t::autonomous) &&
                                           ((false == Doors.permit_needed) || door.open_permit)};
                auto const openrequest{(localopencontrol && door.local_open) ||
                                       (remoteopencontrol && door.remote_open) ||
                                       (autoopenrequest && (false == door.is_open))};

                auto const autocloserequest{((Doors.auto_velocity != -1.f) && (Vel > Doors.auto_velocity)) ||
                                            ((door.auto_timer != -1.f) && (door.auto_timer <= 0.f)) ||
                                            ((Doors.permit_needed) && (false == door.open_permit))};
                auto const closerequest{(door.remote_close && remoteclosecontrol) ||
                                        (door.local_close && localclosecontrol) || (autocloserequest && door.is_open)};

                auto const ispowered{(Doors.voltage == 0     ? true
                                      : Doors.voltage == 24  ? (Power24vIsAvailable || Power110vIsAvailable)
                                      : Doors.voltage == 110 ? Power110vIsAvailable
                                                             : false)};

                door.is_opening = (false == door.is_open) && (true == ispowered) && (false == closerequest) &&
                                  ((true == door.is_opening) || ((true == openrequest) && (false == Doors.is_locked)));
                door.is_closing = (false == door.is_closed) && (true == ispowered) && (false == openrequest) &&
                                  (door.is_closing || closerequest);
                door.step_unfolding = ((Doors.step_range != 0.f) && (Doors.step_enabled) &&
                                       (false == Doors.is_locked) && (door.step_position < 1.f) && (door.is_opening));
                door.step_folding =
                        ((door.step_position > 0.f) // is unfolded
                         && ((false ==
                              Doors.step_enabled) // we lost permission to stay open or our door is calling the shots
                             || (Doors.permit_needed ? (false == door.open_permit) : door.is_closing)) &&
                         ((door.close_delay > Doors.close_delay) ||
                          door.position <= 0.f)); // door is about to close, or already done

                    if (true == door.is_opening) {
                        door.auto_timer = ((localopencontrol && door.local_open) ? Doors.auto_duration
                                           : (remoteopencontrol && door.remote_open && Doors.auto_include_remote)
                                                   ? Doors.auto_duration
                                                   : -1.f);
                }
                    // doors
                    if (true == door.is_opening) {
                            // open door
                            if ((false == door.step_unfolding) // no wait if no doorstep
                                || (Doors.step_type == 2)) {   // no wait for rotating doorstep
                                door.open_delay += Deltatime;
                                    if (door.open_delay > Doors.open_delay) {
                                        door.position = std::min<float>(Doors.range,
                                                                        door.position + Doors.open_rate * Deltatime);
                                }
                        }
                        door.close_delay = 0.f;
                }
                    if (true == door.is_closing) {
                        // close door
                        door.close_delay += Deltatime;
                            if (door.close_delay > Doors.close_delay) {
                                door.position = std::max<float>(0.f, door.position - Doors.close_rate * Deltatime);
                        }
                        door.open_delay = 0.f;
                }
                    // doorsteps
                    if (door.step_unfolding) {
                        // unfold left doorstep
                        door.step_position = std::min<float>(1.f, door.step_position + Doors.step_rate * Deltatime);
                }
                    if (door.step_folding) {
                            // fold left doorstep
                            if ((TrainType == dt_EZT) || (TrainType == dt_DMU)) {
                                    // multi-unit vehicles typically fold the doorstep only after closing the door
                                    if (door.position <= 0.f) {
                                        door.step_position =
                                                std::max<float>(0.f, door.step_position - Doors.step_rate * Deltatime);
                                }
                            } else {
                                door.step_position =
                                        std::max<float>(0.f, door.step_position - Doors.step_rate * Deltatime);
                            }
                }
            }

            if ((false == Doors.instances[side::right].is_open) && (false == Doors.instances[side::left].is_open)) {
                return;
        }

            if (Doors.auto_duration > 0.f) {
                    // update door timers if the door close after defined time
                    for (auto &door: Doors.instances) {

                            if (false == door.is_open) {
                                continue;
                        }

                            if (door.auto_timer > 0.f) {
                                door.auto_timer -= Deltatime;
                        }
                            // if there's load exchange in progress, reset the timer(s) for already open doors
                            if ((door.auto_timer != -1.f) && ((LoadStatus & (2 | 1)) != 0)) {
                                door.auto_timer = Doors.auto_duration;
                        }
                    }
        }
        /*
        // the door are closed if their timer goes below 0, or if the vehicle is moving faster than defined threshold
        std::array<side, 2> const doorids { side::right, side::left };
        for( auto const doorid : doorids ) {
        auto const &door { Doors.instances[ doorid ] };
        if( true == door.is_open ) {
        if( ( ( Doors.auto_velocity != -1.f ) && ( Vel > Doors.auto_velocity ) )
        || ( ( door.auto_timer != -1.f ) && ( door.auto_timer <= 0.f ) ) ) {
        // close the door and set the timer to expired state (closing may happen sooner if vehicle starts moving)
        OperateDoors( doorid, false, range_t::local );
        }
        }
        }
        */
    }

    // *************************************************************************************************
    // Q: 20160713
    // Przesuwa pojazd o podaną wartość w bok względem toru (dla samochodów)
    // *************************************************************************************************
    bool TMoverParameters::ChangeOffsetH(double DeltaOffset) {
        bool COH = false;
            if (TestFlag(CategoryFlag, 2) && TestFlag(RunningTrack.CategoryFlag, 2)) {
                OffsetTrackH = OffsetTrackH + DeltaOffset;
                //     if (fabs(OffsetTrackH) > (RunningTrack.Width / 1.95 - TrackW / 2.0))
                if (fabs(OffsetTrackH) > (0.5 * (RunningTrack.Width - Dim.W) - 0.05)) // Ra: może pół pojazdu od brzegu?
                    COH = false;                                                      // kola na granicy drogi
                else
                    COH = true;
        } else
            COH = false;

        return COH;
    }

    // *************************************************************************************************
    // Q: 20160713
    // Testuje zmienną (narazie tylko 0) i na podstawie uszkodzenia zwraca informację tekstową
    // *************************************************************************************************
    std::string TMoverParameters::EngineDescription(int what) const {
        std::string outstr{"OK"};
            switch (what) {
                    case 0: {
                            if (DamageFlag == 255) {
                                outstr = "WRECKED";
                            } else {
                                    if (TestFlag(DamageFlag, dtrain_thinwheel)) {
                                        if (Power > 0.1)
                                            outstr = "Thin wheel";
                                        else
                                            outstr = "Load shifted";
                                }
                                    if ((WheelFlat > 5.0) || (TestFlag(DamageFlag, dtrain_wheelwear))) {
                                        outstr = "Wheel wear";
                                }
                                    if (TestFlag(DamageFlag, dtrain_bearing)) {
                                        outstr = "Bearing damaged";
                                }
                                    if (TestFlag(DamageFlag, dtrain_coupling)) {
                                        outstr = "Coupler broken";
                                }
                                    if (TestFlag(DamageFlag, dtrain_loaddamage)) {
                                        if (Power > 0.1)
                                            outstr = "Ventilator damaged";
                                        else
                                            outstr = "Load damaged";
                                }
                                    if (TestFlag(DamageFlag, dtrain_loaddestroyed)) {
                                        if (Power > 0.1)
                                            outstr = "Engine damaged";
                                        else
                                            outstr = "LOAD DESTROYED";
                                }
                                    if (TestFlag(DamageFlag, dtrain_axle)) {
                                        outstr = "Axle broken";
                                }
                                    if (TestFlag(DamageFlag, dtrain_out)) {
                                        outstr = "DERAILED";
                                }
                            }
                        break;
                    }
                    default: {
                        outstr = "Invalid qualifier";
                        break;
                    }
            }
        return outstr;
    }

    // *************************************************************************************************
    // Q: 20160709
    // Funkcja zwracajaca napiecie dla calego skladu, przydatna dla EZT
    // *************************************************************************************************
    double TMoverParameters::GetTrainsetVoltage(
            int const Coupling) const { // ABu: funkcja zwracajaca napiecie dla calego skladu, przydatna dla EZT
                                        //  TBD, TODO: call once per vehicle update, return cached results?
        double voltages[] = {0.0, 0.0};
            for (int end = end::front; end <= end::rear; ++end) {
                    if (Couplers[end].Connected == nullptr) {
                        continue;
                }
                auto const &coupler{Couplers[end]};
                auto const fullcoupling{
                        coupler.CouplingFlag |
                        (TestFlag(coupler.CouplingFlag, coupler.PowerCoupling) ? coupler.PowerFlag : 0)};
                    if ((fullcoupling & Coupling) == 0) {
                        continue;
                }
                auto *connectedpowercoupling = ((Coupling & (coupling::highvoltage | coupling::heating)) != 0
                                                        ? &coupler.Connected->Couplers[coupler.ConnectedNr].power_high
                                                : (Coupling & coupling::power110v) != 0
                                                        ? &coupler.Connected->Couplers[coupler.ConnectedNr].power_110v
                                                : (Coupling & coupling::power24v) != 0
                                                        ? &coupler.Connected->Couplers[coupler.ConnectedNr].power_24v
                                                        : nullptr);
                    if ((connectedpowercoupling != nullptr) && (connectedpowercoupling->is_live)) {
                        voltages[end] = connectedpowercoupling->voltage;
                }
            }
        return std::max(voltages[end::front], voltages[end::rear]);
    }

    double TMoverParameters::GetTrainsetHighVoltage() const {

        return std::max(GetTrainsetVoltage(coupling::highvoltage),
                        (HeatingAllow ? GetTrainsetVoltage(coupling::heating) : 0.0));
    }

    // *************************************************************************************************
    // Kasowanie zmiennych pracy fizyki
    // *************************************************************************************************
    bool TMoverParameters::switch_physics(bool const State) // DO PRZETLUMACZENIA NA KONCU
    {
            if (PhysicActivation == State) {
                return false;
        }

        PhysicActivation = State;
            if (true == State) {
                LastSwitchingTime = 0;
        }

        return true;
    }

    // *************************************************************************************************
    // FUNKCJE PARSERA WCZYTYWANIA PLIKU FIZYKI POJAZDU
    // *************************************************************************************************
    bool startBPT;
    bool startMPT, startMPT0;
    bool startRLIST, startUCLIST;
    bool startDIZELMOMENTUMLIST, startDIZELV2NMAXLIST, startHYDROTCLIST, startPMAXLIST;
    bool startDLIST, startFFLIST, startWWLIST;
    bool startLIGHTSLIST;
    bool startCOMPRESSORLIST;
    int LISTLINE;

    bool issection(std::string const &Name, std::string const &Input) {

        return (Input.compare(0, Name.size(), Name) == 0);
    }

    int s2NPW(std::string s) { // wylicza ilosc osi napednych z opisu ukladu osi
        const char A = 64;
        int NPW = 0;
            for (std::size_t k = 0; k < s.size(); ++k) {
                if (s[k] >= (char)65 && s[k] <= (char)90)
                    NPW += s[k] - A;
            }
        return NPW;
    }

    int s2NNW(std::string s) { // wylicza ilosc osi nienapedzanych z opisu ukladu osi
        const char Zero = 48;
        int NNW = 0;
            for (std::size_t k = 0; k < s.size(); ++k) {
                if (s[k] >= (char)49 && s[k] <= (char)57)
                    NNW += s[k] - Zero;
            }
        return NNW;
    }

    // *************************************************************************************************
    // Q: 20160717
    // *************************************************************************************************
    // parsowanie Motor Param Table
    /*
    bool TMoverParameters::readMPT0(std::string const &line) {

        // TBD, TODO: split into separate functions similar to readMPT if more varied schemes appear?
        cParser parser(line);
            if (false == parser.getTokens(7, false)) {
                WriteLog("Read MPT0: arguments missing in line " + std::to_string(LISTLINE));
                return false;
        }
        int idx = 0; // numer pozycji
        parser >> idx;
            switch (EngineType) {
                    case TEngineType::DieselEngine: {
                        parser >> MotorParam[idx].mIsat >> MotorParam[idx].fi0 >> MotorParam[idx].fi >>
                                MotorParam[idx].mfi0 >> MotorParam[idx].mfi >> MotorParam[idx].Isat;
                        break;
                    }
                    default: {
                        parser >> MotorParam[idx].mfi >> MotorParam[idx].mIsat >> MotorParam[idx].mfi0 >>
                                MotorParam[idx].fi >> MotorParam[idx].Isat >> MotorParam[idx].fi0;
                        break;
                    }
            }
            if (true == parser.getTokens(1, false)) {
                int autoswitch;
                parser >> autoswitch;
                MotorParam[idx].AutoSwitch = (autoswitch == 1);
            } else {
                MotorParam[idx].AutoSwitch = false;
            }
        return true;
    }
    */

    /*
    bool TMoverParameters::readMPT(std::string const &line) {

        ++LISTLINE;

            switch (EngineType) {

                    case TEngineType::ElectricSeriesMotor: {
                        return readMPTElectricSeries(line);
                    }
                    case TEngineType::DieselElectric: {
                        return readMPTDieselElectric(line);
                    }
                    case TEngineType::DieselEngine: {
                        return readMPTDieselEngine(line);
                    }
                    default: {
                        return false;
                    }
            }
    }
    */

    /*
    bool TMoverParameters::readMPTElectricSeries(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(5, false)) {
                WriteLog("Read MPT: arguments missing in line " + std::to_string(LISTLINE));
                return false;
        }
        int idx = 0; // numer pozycji
        parser >> idx;
        parser >> MotorParam[idx].mfi >> MotorParam[idx].mIsat >> MotorParam[idx].fi >> MotorParam[idx].Isat;
            if (true == parser.getTokens(1, false)) {
                int autoswitch;
                parser >> autoswitch;
                MotorParam[idx].AutoSwitch = (autoswitch == 1);
            } else {
                MotorParam[idx].AutoSwitch = false;
            }
        return true;
    }
    */

    /*
    bool TMoverParameters::readMPTDieselElectric(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(7, false)) {
                WriteLog("Read MPT: arguments missing in line " + std::to_string(LISTLINE));
                return false;
        }
        int idx = 0; // numer pozycji
        parser >> idx;
        parser >> MotorParam[idx].mfi >> MotorParam[idx].mIsat >> MotorParam[idx].fi >> MotorParam[idx].Isat >>
                MPTRelay[idx].Iup >> MPTRelay[idx].Idown;

        return true;
    }

    bool TMoverParameters::readMPTDieselEngine(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(4, false)) {
                WriteLog("Read MPT: arguments missing in line " + std::to_string(LISTLINE));
                return false;
        }
        int idx = 0; // numer pozycji
        parser >> idx;
        parser >> MotorParam[idx].mIsat >> MotorParam[idx].fi >> MotorParam[idx].mfi;
            if (true == parser.getTokens(1, false)) {
                int autoswitch;
                parser >> autoswitch;
                MotorParam[idx].AutoSwitch = (autoswitch == 1);
            } else {
                MotorParam[idx].AutoSwitch = false;
            }
        return true;
    }

    bool TMoverParameters::readBPT(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(5, false)) {

                WriteLog("Read BPT: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        ++LISTLINE;
        std::string braketype;
        int idx = 0;
        parser >> idx;
        parser >> BrakePressureTable[idx].PipePressureVal >> BrakePressureTable[idx].BrakePressureVal >>
                BrakePressureTable[idx].FlowSpeedVal >> braketype;
            if (braketype == "Pneumatic") {
                BrakePressureTable[idx].BrakeType = TBrakeSystem::Pneumatic;
            } else if (braketype == "ElectroPneumatic") {
                BrakePressureTable[idx].BrakeType = TBrakeSystem::ElectroPneumatic;
            } else {
                BrakePressureTable[idx].BrakeType = TBrakeSystem::Individual;
            }

        return true;
    }

    bool TMoverParameters::readRList(std::string const &Input) {

        cParser parser(Input);
            if (false == parser.getTokens(5, false)) {

                WriteLog("Read RList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        auto idx = LISTLINE++;
            if (idx >= sizeof(RList) / sizeof(TScheme)) {
                WriteLog("Read RList: number of entries exceeded capacity of the data table");
                return false;
        }
        parser >> RList[idx].Relay >> RList[idx].R >> RList[idx].Bn >> RList[idx].Mn >> RList[idx].AutoSwitch;

            if (true == parser.getTokens(1, false)) {
                parser >> RList[idx].ScndAct;
            } else {
                RList[idx].ScndAct = 0;
            }

        return true;
    }

    bool TMoverParameters::readUCList(std::string const &line) {

        cParser parser(line);
        parser.getTokens(10, false);
        auto idx = LISTLINE++;
            if (idx >= sizeof(UniCtrlList) / sizeof(TUniversalCtrl)) {
                WriteLog("Read UCList: number of entries exceeded capacity of the data table");
                return false;
        }
        int i = 0;
        parser >> i >> UniCtrlList[idx].mode >> UniCtrlList[idx].MinCtrlVal >> UniCtrlList[idx].MaxCtrlVal >>
                UniCtrlList[idx].SetCtrlVal >> UniCtrlList[idx].SpeedUp >> UniCtrlList[idx].SpeedDown >>
                UniCtrlList[idx].ReturnPosition >> UniCtrlList[idx].NextPosFastInc >> UniCtrlList[idx].PrevPosFastDec;

        return true;
    }

    bool TMoverParameters::readDList(std::string const &line) {

        cParser parser(line);
        parser.getTokens(3, false);
        auto idx = LISTLINE++;
            if (idx >= sizeof(RList) / sizeof(TScheme)) {
                WriteLog("Read DList: number of entries exceeded capacity of the data table");
                return false;
        }
        parser >> RList[idx].Relay >> RList[idx].R >> RList[idx].Mn;

        return true;
    }

    bool TMoverParameters::readDMList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(2, false)) {

                WriteLog("Read DMList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        auto idx = LISTLINE++;
        double x = 0.0;
        double y = 0.0;
        parser >> x >> y;

        dizel_Momentum_Table.emplace(x / 60.0, y);

        return true;
    }

    bool TMoverParameters::readV2NMAXList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(2, false)) {

                WriteLog("Read V2nmaxList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        auto idx = LISTLINE++;
        double x = 0.0;
        double y = 0.0;
        parser >> x >> y;

        dizel_vel2nmax_Table.emplace(x, y / 60.0);

        return true;
    }

    bool TMoverParameters::readHTCList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(2, false)) {

                WriteLog("Read HTCList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        auto idx = LISTLINE++;
        double x = 0.0;
        double y = 0.0;
        parser >> x >> y;

        hydro_TC_Table.emplace(x, y);

        return true;
    }

    bool TMoverParameters::readPmaxList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(2, false)) {

                WriteLog("Read PmaxList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        auto idx = LISTLINE++;
        double x = 0.0;
        double y = 0.0;
        parser >> x >> y;

        EIM_Pmax_Table.emplace(x, y);

        return true;
    }

    bool TMoverParameters::readFFList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(2, false)) {
                WriteLog("Read FList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        int idx = LISTLINE++;
            if (idx >= sizeof(DElist) / sizeof(TDEScheme)) {
                WriteLog("Read FList: number of entries exceeded capacity of the data table");
                return false;
        }
        parser >> DElist[idx].RPM >> DElist[idx].GenPower;

        return true;
    }

    // parsowanie WWList
    bool TMoverParameters::readWWList(std::string const &line) {

        cParser parser(line);
            if (false == parser.getTokens(4, false)) {
                WriteLog("Read WWList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        int idx = LISTLINE++;
            if (idx >= sizeof(DElist) / sizeof(TDEScheme)) {
                WriteLog("Read WWList: number of entries exceeded capacity of the data table");
                return false;
        }
        parser >> DElist[idx].RPM >> DElist[idx].GenPower >> DElist[idx].Umax >> DElist[idx].Imax;

            if (true == parser.getTokens(3, false)) {
                // optional parameters for shunt mode
                parser >> SST[idx].Umin >> SST[idx].Umax >> SST[idx].Pmax;

                SST[idx].Pmin = std::sqrt(std::pow(SST[idx].Umin, 2) / 47.6);
                SST[idx].Pmax = std::min(SST[idx].Pmax, std::pow(SST[idx].Umax, 2) / 47.6);
        }

        return true;
    }

    bool TMoverParameters::readLightsList(std::string const &Input) {

        cParser parser(Input);
            if (false == parser.getTokens(2, false)) {
                WriteLog("Read LightsList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        int idx = LISTLINE++;
            if (idx > 16) {
                WriteLog("Read LightsList: number of entries exceeded capacity of the data table");
                return false;
        }
        parser >> Lights[0][idx] >> Lights[1][idx];

        return true;
    }

    bool TMoverParameters::readCompressorList(std::string const &Input) {

        cParser parser(Input);
            if (false == parser.getTokens(4, false)) {
                WriteLog("Read CompressorList: arguments missing in line " + std::to_string(LISTLINE + 1));
                return false;
        }
        int idx = LISTLINE++;
            if (idx > 8 - 1) {
                WriteLog("Read CompressorList: number of entries exceeded capacity of the data table");
                return false;
        }
        // NOTE: content of slot [x][0] is hardcoded elsewhere
        parser >> CompressorList[0][idx + 1] >> CompressorList[1][idx + 1] >> CompressorList[2][idx + 1] >>
                CompressorList[3][idx + 1];

        return true;
    }
    */

    // *************************************************************************************************
    // Q: 20160719
    // *************************************************************************************************
    void TMoverParameters::BrakeValveDecode(std::string const &Valve) {

        std::map<std::string, TBrakeValve> valvetypes{{"W", TBrakeValve::W},
                                                      {"W_Lu_L", TBrakeValve::W_Lu_L},
                                                      {"W_Lu_XR", TBrakeValve::W_Lu_XR},
                                                      {"W_Lu_VI", TBrakeValve::W_Lu_VI},
                                                      {"K", TBrakeValve::K},
                                                      {"Kg", TBrakeValve::Kg},
                                                      {"Kp", TBrakeValve::Kp},
                                                      {"Kss", TBrakeValve::Kss},
                                                      {"Kkg", TBrakeValve::Kkg},
                                                      {"Kkp", TBrakeValve::Kkp},
                                                      {"Kks", TBrakeValve::Kks},
                                                      {"Hikp1", TBrakeValve::Hikp1},
                                                      {"Hikss", TBrakeValve::Hikss},
                                                      {"Hikg1", TBrakeValve::Hikg1},
                                                      {"KE", TBrakeValve::KE},
                                                      {"SW", TBrakeValve::SW},
                                                      {"EStED", TBrakeValve::EStED},
                                                      {"NESt3", TBrakeValve::NESt3},
                                                      {"ESt3", TBrakeValve::ESt3},
                                                      {"LSt", TBrakeValve::LSt},
                                                      {"ESt4", TBrakeValve::ESt4},
                                                      {"ESt3AL2", TBrakeValve::ESt3AL2},
                                                      {"EP1", TBrakeValve::EP1},
                                                      {"EP2", TBrakeValve::EP2},
                                                      {"M483", TBrakeValve::M483},
                                                      {"CV1_L_TR", TBrakeValve::CV1_L_TR},
                                                      {"CV1", TBrakeValve::CV1},
                                                      {"CV1_R", TBrakeValve::CV1_R}};
        auto lookup = valvetypes.find(Valve);
        BrakeValve = lookup != valvetypes.end() ? lookup->second : TBrakeValve::Other;

            if ((BrakeValve == TBrakeValve::Other) && (contains(Valve, "ESt"))) {

                BrakeValve = TBrakeValve::ESt3;
        }
    }

    // *************************************************************************************************
    // Q: 20160719
    // *************************************************************************************************
    void TMoverParameters::BrakeSubsystemDecode() {
        BrakeSubsystem = TBrakeSubSystem::ss_None;
            switch (BrakeValve) {
                case TBrakeValve::W:
                case TBrakeValve::W_Lu_L:
                case TBrakeValve::W_Lu_VI:
                case TBrakeValve::W_Lu_XR:
                    BrakeSubsystem = TBrakeSubSystem::ss_W;
                    break;
                case TBrakeValve::ESt3:
                case TBrakeValve::ESt3AL2:
                case TBrakeValve::ESt4:
                case TBrakeValve::EP2:
                case TBrakeValve::EP1:
                    BrakeSubsystem = TBrakeSubSystem::ss_ESt;
                    break;
                case TBrakeValve::KE:
                    BrakeSubsystem = TBrakeSubSystem::ss_KE;
                    break;
                case TBrakeValve::CV1:
                case TBrakeValve::CV1_L_TR:
                    BrakeSubsystem = TBrakeSubSystem::ss_Dako;
                    break;
                case TBrakeValve::LSt:
                case TBrakeValve::EStED:
                    BrakeSubsystem = TBrakeSubSystem::ss_LSt;
                    break;
            }
    }

    // *************************************************************************************************
    // Q: 20160717
    // *************************************************************************************************

    bool TMoverParameters::CheckLocomotiveParameters(bool ReadyFlag, int Dir) {
        // WriteLog("check locomotive parameters...");
        int b;
        bool OK = true;

        AutoRelayFlag = (AutoRelayType == 1);

            if (NominalBatteryVoltage == 0.0) {
                BatteryStart = start_t::disabled;
        }

        Sand = SandCapacity;

            // NOTE: for diesel-powered vehicles we automatically convert legacy "main" power source to more accurate
            // "engine"
            if ((CompressorPower == 0) &&
                ((EngineType == TEngineType::DieselEngine) || (EngineType == TEngineType::DieselElectric))) {
                CompressorPower = 3;
        }

        // WriteLog("aa = " + AxleArangement + " " + std::string( Pos("o", AxleArangement)) );

            if ((contains(AxleArangement, "o")) && (EngineType == TEngineType::ElectricSeriesMotor)) {
                // test poprawnosci ilosci osi indywidualnie napedzanych
                OK = ((RList[1].Bn * RList[1].Mn) == NPoweredAxles);
                // WriteLogSS("aa ok", BoolToYN(OK));
        }

        if (BrakeSystem == TBrakeSystem::Individual)
            if (BrakeSubsystem != TBrakeSubSystem::ss_None)
                OK = false; //!

            if ((BrakeVVolume == 0) && (MaxBrakePress[3] > 0) && (BrakeSystem != TBrakeSystem::Individual)) {
                BrakeVVolume = MaxBrakePress[3] / (5.0 - MaxBrakePress[3]) *
                               (BrakeCylRadius * BrakeCylRadius * BrakeCylDist * BrakeCylNo * M_PI) * 1000;
        }
            if (BrakeVVolume == 0.0) {
                BrakeVVolume = 0.01;
        }
        // WriteLog("BVV = "  + FloatToStr(BrakeVVolume));


            switch (BrakeValve) {
                case TBrakeValve::W:
                    case TBrakeValve::K: {
                        // WriteLog("XBT W, K");
                        Hamulec = std::make_shared<TWest>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                          BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        if (MBPM < 2) // jesli przystawka wazaca
                            Hamulec->SetLP(0, MaxBrakePress[3], 0);
                        else
                            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                        break;
                    }
                    case TBrakeValve::KE: {
                        // WriteLog("XBT WKE");
                        Hamulec = std::make_shared<TKE>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                        BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        Hamulec->SetRM(RapidMult);
                        if (MBPM < 2) // jesli przystawka wazaca
                            Hamulec->SetLP(0, MaxBrakePress[3], 0);
                        else
                            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                        break;
                    }
                case TBrakeValve::NESt3:
                case TBrakeValve::ESt3:
                case TBrakeValve::ESt3AL2:
                    case TBrakeValve::ESt4: {
                        // WriteLog("XBT NESt3, ESt3, ESt3AL2, ESt4");
                        Hamulec = std::make_shared<TNESt3>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                           BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        static_cast<TNESt3 *>(Hamulec.get())->SetSize(BrakeValveSize, BrakeValveParams);
                        if (MBPM < 2) // jesli przystawka wazaca
                            Hamulec->SetLP(0, MaxBrakePress[3], 0);
                        else
                            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                        break;
                    }
                    case TBrakeValve::LSt: {
                        // WriteLog("XBT LSt");
                        Hamulec = std::make_shared<TLSt>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                         BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        Hamulec->SetRM(RapidMult);
                        break;
                    }
                    case TBrakeValve::EStED: {
                        // WriteLog("XBT EStED");
                        Hamulec = std::make_shared<TEStED>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                           BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        Hamulec->SetRM(RapidMult);
                            if (MBPM < 2) {
                                // jesli przystawka wazaca
                                Hamulec->SetLP(0, MaxBrakePress[3], 0);
                            } else {
                                Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                            }
                        break;
                    }
                    case TBrakeValve::EP2: {
                        // WriteLog("XBT EP2");
                        Hamulec =
                                std::make_shared<TEStEP2>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                          BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                        break;
                    }
                    case TBrakeValve::EP1: {
                        // WriteLog("XBT EP1");
                        Hamulec =
                                std::make_shared<TEStEP1>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                          BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
                        Hamulec->SetRM(RapidMult);
                        break;
                    }
                    case TBrakeValve::CV1: {
                        // WriteLog("XBT CV1");
                        Hamulec = std::make_shared<TCV1>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                         BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        break;
                    }
                    case TBrakeValve::CV1_L_TR: {
                        // WriteLog("XBT CV1_L_T");
                        Hamulec =
                                std::make_shared<TCV1L_TR>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                           BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
                        break;
                    }
                default:
                    Hamulec = std::make_shared<TBrake>(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                                                       BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
            }

        Hamulec->SetASBP(MaxBrakePress[4]);
        Hamulec->SetRV(RapidVel);

            switch (BrakeHandle) {
                case TBrakeHandle::FV4a:
                    Handle = std::make_shared<TFV4aM>();
                    break;
                case TBrakeHandle::MHZ_EN57:
                case TBrakeHandle::MHZ_K8P:
                    Handle = std::make_shared<TMHZ_EN57>();
                    break;
                case TBrakeHandle::FVel6:
                    Handle = std::make_shared<TFVel6>();
                    break;
                case TBrakeHandle::FVE408:
                    Handle = std::make_shared<TFVE408>();
                    break;
                case TBrakeHandle::testH:
                    Handle = std::make_shared<Ttest>();
                    break;
                case TBrakeHandle::M394:
                    Handle = std::make_shared<TM394>();
                    break;
                case TBrakeHandle::Knorr:
                    Handle = std::make_shared<TH14K1>();
                    break;
                case TBrakeHandle::St113:
                    Handle = std::make_shared<TSt113>();
                    break;
                case TBrakeHandle::MHZ_K5P:
                    Handle = std::make_shared<TMHZ_K5P>();
                    break;
                case TBrakeHandle::MHZ_6P:
                    Handle = std::make_shared<TMHZ_6P>();
                    break;
                default:
                    Handle = std::make_shared<TDriverHandle>();
            }
        Handle->SetParams(Handle_AutomaticOverload, Handle_ManualOverload, Handle_GenericDoubleParameter1,
                          Handle_GenericDoubleParameter2, Handle_OverloadMaxPressure, Handle_OverloadPressureDecrease);

            switch (BrakeLocHandle) {
                    case TBrakeHandle::FD1: {
                        LocHandle = std::make_shared<TFD1>();
                        LocHandle->Init(MaxBrakePress[0]);
                            if (TrainType == dt_EZT) {

                                dynamic_cast<TFD1 *>(LocHandle.get())->SetSpeed(3.5);
                        }
                        break;
                    }
                    case TBrakeHandle::Knorr: {
                        LocHandle = std::make_shared<TH1405>();
                        LocHandle->Init(MaxBrakePress[0]);
                        break;
                    }
                default:
                    LocHandle = std::make_shared<TDriverHandle>();
            }

        if ((true == TestFlag(BrakeDelays, bdelay_G)) &&
            ((false == TestFlag(BrakeDelays, bdelay_R)) ||
             (Power > 1.0))) // ustalanie srednicy przewodu glownego (lokomotywa lub napędowy
            Spg = 0.792;
        else
            Spg = 0.507;
        // WriteLog("SPG = " + FloatToStr(Spg));

        Pipe = std::make_shared<TReservoir>();
        Pipe2 = std::make_shared<TReservoir>();                   // zabezpieczenie, bo sie PG wywala... :(
        Pipe->CreateCap((std::max(Dim.L, 14.0) + 0.5) * Spg * 1); // dlugosc x przekroj x odejscia i takie tam
        Pipe2->CreateCap((std::max(Dim.L, 14.0) + 0.5) * Spg * 1);
            if (!SpringBrake.Cylinder) {
                SpringBrake.Cylinder = std::make_shared<TReservoir>();
                SpringBrake.Cylinder->CreateCap(1);
        }

        if (LightsPosNo > 0)
            LightsPos = LightsDefPos;
        if (CompressorListPosNo > 0)
            CompressorListPos = CompressorListDefPos;

            // NOTE: legacy compatibility behaviour for vehicles without defined heating power source
            if ((EnginePowerSource.SourceType == TPowerSource::CurrentCollector) &&
                (HeatingPowerSource.SourceType == TPowerSource::NotDefined)) {
                HeatingPowerSource.SourceType = TPowerSource::Main;
        }
            if ((HeatingPowerSource.SourceType == TPowerSource::NotDefined) && (HeatingPower > 0)) {
                HeatingPowerSource.SourceType = TPowerSource::PowerCable;
                HeatingPowerSource.PowerType = TPowerType::ElectricPower;
        }

            // checking ready flag
            // to dac potem do init
            if (ReadyFlag) // gotowy do drogi
            {
                // WriteLog("Ready to depart");
                CompressedVolume = VeselVolume * MinCompressor * (9.8) / 10.0;
                ScndPipePress = (VeselVolume > 0.0 ? CompressedVolume / VeselVolume
                                 : (Couplers[end::front].AllowedFlag & coupling::mainhose) != 0 ? 5.0
                                 : (Couplers[end::rear].AllowedFlag & coupling::mainhose) != 0  ? 5.0
                                                                                                : 0.0);
                PipePress = CntrlPipePress;
                BrakePress = 0.0;
                LocalBrakePosA = 0.0;
                if (CabActive == 0)
                    BrakeCtrlPos = static_cast<int>(Handle->GetPos(bh_NP));
                else
                    BrakeCtrlPos = static_cast<int>(Handle->GetPos(bh_RP));
                /*
                // NOTE: disabled and left up to the driver, if there's any
                MainSwitch( false );
                PantFront( true );
                PantRear( true );
                MainSwitch( true );
                */
                DirActive = 0;                       // Dir; //nastawnik kierunkowy - musi być ustawiane osobno!
                DirAbsolute = DirActive * CabActive; // kierunek jazdy względem sprzęgów
                LimPipePress = CntrlPipePress;

                Battery = (BatteryStart != start_t::disabled);
            } else { // zahamowany}
                // WriteLog("Braked");
                Volume = BrakeVVolume * MaxBrakePress[3];
                CompressedVolume = VeselVolume * MinCompressor * 0.55;
                /*
                   ScndPipePress = 5.1;
                   */
                ScndPipePress = (VeselVolume > 0.0 ? CompressedVolume / VeselVolume
                                 : (Couplers[end::front].AllowedFlag & coupling::mainhose) != 0 ? 5.1
                                 : (Couplers[end::rear].AllowedFlag & coupling::mainhose) != 0  ? 5.1
                                                                                                : 0.0);
                PipePress = LowPipePress;
                PipeBrakePress = MaxBrakePress[3] * 0.5;
                BrakePress = MaxBrakePress[3] * 0.5;
                LocalBrakePosA = 0.0;
                BrakeCtrlPos = static_cast<int>(Handle->GetPos(bh_NP));
                LimPipePress = LowPipePress;
                    if ((LocalBrake == TLocalBrake::ManualBrake) || (MBrake == true)) {
                        IncManualBrakeLevel(ManualBrakePosNo);
                }
                    if (SpringBrake.MaxBrakeForce > 0.0) {
                        SpringBrake.Activate = true;
                }
            }

        ActFlowSpeed = 0.0;
        BrakeCtrlPosR = BrakeCtrlPos;

        if (BrakeLocHandle == TBrakeHandle::Knorr)
            LocalBrakePosA = 0.5;

        Pipe->CreatePress(PipePress);
        Pipe2->CreatePress(ScndPipePress);
        Pipe->Act();
        Pipe2->Act();

        EqvtPipePress = PipePress;

        Handle->Init(PipePress);

        ComputeConstans();

            if (LoadFlag > 0) {

                    if (LoadAmount < MaxLoad * 0.45) {
                        IncBrakeMult();
                        IncBrakeMult();
                        DecBrakeMult(); // TODO: przeinesiono do mover.cpp
                        if (LoadAmount < MaxLoad * 0.35)
                            DecBrakeMult();
                    } else {
                        IncBrakeMult(); // TODO: przeinesiono do mover.cpp
                        if (LoadAmount >= MaxLoad * 0.55)
                            IncBrakeMult();
                    }
        }

        // taki mini automat - powinno byc ladnie dobrze :)
        BrakeDelayFlag = bdelay_P;
        if ((TestFlag(BrakeDelays, bdelay_G)) && !(TestFlag(BrakeDelays, bdelay_R)))
            BrakeDelayFlag = bdelay_G;
        if ((TestFlag(BrakeDelays, bdelay_R)) && !(TestFlag(BrakeDelays, bdelay_G)))
            BrakeDelayFlag = bdelay_R;
        /*
        // disabled, as test mode is used in specific situations and not really a default
        if (BrakeOpModes & bom_PS)
        BrakeOpModeFlag = bom_PS;
        else
        */
        BrakeOpModeFlag = bom_PN;

        // yB: jesli pojazdy nie maja zadeklarowanych czasow, to wsadz z przepisow +-16,(6)%
        int DefBrakeTable[8] = {15, 4, 25, 25, 13, 3, 12, 2};

            for (b = 1; b < 4; b++) {
                if (BrakeDelay[b] == 0)
                    BrakeDelay[b] = DefBrakeTable[b];
                BrakeDelay[b] = BrakeDelay[b] * (2.5 + Random(0.0, 0.2)) / 3.0;
            }

        Hamulec->Init(PipePress, HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        /*
           ScndPipePress = Compressor;
           */
        // WriteLogSS("OK=", BoolTo10(OK));
        // WriteLog("");

            if (EIMCtrlType == 3) {
                    for (auto idx = 0; idx < MainCtrlPosNo; ++idx) {
                            if (UniCtrlList[idx].MaxCtrlVal > 0.0) {
                                UniCtrlNoPowerPos = std::max(0, (idx - 1));
                                break;
                        }
                    }
        }

            // security system
            // by default place the magnet in the vehicle centre
            if (SecuritySystem.MagnetLocation == 0) {
                SecuritySystem.MagnetLocation = Dim.L / 2 - 0.5;
        }
        SecuritySystem.MagnetLocation = clamp(SecuritySystem.MagnetLocation, 0.0, Dim.L);

        return OK;
    }

    // *************************************************************************************************
    // Q: 20160714
    // Wstawia komendę z parametrem, od sprzęgu i w lokalizacji do pojazdu
    // *************************************************************************************************
    void TMoverParameters::PutCommand(std::string NewCommand, double NewValue1, double NewValue2,
                                      const TLocation &NewLocation) {
        CommandLast = NewCommand; // zapamiętanie komendy

        CommandIn.Command = NewCommand;
        CommandIn.Value1 = NewValue1;
        CommandIn.Value2 = NewValue2;
        CommandIn.Location = NewLocation;
        // czy uruchomic tu RunInternalCommand? nie wiem
    }

    // *************************************************************************************************
    // Q: 20160714
    // Pobiera komendę z parametru funkcji oraz wartość zmiennej jako return
    // *************************************************************************************************
    double TMoverParameters::GetExternalCommand(std::string &Command) {
        Command = CommandOut;
        return ValueOut;
    }

    // *************************************************************************************************
    // Q: 20160714
    // Ustawienie komendy wraz z parametrami
    // *************************************************************************************************
    bool TMoverParameters::SetInternalCommand(std::string NewCommand, double NewValue1, double NewValue2,
                                              int const Couplertype) {
        bool SIC;
        if ((CommandIn.Command == NewCommand) && (CommandIn.Value1 == NewValue1) && (CommandIn.Value2 == NewValue2) &&
            (CommandIn.Coupling == Couplertype))
            SIC = false;
            else {
                CommandIn.Command = NewCommand;
                CommandIn.Value1 = NewValue1;
                CommandIn.Value2 = NewValue2;
                CommandIn.Coupling = Couplertype;
                SIC = true;
                LastLoadChangeTime = 0; // zerowanie czasu (roz)ładowania
            }

        return SIC;
    }

    // *************************************************************************************************
    // Q: 20160714
    // wysyłanie komendy w kierunku dir (1=przód, -1=tył) do kolejnego pojazdu (jednego)
    // *************************************************************************************************
    bool TMoverParameters::SendCtrlToNext(std::string const CtrlCommand, double const ctrlvalue, double const dir,
                                          int const Couplertype) {
        bool OK;
        int d; // numer sprzęgu w kierunku którego wysyłamy

        // Ra: był problem z propagacją, jeśli w składzie jest pojazd wstawiony odwrotnie
        // Ra: problem jest również, jeśli AI będzie na końcu składu
        OK = true;                                 // ( dir != 0 ); // experimentally disabled
        d = (1 + static_cast<int>(Sign(dir))) / 2; // dir=-1=>d=0, dir=1=>d=1 - wysyłanie tylko w tył
            if (OK) {
                    // musi być wybrana niezerowa kabina
                    if ((Couplers[d].Connected != nullptr) && (TestFlag(Couplers[d].CouplingFlag, Couplertype))) {

                            if (Couplers[d].ConnectedNr != d) {
                                // jeśli ten nastpęny jest zgodny z aktualnym
                                if (Couplers[d].Connected->SetInternalCommand(CtrlCommand, ctrlvalue, dir, Couplertype))
                                    OK = (Couplers[d].Connected->RunInternalCommand() && OK); // tu jest rekurencja
                            } else {
                                // jeśli następny jest ustawiony przeciwnie, zmieniamy kierunek
                                if (Couplers[d].Connected->SetInternalCommand(CtrlCommand, ctrlvalue, -dir,
                                                                              Couplertype))
                                    OK = (Couplers[d].Connected->RunInternalCommand() && OK); // tu jest rekurencja
                            }
                }
        }
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160723
    // *************************************************************************************************
    // wysłanie komendy otrzymanej z kierunku CValue2 (względem sprzęgów: 1=przod,-1=tył)
    // Ra: Jest tu problem z rekurencją. Trzeba by oddzielić wykonywanie komend od mechanizmu
    // ich propagacji w składzie. Osobnym problemem może być propagacja tylko w jedną stronę.
    // Jeśli jakiś człon jest wstawiony odwrotnie, to również odwrotnie musi wykonywać
    // komendy związane z kierunkami (PantFront, PantRear, DoorLeft, DoorRight).
    // Komenda musi być zdefiniowana tutaj, a jeśli się wywołuje funkcję, to ona nie może
    // sama przesyłać do kolejnych pojazdów. Należy też się zastanowić, czy dla uzyskania
    // jakiejś zmiany (np. IncMainCtrl) lepiej wywołać funkcję, czy od razu wysłać komendę.
    bool TMoverParameters::RunCommand(std::string Command, double CValue1, double CValue2, int const Couplertype) {
        bool OK{false};

            if (Command == "MainCtrl") {
                    if (MainCtrlPosNo >= floor(CValue1)) {
                        MainCtrlPos = static_cast<int>(floor(CValue1));
                            if (DelayCtrlFlag) {
                                if ((LastRelayTime >= InitialCtrlDelay) && (MainCtrlPos == 1))
                                    LastRelayTime = 0;
                        } else if (LastRelayTime > CtrlDelay)
                            LastRelayTime = 0;
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "ScndCtrl") {
                    if (ScndCtrlPosNo >= floor(CValue1)) {
                        ScndCtrlPos = static_cast<int>(floor(CValue1));
                        if (LastRelayTime > CtrlDelay)
                            LastRelayTime = 0;
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            }
            /*  else if command='BrakeCtrl' then
                begin
                if BrakeCtrlPosNo>=Trunc(CValue1) then
                begin
        BrakeCtrlPos:=Trunc(CValue1);
        OK:=SendCtrlToNext(command,CValue1,CValue2);
        end;
        end */
            else if (Command ==
                     "EIMIC") // ElectricInductionMotor Integrated Control - propulsion and brakes in relative values
            {
                eimic_real = CValue1;
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "Brake") // youBy - jak sie EP hamuje, to trza sygnal wyslac...
            {
                if (CValue1 < 0.001)
                    DynamicBrakeEMUStatus = true;
                double temp1 = CValue1;
                if ((DCEMUED_EP_max_Vel > 0.001) && (Vel > DCEMUED_EP_max_Vel) && (DynamicBrakeEMUStatus))
                    temp1 = 0;
                if ((DCEMUED_EP_min_Im > 0.001) && (fabs(Im) > DCEMUED_EP_min_Im) && (DynamicBrakeEMUStatus))
                    temp1 = 0;
                Hamulec->SetEPS(temp1);
                TUHEX_StageActual = CValue1;
                TUHEX_Active = TUHEX_StageActual > 0;
                if (CValue1 < 0.001)
                    DynamicBrakeEMUStatus = true;
                // fBrakeCtrlPos:=BrakeCtrlPos; //to powinnno być w jednym miejscu, aktualnie w C++!!!
                BrakePressureActual = BrakePressureTable[BrakeCtrlPos];
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } // youby - odluzniacz hamulcow, przyda sie
            else if (Command == "BrakeReleaser") {
                OK = BrakeReleaser(
                        Round(CValue1)); // samo się przesyła dalej
                                         // OK:=SendCtrlToNext(command,CValue1,CValue2); //to robiło kaskadę 2^n
            } else if (Command == "WaterPumpBreakerSwitch") {
                /*
                   if( FuelPump.start_type != start::automatic ) {
                // automatic fuel pump ignores 'manual' state commands
                */
                WaterPump.breaker = (CValue1 == 1);
                /*
                   }
                   */
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "WaterPumpSwitch") {

                    if (WaterPump.start_type != start_t::battery) {
                        // automatic fuel pump ignores 'manual' state commands
                        WaterPump.is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "WaterPumpSwitchOff") {

                    if (WaterPump.start_type != start_t::battery) {
                        // automatic fuel pump ignores 'manual' state commands
                        WaterPump.is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "WaterHeaterBreakerSwitch") {
                /*
                   if( FuelPump.start_type != start::automatic ) {
                // automatic fuel pump ignores 'manual' state commands
                */
                WaterHeater.breaker = (CValue1 == 1);
                /*
                   }
                   */
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "WaterHeaterSwitch") {
                /*
                   if( FuelPump.start_type != start::automatic ) {
                // automatic fuel pump ignores 'manual' state commands
                */
                WaterHeater.is_enabled = (CValue1 == 1);
                /*
                   }
                   */
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "WaterCircuitsLinkSwitch") {
                    if (true == dizel_heat.auxiliary_water_circuit) {
                        // can only link circuits if the vehicle has more than one of them
                        WaterCircuitsLink = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "FuelPumpSwitch") {
                    if (FuelPump.start_type != start_t::automatic) {
                        // automatic fuel pump ignores 'manual' state commands
                        FuelPump.is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "FuelPumpSwitchOff") {
                    if (FuelPump.start_type != start_t::automatic) {
                        // automatic fuel pump ignores 'manual' state commands
                        FuelPump.is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "OilPumpSwitch") {
                    if (OilPump.start_type != start_t::automatic) {
                        // automatic pump ignores 'manual' state commands
                        OilPump.is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "OilPumpSwitchOff") {
                    if (OilPump.start_type != start_t::automatic) {
                        // automatic pump ignores 'manual' state commands
                        OilPump.is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MotorBlowersFrontSwitch") {
                    if ((MotorBlowers[end::front].start_type != start_t::manual) &&
                        (MotorBlowers[end::front].start_type != start_t::manualwithautofallback)) {
                        // automatic device ignores 'manual' state commands
                        MotorBlowers[end::front].is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MotorBlowersFrontSwitchOff") {
                    if ((MotorBlowers[end::front].start_type != start_t::manual) &&
                        (MotorBlowers[end::front].start_type != start_t::manualwithautofallback)) {
                        // automatic device ignores 'manual' state commands
                        MotorBlowers[end::front].is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MotorBlowersRearSwitch") {
                    if ((MotorBlowers[end::rear].start_type != start_t::manual) &&
                        (MotorBlowers[end::rear].start_type != start_t::manualwithautofallback)) {
                        // automatic device ignores 'manual' state commands
                        MotorBlowers[end::rear].is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MotorBlowersRearSwitchOff") {
                    if ((MotorBlowers[end::rear].start_type != start_t::manual) &&
                        (MotorBlowers[end::rear].start_type != start_t::manualwithautofallback)) {
                        // automatic device ignores 'manual' state commands
                        MotorBlowers[end::rear].is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "CompartmentLightsSwitch") {

                    if (CompartmentLights.start_type != start_t::automatic) {
                        // automatic lights ignore 'manual' state commands
                        CompartmentLights.is_enabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "CompartmentLightsSwitchOff") {

                    if (CompartmentLights.start_type != start_t::automatic) {
                        // automatic lights ignore 'manual' state commands
                        CompartmentLights.is_disabled = (CValue1 == 1);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MainSwitch") {
                MainSwitch_(CValue1 > 0.0);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "HeatingSwitch") {
                HeatingSwitch_(CValue1 > 0.0);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "Direction") {
                DirActive = static_cast<int>(floor(CValue1));
                DirAbsolute = DirActive * CabActive;
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "CabActivisation") {
                    //  OK:=Power>0.01;
                    switch (static_cast<int>(CValue1 *
                                             CValue2)) { // CValue2 ma zmieniany znak przy niezgodności sprzęgów
                            case 1: {
                                CabActive = 1;
                                break;
                            }
                            case -1: {
                                CabActive = -1;
                                break;
                            }
                            default: {
                                CabActive = 0; // gdy CValue1==0
                                break;
                            }
                    }
                DirAbsolute = DirActive * CabActive;
                CabMaster = false;
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "AutoRelaySwitch") {
                if ((CValue1 == 1) && (AutoRelayType == 2))
                    AutoRelayFlag = true;
                else
                    AutoRelayFlag = false;
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "RelayReset") {
                RelayReset(CValue1, range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "ConverterSwitch") /*NBMX*/
            {
                    if (ConverterStart == start_t::manual) {
                        ConverterAllow = (CValue1 > 0.0);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "BatterySwitch") /*NBMX*/
            {
                    if (BatteryStart == start_t::manual) {
                        Battery = (CValue1 > 0.0);
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            }
            //   else if command='EpFuseSwitch' then         {NBMX}
            //   begin
            //     if (CValue1=1) then EpFuse:=true
            //     else if (CValue1=0) then EpFuse:=false;
            //     OK:=SendCtrlToNext(command,CValue1,CValue2);
            //   end
            else if (Command == "CompressorSwitch") /*NBMX*/
            {
                CompressorSwitch((CValue1 == 1), range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "CompressorPreset") {
                CompressorListPos = clamp(static_cast<int>(CValue1), 0, CompressorListPosNo);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorPermit") {

                auto const left{CValue2 > 0 ? 1 : 2};
                auto const right{3 - left};

                    if (std::abs(static_cast<int>(CValue1)) & right) {
                        PermitDoors_(side::right, (CValue1 > 0));
                }
                    if (std::abs(static_cast<int>(CValue1)) & left) {
                        PermitDoors_(side::left, (CValue1 > 0));
                }

                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorOpen") /*NBMX*/
            {                                 // Ra: uwzględnić trzeba jeszcze zgodność sprzęgów
                    if ((Doors.open_control == control_t::conductor) || (Doors.open_control == control_t::driver) ||
                        (Doors.open_control == control_t::mixed)) {
                            // ignore remote command if the door is only operated locally
                            if (Power24vIsAvailable || Power110vIsAvailable) {

                                auto const left{CValue2 > 0 ? 1 : 2};
                                auto const right{3 - left};

                                    if (static_cast<int>(CValue1) & right) {
                                        Doors.instances[side::right].remote_open = true;
                                        Doors.instances[side::right].remote_close = false;
                                }
                                    if (static_cast<int>(CValue1) & left) {
                                        Doors.instances[side::left].remote_open = true;
                                        Doors.instances[side::left].remote_close = false;
                                }
                        }
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorClose") /*NBMX*/
            {                                  // Ra: uwzględnić trzeba jeszcze zgodność sprzęgów
                    if ((Doors.close_control == control_t::conductor) || (Doors.close_control == control_t::driver) ||
                        (Doors.close_control == control_t::mixed)) {
                            // ignore remote command if the door is only operated locally
                            if (Power24vIsAvailable || Power110vIsAvailable) {

                                auto const left{CValue2 > 0 ? 1 : 2};
                                auto const right{3 - left};

                                    if (static_cast<int>(CValue1) & right) {
                                        Doors.instances[side::right].remote_close = true;
                                        Doors.instances[side::right].remote_open = false;
                                }
                                    if (static_cast<int>(CValue1) & left) {
                                        Doors.instances[side::left].remote_close = true;
                                        Doors.instances[side::left].remote_open = false;
                                }
                        }
                }
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorLock") {
                Doors.lock_enabled = (CValue1 == 1 ? true : false);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorStep") {
                Doors.step_enabled = (CValue1 == 1 ? true : false);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DoorMode") {
                Doors.remote_only = (CValue1 == 1 ? true : false);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "DepartureSignal") {
                DepartureSignal = (CValue1 == 1 ? true : false);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "PantValve") // Winger 160204
            {                                  // Ra: uwzględnić trzeba jeszcze zgodność sprzęgów
                auto const inputend{(static_cast<int>(CValue1) & 0x80) != 0 ? 1 : 0};
                auto const inputcab{(static_cast<int>(CValue1) & 0x40) != 0 ? 1 : 0};
                auto const inputoperation{static_cast<int>(CValue1) & ~(0x80 | 0x40)};
                auto const noswap{(TrainType == dt_EZT) || (TrainType == dt_ET41)};
                auto swap{
                        (false == noswap) &&
                        (TestFlag(Couplers[(CValue2 == -1 ? end::rear : end::front)].CouplingFlag, coupling::control))};
                auto const reversed{inputcab != (CabActive != -1 ? 1 : 0)};
                    if (reversed) {
                        swap = !swap;
                } // TODO: check whether this part has RL equivalent
                OperatePantographValve(static_cast<end>(swap ? 1 - inputend : inputend),
                                       static_cast<operation_t>(inputoperation), range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "PantsValve") {
                OperatePantographsValve(static_cast<operation_t>(static_cast<int>(CValue1)), range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "PantAllDown") {
                DropAllPantographs(CValue1 == 1, range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MaxCurrentSwitch") {
                MaxCurrentSwitch(CValue1 == 1, range_t::local);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "MinCurrentSwitch") {
                OK = MinCurrentSwitch(CValue1 == 1);
            }
            /*test komend oddzialywujacych na tabor*/
            else if (Command == "SetDamage") {
                if (CValue2 == 1)
                    OK = SetFlag(DamageFlag, static_cast<int>(floor(CValue1)));
                if (CValue2 == -1)
                    OK = SetFlag(DamageFlag, static_cast<int>(-floor(CValue1)));
            } else if (Command == "Emergency_brake") {
                if (RadiostopSwitch(floor(CValue1) == 1)) // YB: czy to jest potrzebne?
                    OK = true;
                else
                    OK = false;
            } else if (Command == "BrakeDelay") {
                auto const brakesetting = static_cast<int>(std::floor(CValue1));
                    if (true == Hamulec->SetBDF(brakesetting)) {
                        BrakeDelayFlag = brakesetting;
                        OK = true;
                    } else {
                        OK = false;
                    }
                SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "Sandbox") {
                OK = Sandbox(CValue1 == 1, range_t::local);
            } else if (Command == "CabSignal") /*SHP,Indusi*/
            {                                  // Ra: to powinno działać tylko w członie obsadzonym
                SecuritySystem.set_cabsignal();
                // else OK:=false;
                OK = true; // true, gdy można usunąć komendę
            }
            /*naladunek/rozladunek*/
            // TODO: have these commands leverage load exchange system instead
            // TODO: CValue1 defines amount to load/unload
            else if (issection("Load=", Command)) {
                OK = false;         // będzie powtarzane aż się załaduje
                    if ((Vel < 0.1) // tolerance margin for small vehicle movements in the consist
                        && (MaxLoad > 0) && (LoadAmount < MaxLoad * (1.0 + OverLoadFactor)) &&
                        (Distance(Loc, CommandIn.Location, Dim, Dim) <
                         (CValue2 > 1.0 ? CValue2 : 10.0))) { // ten peron/rampa

                        auto const loadname{ToLower(extract_value("Load", Command))};
                            if (LoadAmount == 0.f) {
                                AssignLoad(loadname);
                        }
                        OK = LoadingDone(LoadSpeed,
                                         loadname); // zmienia LoadStatus
                    } else {
                        // no loading can be done if conditions aren't met
                        LastLoadChangeTime = 0.0;
                    }
            } else if (issection("UnLoad=", Command)) {
                OK = false;                 // będzie powtarzane aż się rozładuje
                    if ((Vel < 0.1)         // tolerance margin for small vehicle movements in the consist
                        && (LoadAmount > 0) // czy jest co rozladowac?
                        &&
                        (Distance(Loc, CommandIn.Location, Dim, Dim) < (CValue2 > 1.0 ? CValue2 : 10.0))) { // ten peron
                        /*mozna to rozladowac*/
                        OK = LoadingDone(-1.f * LoadSpeed, ToLower(extract_value("UnLoad", Command)));
                    } else {
                        // no loading can be done if conditions aren't met
                        LastLoadChangeTime = 0.0;
                    }
            } else if (Command == "SpeedCntrl") {
                if ((EngineType == TEngineType::ElectricInductionMotor) || (SpeedCtrl))
                    SpeedCtrlValue = static_cast<int>(round(CValue1));
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "SpeedCtrlUnit.Parking") {
                SpeedCtrlUnit.Parking = static_cast<bool>(CValue1);
                OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
            } else if (Command == "SpringBrakeActivate") {
                    if (Power24vIsAvailable || Power110vIsAvailable) {
                        SpringBrake.Activate = CValue1 > 0;
                        OK = SendCtrlToNext(Command, CValue1, CValue2, Couplertype);
                } else
                    OK = true;
        }

        return OK; // dla true komenda będzie usunięta, dla false wykonana ponownie
    }

    // *************************************************************************************************
    // Q: 20160714
    // Uruchamia funkcję RunCommand aż do skutku. Jeśli będzie pozytywny to kasuje komendę.
    // *************************************************************************************************
    bool TMoverParameters::RunInternalCommand() {
        bool OK;

            if (!CommandIn.Command.empty()) {
                OK = RunCommand(CommandIn.Command, CommandIn.Value1, CommandIn.Value2, CommandIn.Coupling);
                    if (OK) {

                        CommandIn.Command.clear(); // kasowanie bo rozkaz wykonany
                        CommandIn.Value1 = 0;
                        CommandIn.Value2 = 0;
                        CommandIn.Coupling = 0;
                        CommandIn.Location.X = 0;
                        CommandIn.Location.Y = 0;
                        CommandIn.Location.Z = 0;
                        switch_physics(true);
                }
        } else
            OK = false;
        return OK;
    }

    // *************************************************************************************************
    // Q: 20160714
    // Zwraca wartość natężenia prądu na wybranym amperomierzu. Podfunkcja do ShowCurrent.
    // *************************************************************************************************
    double TMoverParameters::ShowCurrentP(int AmpN) const {
        int b, Bn;
        bool Grupowy;

        // ClearPendingExceptions;
        Grupowy = ((DelayCtrlFlag) && (TrainType == dt_ET22)); // przerzucanie walu grupowego w ET22;
        Bn = RList[MainCtrlActualPos].Bn;                      // ile równoległych gałęzi silników

        if ((DynamicBrakeType == dbrake_automatic) && (DynamicBrakeFlag))
            Bn = DynamicBrakeAmpmeters;
            if (Power > 0.01) {
                    if (AmpN > 0) // podać prąd w gałęzi
                    {
                        if ((Bn < AmpN) || ((Grupowy) && (AmpN == Bn - 1)))
                            return 0;
                        else // normalne podawanie pradu
                            return floor(fabs(Im));
                } else // podać całkowity
                    return floor(fabs(Itot));
            } else // pobor pradu jezeli niema mocy
            {
                int current = 0;
                for (b = 0; b < 2; b++)
                    // with Couplers[b] do
                    if (TestFlag(Couplers[b].CouplingFlag, coupling::control))
                        if (Couplers[b].Connected->Power > 0.01)
                            current = static_cast<int>(Couplers[b].Connected->ShowCurrent(AmpN));
                return current;
            }
    }
    namespace simulation {

        weights_table Weights;

    } // namespace simulation
} // namespace Maszyna
