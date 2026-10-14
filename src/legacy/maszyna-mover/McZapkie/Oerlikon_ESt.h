#pragma once
#ifndef INCLUDED_OERLIKON_EST_H
#define INCLUDED_OERLIKON_EST_H
/*fizyka hamulcow Oerlikon ESt dla symulatora*/

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
   Brakes. Oerlikon ESt.
   Copyright (C) 2007-2014 Maciej Cierniak
   */

#include <memory>
#include "friction.h" // Pascal unit
#include "hamulce.h" // Pascal unit

/*
   (C) youBy
   Co jest:
   - glowny przyrzad rozrzadczy
   - napelniacz zbiornika pomocniczego
   - napelniacz zbiornika sterujacego
   - zawor podskoku
   - nibyprzyspieszacz
   - tylko 16",14",12",10"
   - nieprzekladnik rura
   - przekladnik 1:1
   - przekladniki AL2
   - przeciwposlizgi
   - rapid REL2
   - HGB300
   - inne srednice
   Co brakuje:
   - dobry przyspieszacz
   - mozliwosc zasilania z wysokiego cisnienia ESt4
   - ep: EP1 i EP2
   - samoczynne ep
   - PZZ dla dodatkowego
   */

using namespace Maszyna;

namespace Maszyna {
	static constexpr int dMAX = 11; // dysze
	static constexpr int dON = 0; // osobowy napelnianie (+ZP)
	static constexpr int dOO = 1; // osobowy oproznianie
	static constexpr int dTN = 2; // towarowy napelnianie (+ZP)
	static constexpr int dTO = 3; // towarowy oproznianie
	static constexpr int dP = 4; // zbiornik pomocniczy
	static constexpr int dSd = 5; // zbiornik sterujacy
	static constexpr int dSm = 6; // zbiornik sterujacy
	static constexpr int dPd = 7; // duzy przelot zamykajcego
	static constexpr int dPm = 8; // maly przelot zamykajacego
	static constexpr int dPO = 9; // zasilanie pomocniczego O
	static constexpr int dPT = 10; // zasilanie pomocniczego T

	// przekladniki
	static constexpr int p_none = 0;
	static constexpr int p_rapid = 1;
	static int const p_pp = 2;
	static int const p_al2 = 3;
	static int const p_ppz = 4;
	static int const P_ed = 5;

	/**
	 * Przekładnik
	 */
	class TPrzekladnik : public TReservoir {
		public:
			std::shared_ptr<TReservoir> BrakeRes;
			std::shared_ptr<TReservoir> Next;
			TPrzekladnik() : TReservoir(){};
			virtual void Update(double dt);
			virtual void SetSlippery(bool flag) {}
			virtual void SetP(double P) {}
			virtual void SetMult(double m) {}
			virtual void SetLBP(double P) {}
			virtual void SetRapidParams(double mult, double size) {}
			virtual void SetRapidStatus(bool rs) {}
	};

	/**
	 * Nie przekłądnia, rura łącząca
	 */
	class TRura : public TPrzekladnik {
		public:
			TRura() = default;
			double P() override;
			void Update(double dt) override;
	};

	/**
	 * Przy napełnianiu - rura
	 * Przy poślizgu - upust
	 */
	class TPrzeciwposlizg : public TRura {
			bool Slippery;

		public:
			void SetSlippery(bool flag) override;
			void Update(double dt) override;
			TPrzeciwposlizg() : TRura() { Slippery = false; }
	};

	/**
	 * Przekładnik dwustopniowy
	 */
	class TRapid : public TPrzekladnik {
			bool RapidStatus = false; // status rapidu
			double RapidMult = 0.0; // przelozenie (w dol)
									//        Komora2: real;
			double DN = 0.0;
			double DL = 0.0; // srednice dysz napelniania i luzowania

