//model pojazdu i definicje podmodeli:
models: (p1).t3d#
animations: 8 8 0 4 2 0 -1
lowpolyinterior: 61]lowpoly.t3d
animwheelprefix: wheel0
animdoorprefix: door0
endmodels

locations:
doors: -2.67 both end
tractionmotors: -7.61 end
bogies: -7.61 7.61 end
endlocations

sounds:
wheel_clatter: 70 -9.4 sn61_wheel_4.wav -5.6 sn61_wheel_5.wav 6.5 sn61_wheel_1.wav 9.0 sn61_wheel_2.wav end
engine: {
	soundmain: 697_sn61-motor.wav
	soundend: 697_sn61-motor-stop.wav
	range: 280.0
	offset: [ 0, 3, 9.4 ]
	amplitudefactor: 0.6
	amplitudeoffset: 1.15
	frequencyfactor: 0.09
	frequencyoffset: 0.0 //-0.05
}
brake: {
 amplitudefactor: 1.35
 amplitudeoffset: 0.85 
 range: 100
 crossfade: 85
 sound1: 20786]brake_111a_v2.WAV
 sound7: 20786]brake_111a_v2.WAV
 sound12: 20786]brake_111a_v2.WAV 
 sound15: silence1.wav
 sound30: silence1.wav
 sound200: silence1.wav }
      brakesound: {
 amplitudefactor: 1.7
 amplitudeoffset: 0.1 
 range: 130
 crossfade: 85
 placement: external
 soundproofing: [ 0 0.1 0 1 1 1 ]
 sound0: silence1.wav
 sound15: brake-15.wav
 sound25: brake-25.wav
 sound30: brake-30.wav
 sound40: brake-40.wav
 sound50: brake-50.wav
 sound60: brake-60.wav
 sound80: brake-80.wav
 sound100: brake-100.wav
 sound130: brake-130.wav 
 sound200: brake-130.wav }
unbrake: [1007]estluz.wav 50
brakeacc: { soundmain: [ brakeacc_est3f.wav [1007]est_bac.wav ] range: 50 }
derail: derail.wav 200
curve: {
 amplitudefactor: 1.35
 amplitudeoffset: 1.35
 range: 100
 crossfade: 75
 sound20: silence1.wav
 sound30: 104e_3]curve.wav
 sound80: silence1.wav }
horn1: 697_060da-horn4.wav 697_060da-horn5.wav 697_060da-horn6.wav 1000
horn2: 697_su45-horn3a.wav 697_su45-horn3b.wav 697_su45-horn3c.wav 1000
sand: sand_1.wav sand_2.wav sand_3.wav 15 
releaser: releaser_1.wav releaser_2.wav releaser_3.wav 15 
dooropen: 20786]doors_open_111a_v2.wav
doorclose: 20786]doors_close_111a_v2.wav
outernoise: { range: 160 soundmain: runningnoise.wav }
unloading: [ passengers_loading_1.wav passengers_loading_2.wav ]
loading: [ passengers_loading_1.wav passengers_loading_2.wav ]
emergencybrake: { soundbegin: 20786_et22_emergency_brake_1 soundmain: 20786_et22_emergency_brake_2 soundend: 20786_et22_emergency_brake_3 }
endsounds

