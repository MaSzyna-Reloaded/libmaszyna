/*fizyka hamulcow dla symulatora*/

/*
   This Source Code Form is subject to the
   terms of the Mozilla Public License, v.
   2.0. If a copy of the MPL was not
   distributed with this file, You can
   obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
   MaSzyna EU07 - SPKS
   Brakes.
   Copyright (C) 2007-2014 Maciej Cierniak
   */


/*
   (C) youBy
   Co brakuje:
   moze jeszcze jakis SW
   */
/*
Zrobione:
ESt3, ESt3AL2, ESt4R, LSt, FV4a, FD1, EP2, prosty westinghouse
duzo wersji żeliwa
KE
Tarcze od 152A
Magnetyki (implementacja w mover.pas)
Matrosow 394
H14K1 (zasadniczy), H1405 (pomocniczy), St113 (ep)
Knorr/West EP - żeby był
*/

#pragma once

#include <cstring>
#include <memory>
#include "friction.h" // Pascal unit
#include "../../../utils/utils.hpp"
using namespace Maszyna;

namespace Maszyna {

	static constexpr int LocalBrakePosNo = 10; /*ilosc nastaw hamulca recznego lub pomocniczego*/
	static constexpr int MainBrakeMaxPos = 10; /*max. ilosc nastaw hamulca zasadniczego*/

	/*nastawy hamulca*/
	static constexpr int bdelay_G = 1; // G
	static constexpr int bdelay_P = 2; // P
	static constexpr int bdelay_R = 4; // R
	static constexpr int bdelay_M = 8; // Mg


	/*stan hamulca*/
	static constexpr int b_off = 0; // luzowanie
	static constexpr int b_hld = 1; // trzymanie
	static constexpr int b_on = 2; // napelnianie
	static constexpr int b_rfl = 4; // uzupelnianie
	static constexpr int b_rls = 8; // odluzniacz
	static constexpr int b_ep = 16; // elektropneumatyczny
	static constexpr int b_asb = 32; // przeciwposlizg-wstrzymanie
	static constexpr int b_asb_unbrake = 64; // przeciwposlizg-luzowanie
	static constexpr int b_dmg = 128; // wylaczony z dzialania

	/*uszkodzenia hamulca*/
	static constexpr int df_on = 1; // napelnianie
	static constexpr int df_off = 2; // luzowanie
	static constexpr int df_br = 4; // wyplyw z ZP
	static constexpr int df_vv = 8; // wyplyw z komory wstepnej
	static constexpr int df_bc = 16; // wyplyw z silownika
	static constexpr int df_cv = 32; // wyplyw z ZS
	static constexpr int df_PP = 64; // zawsze niski stopien
	static constexpr int df_RR = 128; // zawsze wysoki stopien

	/*indeksy dzwiekow FV4a*/
	static constexpr int s_fv4a_b = 0; // hamowanie
	static constexpr int s_fv4a_u = 1; // luzowanie
	static constexpr int s_fv4a_e = 2; // hamowanie nagle
	static constexpr int s_fv4a_x = 3; // wyplyw sterujacego fala
	static constexpr int s_fv4a_t = 4; // wyplyw z czasowego

	/*pary cierne*/
	static constexpr int bp_P10 = 0;
	static constexpr int bp_P10Bg = 2; // żeliwo fosforowe P10
	static constexpr int bp_P10Bgu = 1;
	static constexpr int bp_LLBg = 4; // komp. b.n.t.
	static constexpr int bp_LLBgu = 3;
	static constexpr int bp_LBg = 6; // komp. n.t.
	static constexpr int bp_LBgu = 5;
	static constexpr int bp_KBg = 8; // komp. w.t.
	static constexpr int bp_KBgu = 7;
	static constexpr int bp_D1 = 9; // tarcze
	static constexpr int bp_D2 = 10;
	static constexpr int bp_FR513 = 11; // Frenoplast FR513
	static constexpr int bp_Cosid = 12; // jakistam kompozyt :D
	static constexpr int bp_PKPBg = 13; // żeliwo PKP
	static constexpr int bp_PKPBgu = 14;
	static constexpr int bp_MHS = 128; // magnetyczny hamulec szynowy
	static constexpr int bp_P10yBg = 15; // żeliwo fosforowe P10
	static constexpr int bp_P10yBgu = 16;
	static constexpr int bp_FR510 = 17; // Frenoplast FR510