		public:
			void SetRapidParams(double mult, double size) override;
			void SetRapidStatus(bool rs) override;
			void Update(double dt) override;
			TRapid() = default;
	};

	/**
	 * AL2
	 */
	class TPrzekCiagly : public TPrzekladnik {
			double Mult = 0.0;

		public:
			void SetMult(double m) override;
			void Update(double dt) override;
			TPrzekCiagly() = default;
	};

	/**
	 * Podwójny zawór zwrotny
	 */
	class TPrzek_PZZ : public TPrzekladnik {
			double LBP = 0.0;

		public:
			void SetLBP(double P) override;
			void Update(double dt) override;
			TPrzek_PZZ() = default;
	};

	/**
	 * Knicksventil
	 */
	class TPrzekZalamany : public TPrzekladnik {
		public:
			TPrzekZalamany() = default;
	};

	/**
	 * Przy napełnianiu - rura
	 * Przy hamowaniu - upust
	 */
	class TPrzekED : public TRura {
			double MaxP = 0.0;

		public:
			void SetP(double P) override;
			void Update(double dt) override;
			TPrzekED() : TRura() {}
	};

	class TNESt3 : public TBrake {
			std::shared_ptr<TReservoir> CntrlRes; // zbiornik sterujacy
			std::shared_ptr<TReservoir> Miedzypoj; // pojemnosc posrednia (urojona) do napelniania ZP i ZS
			std::shared_ptr<TPrzekladnik> Przekladniki[4];
			double Nozzles[dMAX]{}; // dysze
			double BVM = 0.0; // przelozenie PG-CH
							  //        ValveFlag: byte;           //polozenie roznych zaworkow
			bool Zamykajacy = true; // pamiec zaworka zamykajacego
									//        Przys_wlot: boolean;       //wlot do komory przyspieszacza
			bool Przys_blok = true; // blokada przyspieszacza
			bool RapidStatus = true;
			bool RapidStaly = true;
			double LoadC = 0.0;
			double TareM = 0.0; // masa proznego
			double LoadM = 0.0; // masa pelnego
			double TareBP = 0.0; // cisnienie dla proznego
			double HBG300 = 0.0; // zawor ograniczajacy cisnienie
			double Podskok = 0.0; // podskok preznosci poczatkowej
								  //        HPBR: real;               //zasilanie ZP z wysokiego cisnienia
			bool autom = true; // odluzniacz samoczynny
			double LBP = 0.0; // cisnienie hamulca pomocniczego

		public:
			TNESt3(double i_mbp, double i_bcr, double i_bcd, double i_brc, int i_bcn, int i_BD, int i_mat, int i_ba,
				   int i_nbpa) : TBrake(i_mbp, i_bcr, i_bcd, i_brc, i_bcn, i_BD, i_mat, i_ba, i_nbpa) {}
			void Init(double PP, double HPP, double LPP, double BP, int BDF) override;
			double GetPF(double PP, double dt, double Vel) override /*override*/;
			// przeplyw miedzy komora wstepna i PG
			static void EStParams(double i_crc); // parametry charakterystyczne dla ESt
			double GetCRP() override /*override*/;
			void CheckState(double BCP, double &dV1); // glowny przyrzad rozrzadczy
			void CheckReleaser(double dt); // odluzniacz
			double CVs(double BP) const; // napelniacz sterujacego
			double BVs(double BCP) const; // napelniacz pomocniczego
			void SetSize(int size, std::string const &params); // ustawianie dysz (rozmiaru ZR), przekladniki
			void PLC(double mass) override; // wspolczynnik cisnienia przystawki wazacej
			void SetLP(double TM, double LM, double TBP) override; // parametry przystawki wazacej
			void ForceEmptiness() override; // wymuszenie bycia pustym
			void SetLBP(double P) override; // cisnienie z hamulca pomocniczego
	};

	extern double d2A(double d);

#endif // INCLUDED_OERLIKON_EST_H
	   // END
}