internaldata:
rainsound: { sound0: rainsound_default.wav sound30: rain_normal.wav sound70: rain_heavy.wav amplitudefactor: 0.5 }
buzzer: [ buczek.wav 24150_buczek_cashp.wav 20786_203e_buzzer.wav 697_su45-czuwak.wav 14745_czuwak181.wav 07]bzyczek.wav bzyczek.wav et42_bzyczek.wav bzyczek4.wav bzyczek3.wav ]
tachoclock: { sound1: hasler_1_km.wav sound2: hasler_2_km.wav sound3: hasler_3_km.wav sound4: hasler_4_km.wav sound5: 20786_hasler_5km.wav sound10: 20786_hasler_10_km.wav sound20: 20786_hasler_20_km.wav sound30: 20786_hasler_30_km.wav sound40: 20786_hasler_40_km.wav sound50: 20786_hasler_50_km.wav  sound60: 20786_hasler_60_km.wav sound70: 20786_hasler_70_km.wav sound80: 20786_hasler_80_km.wav sound90: 20786_hasler_90_km.wav sound100: 20786_hasler_100_km.wav sound110: 20786_hasler_110_km.wav sound120: 20786_hasler_120_km.wav sound130: 20786_hasler_130_km.wav sound140: 20786_hasler_140_km.wav sound150: 20786_hasler_150_km.wav sound500: 20786_hasler_150_km.wav }
//switch: switch.wav
relay: pss.wav //relay.wav
pneumaticrelay: pneumaticrelay.wav
  couplerattach: [ couplerattach_v2.wav couplerattach_default.wav couplerattach.wav ]
  couplerdetach: [ couplerdetach_v2.wav couplerdetach_default.wav couplerdetach.wav ]
  brakehoseattach: brakehoseattach.wav  // przewod glowny hamulca zapiecie
  brakehosedetach: mainhosedetach.wav  // przewod glowny hamulca rozpiecie
  mainhoseattach: mainhoseattach.wav  // przewod zasilajacy zapiecie
  mainhosedetach: mainhosedetach.wav  // przewod zasilajacy rozpiecie
  controlattach: controlattach.wav // ukrotnienie zapiecie
  controldetach: controldetach.wav // ukrotnienie rozpiecie
  gangwayattach: gangwaydetach.wav // mostek zapiecie
  gengwaydetach: gengwaydetach.wav // mostek rozpiecie
  heatingattach: heatingattach.wav // ogrzewanie zapiecie
  heatingdetach: heatingdetach.wav // ogrzewanie rozpiecie
dooropen: 20786_switch_en57-1112.wav
doorclose: 20786_switch_en57-1112.wav
ignition: {
    soundmain: ignition-sn61.wav
    range: 280.0
    offset: [ 0, 3, 9.4 ]
}
 radiostop: [ 24150_radmor_rs_sygnal.wav 697_radiostop.wav ]
brakesound: 697_brake.wav 0.1 0 0.4 0.9
slipperysound: brake-slipp.wav 2.0 0.2
airsound:  { soundmain: [ 697_fv4a_syk.wav 697_ep05-hiss.wav sm42-powietrze.wav 07]hiss.wav ] amplitudefactor: 0.7 amplitudeoffset: -0.01 }
airsound2: [1007]fv4a_luz.wav 0.05 -0.01
airsound3: [1007]fv4a_e.wav 0.15 -0.01
airsound4: { soundmain: [ [1007]fv4a_x.wav 20786_et22_popelnianie.wav ] amplitudefactor: 0.15 amplitudeoffset: -0.01 }
airsound5: [1007]fv4a_t.wav 10.0 -0.01
localbrakesound: { soundset: [ 697_303e_localbrake-start.wav|697_303e_localbrake-end.wav|silence1.wav, 26284_201e_localbrakesound_start.wav|26284_201e_localbrakesound_main.wav|26284_201e_localbrakesound_stop.wav ] placement: internal amplitudeoffset: 0.2 amplitudefactor: 2.5 range: 50 }
localbrakesound2: { soundbegin: 697_303e_localbrake2-start.wav soundmain: 697_303e_localbrake2-end.wav soundend: silence1.wav placement: internal amplitudefactor: 0.3 }
runningnoise: { range: -1 amplitudefactor: 0.4 soundmain: runningnoise.wav }
engageslippery: {
	soundmain: runningnoisesn61.wav
	range: 30.0
	offset: [ 0, 1, 8.7 ]
	amplitudefactor: 1.5 //1.0
	amplitudeoffset: 2.0 //0.7
	frequencyfactor: 0.5
	frequencyoffset: 0.8
}

maxtachovel: 120
mechspring: 100      0.02 0.2 0.2 0.03 0.15  0.02   0.01