	static constexpr int sf_Acc = 1; // przyspieszacz
	static constexpr int sf_BR = 2; // przekladnia
	static constexpr int sf_CylB = 4; // cylinder - napelnianie
	static constexpr int sf_CylU = 8; // cylinder - oproznianie
	static constexpr int sf_rel = 16; // odluzniacz
	static constexpr int sf_ep = 32; // zawory ep

	static constexpr int bh_MIN = 0; // minimalna pozycja
	static constexpr int bh_MAX = 1; // maksymalna pozycja
	static constexpr int bh_FS = 2; // napelnianie uderzeniowe //jesli nie ma, to jazda
	static constexpr int bh_RP = 3; // jazda
	static constexpr int bh_NP = 4; // odciecie - podwojna trakcja
	static constexpr int bh_MB = 5; // odciecie - utrzymanie stopnia hamowania/pierwszy 1 stopien hamowania
	static constexpr int bh_FB = 6; // pelne
	static constexpr int bh_EB = 7; // nagle
	static constexpr int bh_EPR = 8; // ep - luzowanie  //pelny luz dla ep kątowego
	static constexpr int bh_EPN = 9; // ep - utrzymanie //jesli rowne luzowaniu, wtedy sterowanie przyciskiem
	static constexpr int bh_EPB = 10; // ep - hamowanie  //pelne hamowanie dla ep kątowego


	static constexpr double SpgD = 0.7917;
	static constexpr double SpO =
			0.5067; // przekroj przewodu 1" w l/m
					// wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
					// i pojemnosci sa podane w litrach (rozsadne wielkosci)
					// zas dlugosc pojazdow jest podana w metrach
					// a predkosc przeplywu w m/s                           //3.5
					// 7//1.5
					//    BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (14, 5.4), (9, 5.0), (6, 4.6), (9, 4.5),
					//    (9, 4.0), (9, 3.5), (9, 2.8), (34, 2.8)); BPT: array[-2..6] of array [0..1] of real=
					//    ((0, 5.0), (7, 5.0), (2.0, 5.0), (4.5, 4.6), (4.5, 4.2), (4.5, 3.8), (4.5, 3.4), (4.5, 2.8),
					//    (8, 2.8));
	static double const BPT[9][2] = {{0, 5.0},	 {7, 5.0},	 {2.0, 5.0}, {4.5, 4.6}, {4.5, 4.2},
									 {4.5, 3.8}, {4.5, 3.4}, {4.5, 2.8}, {8, 2.8}};
	static double const BPT_394[7][2] = {{13, 10.0}, {5, 5.0}, {0, -1}, {5, -1}, {5, 0.0}, {5, 0.0}, {18, 0.0}};
	// double *BPT = zero_based_BPT[2]; //tablica pozycji hamulca dla zakresu -2..6
	// double *BPT_394 = zero_based_BPT_394[1]; //tablica pozycji hamulca dla zakresu -1..5
	//    BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (12, 5.4), (9, 5.0), (9, 4.6), (9, 4.2), (9, 3.8),
	//    (9, 3.4), (9, 2.8), (34, 2.8));
	//       BPT: array[-2..6] of array [0..1] of real= ((0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0,
	//       0));
	//  static double const pi = 3.141592653589793; //definicja w mctools

	enum TUniversalBrake // możliwe działania uniwersalnego przycisku hamulca
	{
		// kolejne flagi
		ub_Release = 0x01, // odluźniacz - ZR
		ub_UnlockPipe = 0x02, // odblok PG / mostkowanie hamulca bezpieczeństwa - POJAZD
		ub_HighPressure = 0x04, // impuls wysokiego ciśnienia - ZM
		ub_Overload = 0x08, // przycisk asymilacji / kontrolowanego przeładowania - ZM
		ub_AntiSlipBrake = 0x10, // przycisk przyhamowania przeciwposlizgowego - ZR
		ub_Ostatni = 0x80000000 // ostatnia flaga bitowa
	};

	// klasa obejmujaca pojedyncze zbiorniki
	class TReservoir {
		protected:
			double Cap{1.0};
			double Vol{0.0};
			double dVol{0.0};

		public:
			virtual ~TReservoir() = default;
			void CreateCap(double Capacity);
			void CreatePress(double Press);
			virtual double pa();
			virtual double P();
			void Flow(double dv);
			void Act();
			TReservoir() = default;
	};

	typedef TReservoir *PReservoir;

	class TBrakeCyl final : public TReservoir {
		public:
			double pa() override;
			double P() override;
			TBrakeCyl() : TReservoir(){};
	};

	// klasa obejmujaca uklad hamulca zespolonego pojazdu
	class TBrake {
		protected:
			std::shared_ptr<TReservoir> BrakeCyl{}; // silownik
			std::shared_ptr<TReservoir> BrakeRes{}; // ZP
			std::shared_ptr<TReservoir> ValveRes{}; // komora wstepna
			int BCN = 0; // ilosc silownikow
			double BCM = 0.0; // przekladnia hamulcowa
			double BCA = 0.0; // laczny przekroj silownikow
			int BrakeDelays = 0; // dostepne opoznienia
			int BrakeDelayFlag = 0; // aktualna nastawa
			std::shared_ptr<TFricMat> FM{}; // material cierny
			double MaxBP = 0.0; // najwyzsze cisnienie
			int BA = 0; // osie hamowane
			int NBpA = 0; // klocki na os
			double SizeBR = 0.0; // rozmiar^2 ZP (w stosunku do 14")
			double SizeBC = 0.0; // rozmiar^2 CH (w stosunku do 14")
			bool DCV = false; // podwojny zawor zwrotny
			double ASBP = 0.0; // cisnienie hamulca pp
			double RV = 0.0; // rapid activation vehicle velocity threshold

			int UniversalFlag = 0; // flaga wcisnietych przyciskow uniwersalnych
			int BrakeStatus{b_off}; // flaga stanu
			int SoundFlag = 0;

		public:
			virtual ~TBrake() = default;
			TBrake(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba,
				   int i_nbpa);

			// maksymalne cisnienie, promien, skok roboczy, pojemnosc ZP, ilosc cylindrow, opoznienia hamulca, material
			// klockow, osie hamowane, klocki na os;
			virtual void Init(double PP, double HPP, double LPP, double BP, int BDF);

			// inicjalizacja hamulca

			double GetFC(double Vel, double N) const; // wspolczynnik tarcia - hamulec wie lepiej
			virtual double GetPF(double PP, double dt, double Vel); // przeplyw miedzy komora wstepna i PG
			double GetBCF() const; // sila tlokowa z tloka
			virtual double GetHPFlow(double HP, double dt); // przeplyw - 8 bar
			double GetBCP() const; // cisnienie cylindrow hamulcowych
			virtual double GetEDBCP(); // cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
			double GetBRP() const; // cisnienie zbiornika pomocniczego
			double GetVRP() const; // cisnienie komory wstepnej rozdzielacza
			virtual double GetCRP(); // cisnienie zbiornika sterujacego
			bool SetBDF(int nBDF); // nastawiacz GPRM
			void Releaser(int state); // odluzniacz
			bool Releaser() const;
			virtual void SetEPS(double nEPS); // hamulec EP
			virtual void SetRM(double const RMR){}; // ustalenie przelozenia rapida
			virtual void SetRV(double const RVR) { RV = RVR; }; // ustalenie przelozenia rapida
			virtual void SetLP(double const TM, double const LM, double const TBP){}; // parametry przystawki wazacej
			virtual void SetLBP(double const P){}; // cisnienie z hamulca pomocniczego
			virtual void PLC(double const mass){}; // wspolczynnik cisnienia przystawki wazacej
			void ASB(int state); // hamulec przeciwposlizgowy
			int GetStatus() const; // flaga statusu, moze sie przydac do odglosow
			void SetASBP(double Press); // ustalenie cisnienia pp
			virtual void ForceEmptiness();