cab1definition:   //rozmiary kabiny po ktorej mozna chodzic
-1.464 1.5 10.23 1.464 1.5 11.113 
//-1.464 1.719 10.214 1.464 3.177 11.113 
driver1pos:       // pozycja poczatkowa
-1.108 2.98 10.622
driver1sitpos:    // pozycja siedzaca 
-0.716 2.614 10.514
cab1model: 61]kabina_a.t3d //nazwa modelu kabiny a

cab2definition:
-1.464 1.5 -11.113 1.464 1.5 -10.23
driver2pos:
0 2.98 -10.622
driver2sitpos:
0.716 2.614 -10.514
//nazwa modelu kabiny b
cab2model: 61]kabina_b.t3d

// parametry dla regulatorow (controllers):
mainctrl: { nastawnik rot 0.05 0.0 0.1 sound0: 697_sn61_nast-1.wav sound1: 697_sn61_nast-2.wav sound2: 697_sn61_nast-3.wav sound3: 697_sn61_nast-1.wav sound4: 697_sn61_nast-3.wav sound5: 697_sn61_nast-2.wav sound6: 697_sn61_nast-1.wav sound7: 697_sn61_nast-2.wav sound8: 697_sn61_nast-3.wav }
dirkey: { nawrotnik rot -0.2 0.01 0.1 soundinc: 20786_203E_reverser_inc_1.wav sounddec: 20786_203E_reverser_inc_1.wav sound0: 20786_203E_reverser_dec_1.wav }
brakectrl: { zasadniczy rot -0.073 0.0 0.1 sound-2: 20786_203E_nast_ham_odc_1.wav sound-1: 20786_203E_nast_ham_pop_1.wav sound0: 20786_203E_nast_ham_0_4.wav sound1: 20786_203E_nast_ham_1_1.wav sound4: 20786_203E_nast_ham_2_1.wav sound5: 20786_203E_nast_ham_3_1.wav sound6: 20786_203E_nast_ham_nag_1.wav }         // parametry dla kranow hamulcow
localbrake: { pomocniczy rot -0.05 0.0 0.1 soundinc: 20786_203e_additional_brake_1.wav sounddec: 20786_203e_additional_unbrake_1.wav sound0: 20786_203e_additional_strong_unbrake_1.wav sound10: 20786_203e_additional_strong_brake_1.wav }
ignitionkey: { klucz rot -0.1 0 0.15 soundinc: 20786_203e_button_sam_wl_roz_spre_2_on.wav sounddec: 20786_203e_button_sam_wl_roz_spre_2_on.wav }

alarmchain: { ackerman wip -0.1 0 0.05 soundinc: 20786_4e_control_5.wav sounddec: 20786_4e_bocznik_1.wav }

//przelaczniki (switches):
//przyciski monostabilne:
main_off_bt: { stop01  mov -0.005 0 0.1 soundinc: 20786_203E_button_ws_of_1_on.wav sounddec: 20786_203E_button_ws_of_1_of.wav }
main_on_bt: { start01  mov -0.005 0 0.1 soundinc: 20786_203E_button_ws_3_on.wav sounddec: 20786_203E_button_ws_3_of.wav }
horn_bt: { lapa rot -0.02 0.0 0.1 soundinc: 20786_203E_switch_horn_pom_high_2_on.wav sounddec: 20786_203E_switch_horn_pom_high_2_of.wav }
sand_bt: { piasecznica mov 0.004 0 0.1 soundinc: 20786_203E_button_releaser_1_on.wav sounddec: 20786_203E_button_releaser_1_of.wav }
leftlight_sw: { refl_lewy rot 0.25 0 0.1 soundinc: 20786_203E_left_red_light_2_on.wav sounddec: 20786_203E_left_red_light_2_of.wav }
rightlight_sw: { refl_prawy rot 0.25 0 0.1 soundinc: 20786_203E_switch_up_white_light_3_on.wav sounddec: 20786_203E_switch_up_white_light_3_of.wav }
dimheadlights_sw: { przyciemnienie_refl rot 0.25 0 0.1 soundinc: 20786_203E_switch_right_white_light_2_on.wav sounddec: 20786_203E_switch_right_white_light_2_of.wav }
instrumentlight_sw: { osw_przyrz_kab rot 0.25 0 0.1 soundinc: 20786_203E_switch_light_devices_2_on.wav sounddec: 20786_203E_switch_light_devices_2_of.wav }
cablight_sw: { osw_kab rot 0.25 0 0.1 soundinc: 20786_203E_switch_light_devices_2_on.wav sounddec: 20786_203E_switch_light_devices_2_of.wav }
battery_sw: { wylacznik_glowny rot 0.25 0 0.1 soundinc: 20786_203E_switch_battery_1_on.wav sounddec: 20786_203E_switch_battery_1_of.wav }
door_left_sw: { drzwi_lewe  rot -0.2 0 0.2 soundinc: silence1.wav sounddec: silence1.wav }
door_right_sw: { drzwi_prawe rot -0.2 0 0.2 soundinc: silence1.wav sounddec: silence1.wav }