			// removes specified amount of air from the reservoirs
			virtual void ForceLeak(double Amount);
			int GetSoundFlag();
			int GetBrakeStatus() const { return BrakeStatus; }
			void SetBrakeStatus(int const Status) { BrakeStatus = Status; }
			virtual void SetED(double const EDstate){}; // stan hamulca ED do luzowania
			virtual void SetUniversalFlag(const int flag) { UniversalFlag = flag; } // przycisk uniwersalny
	};

	class TWest final : public TBrake {
			double LBP = 0.0; // cisnienie hamulca pomocniczego
			double dVP = 0.0; // pobor powietrza wysokiego cisnienia
			double EPS = 0.0; // stan elektropneumatyka
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // i pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double LoadC = 0.0; // wspolczynnik przystawki wazacej

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			void SetLBP(double P) override; // cisnienie z hamulca pomocniczego
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			double GetHPFlow(double HP, double dt) override;
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetEPS(double nEPS) override; // stan hamulca EP
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			TWest(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				  const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
	};

	class TESt : public TBrake {
		protected:
			std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujący
			double BVM = 0.0; // przelozenie PG-CH

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			static void EStParams(double i_crc); // parametry charakterystyczne dla ESt
			double GetCRP() override;
			void CheckState(double BCP, double &dV1); // glowny przyrzad rozrzadczy
			void CheckReleaser(double dt); // odluzniacz
			double CVs(double BP) const; // napelniacz sterujacego
			double BVs(double BCP) const; // napelniacz pomocniczego
			void ForceEmptiness() override; // wymuszenie bycia pustym
			TESt(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				 const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				CntrlRes = std::make_shared<TReservoir>();
			}
	};

	class TESt3 : public TESt {
			// double CylFlowSpeed[2][2]; //zmienna nie uzywana

		public:
			double GetPF(double PP, double dt, double Vel) override;
			TESt3(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				  const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
	};

	class TESt3AL2 final : public TESt3 {
			std::shared_ptr<TReservoir> ImplsRes; // komora impulsowa
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // i pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double LoadC = 0.0;

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			TESt3AL2(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
					 const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TESt3(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				ImplsRes = std::make_shared<TReservoir>();
			}
	};

	class TESt4R : public TESt {
			bool RapidStatus = false;

		protected:
			std::shared_ptr<TReservoir> ImplsRes; // komora impulsowa
			double RapidTemp = 0.0; // aktualne, zmienne przelozenie

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			TESt4R(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				   const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TESt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				ImplsRes = std::make_shared<TReservoir>();
			}
	};

	class TLSt : public TESt4R {
		protected:
			double LBP = 0.0; // cisnienie hamulca pomocniczego
			double RM = 0.0; // przelozenie rapida
			double EDFlag = 0.0; // luzowanie hamulca z powodu zalaczonego ED

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			void SetLBP(double P) override; // cisnienie z hamulca pomocniczego
			void SetRM(double RMR) override; // ustalenie przelozenia rapida
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			double GetHPFlow(double HP, double dt) override; // przeplyw - 8 barów
			double GetEDBCP() override; // cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
			void SetED(double EDstate) override; // stan hamulca ED do luzowania
			TLSt(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				 const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TESt4R(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
	};

	class TEStED final : public TLSt {
			// zawor z EP09 - Est4 z oddzielnym przekladnikiem, kontrola rapidu i takie tam

			std::shared_ptr<TReservoir> Miedzypoj; // pojemnosc posrednia (urojona) do napelniania ZP i ZS
			double Nozzles[11]; // dysze
			bool Zamykajacy = false; // pamiec zaworka zamykajacego
			bool Przys_blok = false; // blokada przyspieszacza
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // i pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double LoadC = 0.0;

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			double GetEDBCP() override; // cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			TEStED(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba,
				   int i_nbpa) : TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa), Nozzles{} {
				Miedzypoj = std::make_shared<TReservoir>();
			}
	};