radiochannel_sw: { swr-kanal rot -0.085 0 0.1 soundinc: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] sounddec: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] }
radiovolume_sw: { swr-glosnosc rot -0.7 0 0.1 soundinc: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] sounddec: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] }
radiostop_sw: radiostop mov -0.005 0 0
radio_sw: { swr-wl mov -0.004 0 0.002 soundinc: 20786_203e_button_radio_3_on.wav sounddec: 20786_203e_button_radio_3_of.wav }
radiocall3_sw: { swr-wywolanie3 mov -0.004 0 0.002 soundinc: 20786_203e_button_radio_3_on.wav sounddec: 20786_203e_button_radio_3_of.wav }

// parametry dla wskaznikow (measurement devices):
tachometer: speedknob rot 0.00545 0 0  //parametry dla wskazowki predkosciomierza
enrot1m: obr_silnik rot 0.02744 0 0.3         // nazwa, rodzaj ruchu, przelicznik, offset, lepkosc
enrot2m: obr_silnik01 rot 0.02744 0 0.3  
enrot3m: obr_silnik02 rot 0.02744 0 0.3 // dla max 3 wagonow w trakcji ukrotnionej
enrot1m: obr_silnik_swi rot 00.02744 0 0.3         // nazwa, rodzaj ruchu, przelicznik, offset, lepkosc
enrot2m: obr_silnik01_swi rot 0.02744 0 0.3  
enrot3m: obr_silnik02_swi rot 0.02744 0 0.3 // dla max 3 wagonow w trakcji ukrotnionej
engageratio: sprzknob rot 0.41 0 0.4 
maingearstatus: skrzknob rot 0.4 0 0.5
brakepress: cylhamknob rot 0.79 0 0
pipepress: przglknob rot 0.55 0 0
Lvoltage: voltomierz_nn rot -0.0007 0 0.3
manualbrake: { ham_reczny rot -0.439 0 1.9 sound0: hamulec_reczny_poczatek.wav sound1: 20786_201e_boczV2_dec_1.wav sound2: hamulec_reczny_1.wav sound3: hamulec_reczny_2.wav sound17: hamulec_reczny_4.wav sound18: hamulec_reczny_4.wav sound19: hamulec_reczny_4.wav sound20: hamulec_reczny_koniec.wav soundinc: hamulec_reczny_3.wav sounddec: hamulec_reczny_3.wav }
compressor: zbglknob rot 0.59 0 0.2
clock_seconds: {
 ClockMhand rot 0 0 0
 soundinc: { soundmain: 24014_haslerclocktick2.wav amplitudefactor: 0.65 range: 1.0 }
 sounddec: { soundmain: 24014_haslerclocktick2.wav amplitudefactor: 0.65 range: 1.0 }
}
clock: analog 
distcounter: hasler dgt 1 351349 0


//lampki (indicators):

i-trainheating: kociol
i-mainbreaker: pradnica
i-instrumentlight: pulpit
i-forward: przod
i-cablight: lampy_sufit
i-radio: ir-wl

//korytarz (corridor)
cab0definition:
-0.3 1.5 -4.8 0.2 1.5 4.8
driver0pos:
0.0 3.0 1.0
//nazwa modelu korytarza
cab0model: none
//na koncu zeby nie szukal nieistniejacych regulatorow i lampek!