	class TEStEP2 : public TLSt {
		protected:
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // masa pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double LoadC = 0.0;
			double EPS = 0.0;

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;

			// inicjalizacja
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetEPS(double nEPS) override; // stan hamulca EP
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			void virtual EPCalc(double dt);
			TEStEP2(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
					const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TLSt(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
	};

	class TEStEP1 final : public TEStEP2 {
		public:
			void EPCalc(double dt) override;
			void SetEPS(double nEPS) override; // stan hamulca EP
			TEStEP1(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
					const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TEStEP2(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
	};

	class TCV1 : public TBrake {
			double BVM = 0.0; // przelozenie PG-CH

		protected:
			std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujący

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			double GetCRP() override;
			void CheckState(double BCP, double &dV1);
			static double CVs(double BP);
			double BVs(double BCP) const;
			void ForceEmptiness() override; // wymuszenie bycia pustym
			TCV1(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				 const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				CntrlRes = std::make_shared<TReservoir>();
			}
	};

	class TCV1L_TR final : public TCV1 {
			std::shared_ptr<TReservoir> ImplsRes; // komora impulsowa
			double LBP = 0.0; // cisnienie hamulca pomocniczego

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			void SetLBP(double P) override; // cisnienie z hamulca pomocniczego
			double GetHPFlow(double HP, double dt) override; // przeplyw - 8 barów
			TCV1L_TR(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
					 const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TCV1(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				ImplsRes = std::make_shared<TReservoir>();
			}
	};

	class TKE final : public TBrake {
			// Knorr Einheitsbauart — jeden do wszystkiego

			std::shared_ptr<TReservoir> ImplsRes; // komora impulsowa
			std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujący
			std::shared_ptr<TReservoir> Brak2Res; // zbiornik pomocniczy 2
			bool RapidStatus = false;
			double BVM = 0.0; // przelozenie PG-CH
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // masa pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double LoadC = 0.0; // wspolczynnik zaladowania
			double RM = 0.0; // przelozenie rapida
			double LBP = 0.0; // cisnienie hamulca pomocniczego

		public:
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			void SetRM(double RMR) override; // ustalenie przelozenia rapida
			double GetPF(double PP, double dt, double Vel) override; // przeplyw miedzy komora wstepna i PG
			double GetHPFlow(double HP, double dt) override; // przeplyw - 8 barów
			double GetCRP() override;
			void CheckState(double BCP, double &dV1);
			void CheckReleaser(double dt); // odluzniacz
			double CVs(double BP) const; // napelniacz sterujacego
			double BVs(double BCP) const; // napelniacz pomocniczego
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			void SetLBP(double P) override; // cisnienie z hamulca pomocniczego
			void ForceEmptiness() override; // wymuszenie bycia pustym
			TKE(const double i_mbp, const double i_bcr, const double i_bcd, const double i_brc, const int i_bcn,
				const int i_BD, const int i_mat, const int i_ba, const int i_nbpa) :
				TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {
				ImplsRes = std::make_shared<TReservoir>();
				CntrlRes = std::make_shared<TReservoir>();
				Brak2Res = std::make_shared<TReservoir>();
			}
	};

	// klasa obejmujaca krany
	class TDriverHandle {
		protected:
			//        BCP: integer;
			bool AutoOvrld = false; // czy jest asymilacja automatyczna na pozycji -1
			bool ManualOvrld = false; // czy jest asymilacja reczna przyciskiem
			bool ManualOvrldActive = false; // czy jest wcisniety przycisk asymilacji
			int UniversalFlag = 0; // flaga wcisnietych przyciskow uniwersalnych
			int i_bcpno = 6;

		public:
			virtual ~TDriverHandle() = default;
			bool Time = false;
			bool TimeEP = false;
			double Sounds[5]{}; // wielkosci przeplywow dla dzwiekow
			virtual double GetPF(double i_bcp, double PP, double HP, double dt, double ep);
			virtual void Init(double Press);
			virtual double GetCP();
			virtual double GetEP();
			virtual double GetRP();
			virtual void SetReductor(double nAdj); // korekcja pozycji reduktora cisnienia
			virtual double GetSound(int i); // pobranie glosnosci wybranego dzwieku
			virtual double GetPos(int i); // pobranie numeru pozycji o zadanym kodzie (funkcji)
			virtual double GetEP(double pos); // pobranie sily hamulca ep
			virtual void SetParams(bool AO, bool MO, double, double, double OMP,
								   double OPD){}; // ustawianie jakichs parametrow dla zaworu
			virtual void OvrldButton(bool Active); // przycisk recznego przeladowania/asymilacji
			virtual void SetUniversalFlag(int flag); // przycisk uniwersalny
			TDriverHandle() { memset(Sounds, 0, sizeof(Sounds)); }
	};

	class TFV4a final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący
			double TP = 0.0; // zbiornik czasowy
			double RP = 0.0; // zbiornik redukcyjny

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			TFV4a() = default;
	};

	class TFV4aM final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący
			double TP = 0.0; // zbiornik czasowy
			double RP = 0.0; // zbiornik redukcyjny
			double XP = 0.0; // komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)
								 //         Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
			bool Fala = false;
			static double const pos_table[11]; // = { -2, 6, -1, 0, -2, 1, 4, 6, 0, 0, 0 };
			static double LPP_RP(double pos);
			static bool EQ(double pos, double i_pos);

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetSound(int i) override;
			double GetPos(int i) override;
			double GetCP() override;
			double GetRP() override;
			TFV4aM() = default;
	};

	class TMHZ_EN57 final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący
			double TP = 0.0; // zbiornik czasowy
			double RP = 0.0; // zbiornik redukcyjny
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)
			bool Fala = false;
			double UnbrakeOverPressure = 0.0;
			double OverloadMaxPressure = 1.0; // maksymalne zwiekszenie cisnienia przy asymilacji
			double OverloadPressureDecrease = 0.045; // predkosc spadku cisnienia przy asymilacji
			static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };
			static double LPP_RP(double pos);
			static bool EQ(double pos, double i_pos);

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetSound(int i) override;
			double GetPos(int i) override;
			double GetCP() override;
			double GetRP() override;
			double GetEP(double pos) override;
			void SetParams(bool AO, bool MO, double OverP, double, double OMP, double OPD) override;
			TMHZ_EN57() = default;
	};

	class TMHZ_K5P final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący
			double TP = 0.0; // zbiornik czasowy
			double RP = 0.0; // zbiornik redukcyjny
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)
			bool Fala = false; // czy jest napelnianie uderzeniowe
			double UnbrakeOverPressure = 0.0;
			double OverloadMaxPressure = 1.0; // maksymalne zwiekszenie cisnienia przy asymilacji
			double OverloadPressureDecrease = 0.002; // predkosc spadku cisnienia przy asymilacji
			double FillingStrokeFactor = 1.0; // mnożnik otwarcia zaworu przy uderzeniowym (bez fali)
			static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };
			static bool EQ(double pos, double i_pos);

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetSound(int i) override;
			double GetPos(int i) override;
			double GetCP() override;
			double GetRP() override;
			void SetParams(bool AO, bool MO, double, double, double OMP, double OPD) override;
			TMHZ_K5P() = default;
	};

	class TMHZ_6P final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący
			double TP = 0.0; // zbiornik czasowy
			double RP = 0.0; // zbiornik redukcyjny
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)
			bool Fala = false; // czy jest napelnianie uderzeniowe
			double UnbrakeOverPressure = 0.0; // wartosc napelniania uderzeniowego
			double OverloadMaxPressure = 1.0; // maksymalne zwiekszenie cisnienia przy asymilacji
			double OverloadPressureDecrease = 0.002; // predkosc spadku cisnienia przy asymilacji
			double FillingStrokeFactor = 1.0; // mnożnik otwarcia zaworu przy uderzeniowym (bez fali)
			static double const pos_table[11]; //= { -2, 10, -1, 0, 0, 2, 9, 10, 0, 0, 0 };
			static bool EQ(double pos, double i_pos);

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetSound(int i) override;
			double GetPos(int i) override;
			double GetCP() override;
			double GetRP() override;
			void SetParams(bool AO, bool MO, double, double, double OMP, double OPD) override;
			TMHZ_6P() = default;
	};

	class TM394 final : public TDriverHandle {
			double CP = 0.0; // zbiornik sterujący, czasowy, redukcyjny
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)
			static double const pos_table[11]; // = { -1, 5, -1, 0, 1, 2, 4, 5, 0, 0, 0 };

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetCP() override;
			double GetRP() override;
			double GetPos(int i) override;
			TM394() { i_bcpno = 5; }
	};

	class TH14K1 : public TDriverHandle {
			static double const BPT_K[/*?*/ /*-1..4*/ (4) - (-1) + 1][2];
			static double const pos_table[11]; // = {-1, 4, -1, 0, 1, 2, 3, 4, 0, 0, 0};

		protected:
			double CP = 0.0; // zbiornik sterujący, czasowy, redukcyjny
			double RedAdj = 0.0; // dostosowanie reduktora cisnienia (krecenie kapturkiem)

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			void SetReductor(double nAdj) override;
			double GetCP() override;
			double GetRP() override;
			double GetPos(int i) override;
			TH14K1() { i_bcpno = 4; }
	};

	class TSt113 final : public TH14K1 {
			double EPS = 0.0;
			static double const BPT_K[/*?*/ /*-1..4*/ (4) - (-1) + 1][2];
			static double const BEP_K[/*?*/ /*-1..5*/ (5) - (-1) + 1];
			static double const pos_table[11]; // = {-1, 5, -1, 0, 2, 3, 4, 5, 0, 0, 1};
			double CP = 0;

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			double GetCP() override;
			double GetRP() override;
			double GetEP() override;
			double GetPos(int i) override;
			void Init(double Press) override;
			TSt113() = default;
	};

	class Ttest final : public TDriverHandle {
			double CP = 0.0;

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			Ttest() = default;
	};

	class TFD1 final : public TDriverHandle {
			double MaxBP = 0.0; // najwyzsze cisnienie
			double BP = 0.0; // aktualne cisnienie

		public:
			double Speed = 0.0; // szybkosc dzialania

			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			double GetCP() override;
			void SetSpeed(double nSpeed);
			TFD1() = default;
	};

	class TH1405 final : public TDriverHandle {
			double MaxBP = 0.0; // najwyzsze cisnienie
			double BP = 0.0; // aktualne cisnienie

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			void Init(double Press) override;
			double GetCP() override;
			TH1405() = default;
	};

	class TFVel6 final : public TDriverHandle {
			double EPS = 0.0;
			static double const pos_table[11]; // = {-1, 6, -1, 0, 6, 4, 4.7, 5, -1, 0, 1};
			double CP = 0.0;

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			double GetCP() override;
			double GetRP() override;
			double GetEP() override;
			double GetPos(int i) override;
			double GetSound(int i) override;
			void Init(double Press) override;
			TFVel6() = default;
	};

	class TFVE408 final : public TDriverHandle {
			double EPS = 0.0;
			static double const pos_table[11]; // = {-1, 6, -1, 0, 6, 4, 4.7, 5, -1, 0, 1};
			double CP = 0.0;

		public:
			double GetPF(double i_bcp, double PP, double HP, double dt, double ep) override;
			double GetCP() override;
			double GetEP() override;
			double GetRP() override;
			double GetPos(int i) override;
			double GetSound(int i) override;
			void Init(double Press) override;
			TFVE408() = default;
	};

	extern double PF(double P1, double P2, double S, double DP = 0.25);
	extern double PF1(double P1, double P2, double S);
	extern double PFVa(double PH, double PL, double S, double LIM, double DP = 0.1);
	// zawor napelniajacy z PH do PL, PL do LIM
	extern double PFVd(double PH, double PL, double S, double LIM, double DP = 0.1);

	// zawor wypuszczajacy z PH do PL, PH do LIM
	//
} // namespace Maszyna
