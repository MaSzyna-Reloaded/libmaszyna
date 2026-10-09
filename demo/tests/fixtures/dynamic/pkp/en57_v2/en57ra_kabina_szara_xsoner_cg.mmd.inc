models: main/(p1).t3d#
animations: 4 8 0 0 2 0 -1
lowpolyinterior: lowpoly/(p2)
animwheelprefix: wheel
animdoorprefix: door
coupleradapter: models/tabor/polsprzeg.t3d, 0.085, 0.861
loads: 
{
passengers: loads/(p3)
}
endmodels

locations:
 doors: -3.71 both 2.94 both end
 bogies: -7.59 7.27 end
endlocations

sounds:
curve: {
 amplitudefactor: 1.35
 amplitudeoffset: 1.35
 range: 100
 crossfade: 75
 placement: custom
 sound1: 697_104e_curve_1.wav
 sound30: 697_en57-skrzypi.wav
 sound45: curve_en57-1.wav
 sound60: curve_en57.wav
sound80: silence1.wav }
include sounds/stukoty_ra.inc end
include sounds/brake_external.inc end
include sounds/brakesound_external.inc end
derail: derail.wav 200
include [ sounds/horn_set1.inc sounds/horn_set2.inc sounds/horn_set3.inc sounds/horn_set4.inc ]
0.16 
10.485
-0.15 
10.486 
end
include sounds/sprezarka_(p4).inc end
departuresignal: { soundset: [ 26284_en57-986_buczek-start.wav|26284_en57-986_buczek-trwa.wav|26284_en57-986_buczek-stop.wav, 20786_en57_departure_buzzer_1.wav|20786_en57_departure_buzzer_2.wav|20786_en57_departure_buzzer_3.wav, 697_en57-door-signal-1.wav|697_en57-door-signal-2.wav|697_en57-door-signal-3.wav, 20786_door_signal_1_en71-016.wav|20786_door_signal_2_en71-026.wav|20786_door_signal_3_en71-026.wav, 4625_en57_905_signal_start.wav|4625_en57_905_signal.wav|4625_en57_905_signal_stop.wav, 4625_en57_1027_signal_start.wav|4625_en57_1027_signal.wav|4625_en57_1027_signal_stop.wav, 4625_en57_1805_signal_start.wav|4625_en57_1805_signal.wav|4625_en57_1805_signal_stop.wav, 4625_en57_1039_signal_start.wav|4625_en57_1039_signal.wav|4625_en57_1039_signal_stop.wav, 4625_en57_920_signal_start.wav|4625_en57_920_signal.wav|4625_en57_920_signal_stop.wav, 697_en57_door_buczek_start.wav|697_en57_door_buczek_trwa.wav|697_en57_door_buczek_end.wav, 20439_653-signal_start.wav|20439_653-signal_trwa.wav|20439_653-signal_stop.wav ] range: 25 placement: external pitchvariation: 0 }
sand: sand_1.wav sand_2.wav sand_3.wav 15 
releaser: releaser_1.wav releaser_2.wav releaser_3.wav 15 
include sounds/door_set_(p5).inc end
include sounds/doorlock_(p6).inc end
outernoise: {
 range: 130
 crossfade: 85
 sound0: silence1.wav pitch0: 35
 sound35: 697_en57_outernoise_35.wav pitch35: 35
 sound80: 697_en57_outernoise_80.wav pitch80: 40
 sound200: 697_en57_outernoise_80.wav pitch200: 80 }
unloading: 24150_en57_pasazery.flac
loading: 24150_en57_pasazery.flac
brakecylinderinc: { soundproofing: [ 0.04 0.25 0.04 0.64 0.64 0.64 ]
 sound1: 2193_en57-brakecyl-begin
 sound2: 2193_en57-brakecyl-low pitch2: 1.1
 sound5: 2193_en57-brakecyl-low pitch5: 1.0
 sound6: 2193_en57-brakecyl-high pitch6: 1.0
 sound9: 2193_en57-brakecyl-high pitch9: 0.9 }
unbrake: { soundbegin: 697_en57-luz-start.wav soundmain: 697_en57-luz-trwa.wav soundend: 697_en57-luz-koniec.wav soundproofing: [ 0.0001 0.25 0.0001 0.64 0.64 0.64 ] }
emergencybrake: { soundbegin: 20786_et22_emergency_brake_1 soundmain: 20786_et22_emergency_brake_2 soundend: 20786_et22_emergency_brake_3 placement: internal offset: [-0.61, 1.72, 9.40] }
endsounds 

internaldata:
rainsound: { sound0: rainsound_default.wav sound30: rain_normal.wav sound70: rain_heavy.wav amplitudefactor: 0.5 }
buzzer: [ buczek.wav 24150_buczek_cashp.wav 20786_203e_buzzer.wav 697_su45-czuwak.wav 14745_czuwak181.wav 07]bzyczek.wav bzyczek.wav et42_bzyczek.wav bzyczek4.wav bzyczek3.wav ]
tachoclock: { soundset: [ 26284_en57_hasler-start.wav|26284_en57_hasler-trwa.wav|26284_en57_hasler-stop.wav, 26284_en71_hasler-start.wav|26284_en71_hasler-trwa.wav|26284_en71_hasler-stop.wav ] amplitudefactor: 2.0 }
//switch: 697_en57-switch-1.wav
relay: 697_en57_ham-0.wav
pneumaticrelay: 697_en57_hiss.wav
couplerattach: 24150_en57_scharfenberg_zapiecie.flac
couplerdetach: 24150_pg_otwarcie_kurka.wav
//pneumaticswitch: 697_en57_ham-0.wav		//w przyszłości dorobi się dedyka pod ten dźwięk 
couplerstretch: en57_bufferclamp.wav
bufferclamp: en57_bufferclamp.wav
startjolt: { soundmain: [ 697_en57_szarpniecie.wav 697_en57_szarpniecie-2.wav 697_en57_szarpniecie-3.wav 697_en57_szarpniecie-4.wav en57_startjoint_891.flac en57_startjoint_928.flac ] placement: general }

include sounds/brakesound_internal.inc end
slipperysound: brake-slipp.wav 2.0 0.2 
airsound: [1007]fvel6_brake_1.wav 5 -0.01 
airsound2: [1007]fvel6_unbrake_15db_1.wav 5 -0.01 
// airsound3 w incu kabiny bo sie rozni dla knorra

include sounds/runningnoise_set(p7).inc end
include sounds/huntingnoise_(p8).inc end

mechspring: 100 0.02 0.2 0.2 0.03 0.15 0.02 0.01

soundproofing:
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 0.5 1.0 0.5 0.65 0.65 0.65
 
airsound3: 26284_en57_hamowanie_nagle.wav 10 -0.01
radiostop: [ 24150_radmor_rs_sygnal.wav 697_radiostop.wav ]

cab1definition: 
-1.8 1.2 8.83 1.8 1.1 9.5
driver1pos: 
0.0 2.8 8.93
driver1sitpos: 
-0.85 2.43 8.85
cab1model: interior\cab_xsoner\kabina_szara_xsoner_ra_cg.t3d

mainctrl: { nastawnik rot 0.045 0.0 0.08  sound0: 26284_en57_nastawnik-0.wav soundinc: 26284_en57_nastawnik-1 sounddec: 26284_en57_nastawnik-1_v3.wav sound2: 26284_en57_nastawnik-2.wav sound3: 26284_en57_nastawnik-3.wav sound4: 26284_en57_nastawnik-4.wav sound5: 26284_en57_nastawnik-5.wav sound6: [ 26284_en57_nastawnik-6.wav 26284_en57_nastawnik-6_v2.wav ] }
dirkey: { nawrotnik rot -0.05 0.01 0.1 
sound2: [ 26284_en57_kierunek_1.wav 26284_en57_kierunek_2.wav 26284_en57_kierunek_3.wav 26284_en57_kierunek_4.wav 26284_en57_kierunek_6.wav 26284_en57_kierunek_7.wav 26284_en57_kierunek_8.wav 26284_en57_kierunek_9.wav ] 
sound1: [ 26284_en57_kierunek_1.wav 26284_en57_kierunek_2.wav 26284_en57_kierunek_3.wav 26284_en57_kierunek_4.wav 26284_en57_kierunek_6.wav 26284_en57_kierunek_7.wav 26284_en57_kierunek_8.wav 26284_en57_kierunek_9.wav ] 
sound0: [ 26284_en57_kierunek_2.wav 26284_en57_kierunek_3.wav 26284_en57_kierunek_1.wav 26284_en57_kierunek_8.wav 26284_en57_kierunek_4.wav 26284_en57_kierunek_6.wav 26284_en57_kierunek_7.wav ] 
sound-1: [ 26284_en57_kierunek_2.wav 26284_en57_kierunek_1.wav 26284_en57_kierunek_4.wav 26284_en57_kierunek_3.wav 26284_en57_kierunek_7.wav 26284_en57_kierunek_6.wav 26284_en57_kierunek_9.wav 26284_en57_kierunek_8.wav ] }
brakectrl: { kran_zasadniczy rot -0.06 0.0 0.1
 sound-1: [ 26284_en57_kran_luzowanie.wav 26284_en57_kran_luzowanie_2.wav 26284_en57_kran_luzowanie_3.wav 26284_en57_kran_luzowanie_4.wav 26284_en57_kran_luzowanie_5.wav 26284_en57_kran_luzowanie_6.wav ]
 sound0: [ 26284_en57_kran_jazda.wav 26284_en57_kran_jazda_2.wav 26284_en57_kran_jazda_3.wav 26284_en57_kran_jazda_4.wav ]
 sound1: [ 26284_en57_kran_hamowanie.wav 26284_en57_kran_hamowanie_2.wav 26284_en57_kran_hamowanie_3.wav 26284_en57_kran_hamowanie_4.wav ]
 sound2: [ 26284_en57_kran_hamowanie.wav 26284_en57_kran_hamowanie_2.wav 26284_en57_kran_hamowanie_3.wav 26284_en57_kran_hamowanie_4.wav ]
 sound3: [ 26284_en57_kran_hamowanie.wav 26284_en57_kran_hamowanie_2.wav 26284_en57_kran_hamowanie_3.wav 26284_en57_kran_hamowanie_4.wav ]
 sound4: [ 26284_en57_kran_hamowanie.wav 26284_en57_kran_hamowanie_2.wav 26284_en57_kran_hamowanie_3.wav 26284_en57_kran_hamowanie_4.wav ]
 sound5: [ 26284_en57_kran_hamowanie.wav 26284_en57_kran_hamowanie_2.wav 26284_en57_kran_hamowanie_3.wav 26284_en57_kran_hamowanie_4.wav ]
 sound6: 26284_en57_kran_odciecie.wav }
 epctrlvalue: { epctrl rot 0 0 0 
 sound-1: [ 26284_en57_luzowanie_ep.wav 26284_en57_luzowanie_ep_2.wav 26284_en57_luzowanie_ep_3.wav 26284_en57_luzowanie_ep_4.wav ]
 sound0: [ 26284_en57_jazda_ep.wav 26284_en57_jazda_ep_2.wav 26284_en57_jazda_ep_3.wav 26284_en57_jazda_ep_4.wav ]
 sound1:  26284_en57_hamowanie_ep.wav }

main_on_bt: { wylszybki rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_1-on.wav sounddec: 26284_en57_hebelek_impulsowy_1-off.wav }
security_reset_bt: { przycisk-shp mov -0.007 0 0.1 soundinc: 26284_en57_przycisk_shp_1-on.wav sounddec: 26284_en57_przycisk_shp_1-off.wav }
fuse_bt: { odblprzeknadm rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_3-on.wav sounddec: 26284_en57_hebelek_impulsowy_3-off.wav }
compressor_sw: { sprezarka rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
converter_sw: { przetwzal rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_5-on.wav sounddec: 26284_en57_hebelek_impulsowy_5-off.wav }
converteroff_sw: { przetwwyl rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_4-on.wav sounddec: 26284_en57_hebelek_impulsowy_5-off.wav }
pantrear_sw: { panttylnypodniesiony rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_3-on.wav sounddec: 26284_en57_hebelek_impulsowy_3-off.wav }
pantfront_sw: { pantprzednipodniesiony rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_2-on.wav sounddec: 26284_en57_hebelek_impulsowy_2-off.wav }
pantfrontoff_sw: { pantprzedniopuszcz rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_1-on.wav sounddec: 26284_en57_hebelek_impulsowy_1-off.wav }
pantalloff_sw: { przycisk-pantopuszcz mov -0.005 0 0.1 type: return soundinc: 20786_203E_button_ws_of_2_on.wav sounddec: 20786_203E_button_ws_of_2_of.wav }
trainheating_sw: { ogrzewanie rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_2-on.wav sounddec: 26284_en57_hebelek_stabilny_2-off.wav }
radio_sw: { radio rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
upperlight_sw: { reflgorny rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
leftlight_sw: { refllewy rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_2-on.wav sounddec: 26284_en57_hebelek_stabilny_2-off.wav }
rightlight_sw: { reflprawy rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
leftend_sw: { lewyprzyc rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
rightend_sw: { prawyprzyc rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_2-on.wav sounddec: 26284_en57_hebelek_stabilny_2-off.wav }
instrumentlight_sw: { oswietlenieprzyz rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
cablight_sw: { oswietleniekab rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_1-on.wav sounddec: 26284_en57_hebelek_stabilny_1-off.wav }
dimheadlights_sw: { reflprzyc rot -0.18 0 0.1 soundinc: 26284_en57_hebelek_stabilny_2-on.wav sounddec: 26284_en57_hebelek_stabilny_2-off.wav }
compartmentlightson_sw: { oswietlzal5555 rot -0.2 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_2-on.wav sounddec: 26284_en57_hebelek_impulsowy_2-off.wav }
compartmentlightsoff_sw: { oswlwyl rot -0.2 0 0.1 soundinc: 26284_en57_hebelek_impulsowy_5-on.wav sounddec: 26284_en57_hebelek_impulsowy_5-off.wav }
hornlow_bt: hornlow rot 0.01 0 0.02
hornhigh_bt: hornhigh rot 0.01 0 0.02
radiochannel_sw: { swr-kanal rot -0.085 0 0.1 soundinc: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] sounddec: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] }
radiovolume_sw: { swr-glosnosc rot -0.7 0 0.1 soundinc: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] sounddec: [ 20786_radmor_pokretlo1.wav 20786_radmor_pokretlo2.wav 20786_radmor_pokretlo3.wav 20786_radmor_pokretlo4.wav 20786_radmor_pokretlo5.wav ] }
radiostop_sw: radiostop mov -0.005 0 0
radiocall3_sw: { swr-wywolanie3 mov -0.003 0 0.002 soundinc: 20786_203e_button_radio_3_on.wav sounddec: 20786_203e_button_radio_3_of.wav }
manualbrake: { hamulecreczny rot -0.439 0 0.5 sound0: hamulec_reczny_poczatek.wav sound1: 20786_201e_boczV2_dec_1.wav sound2: hamulec_reczny_1.wav sound3: hamulec_reczny_2.wav sound17: hamulec_reczny_4.wav sound18: hamulec_reczny_4.wav sound19: hamulec_reczny_4.wav sound20: hamulec_reczny_koniec.wav soundinc: hamulec_reczny_3.wav sounddec: hamulec_reczny_3.wav }
universal0: { klapka rot 0.25 0.0 0.25 soundinc: 20786_201e_szafka_open.wav sounddec: 20786_201e_szafka_close.wav }

tachometer: speedknob rot 0.0053975 0.0 0 
HVcurrent: amp_siln rot 0.00044625 0 0.3 
Hvoltage: wolt_wn rot 0.000062125 0 0.3 
Lvoltage: wolt_nn rot 0.00167 -0.142 0.3
brakepress: Cyl rot 0.73013699 0 0
pipepress: PrzGl rot 0.74193548 0 0
compressor: Zb rot 0.45146245 0 0
clock_seconds: {
 clockmhand rot 0 0 0
 soundinc: { soundmain: 26284_en57_hasler_clock.wav range: 2.0 amplitudefactor: 0.65 }
 sounddec: { soundmain: 26284_en57_hasler_clock.wav range: 2.0 amplitudefactor: 0.65 }
}
clock: analog 
distcounter: rt9 dgt 1 282160 0


//i-slippery: poslizg
//i-contactors: styczn
i-converteroff: przetw
i-motor_ovld: przeknadm
i-mainbreakeroff: szybki
//i-vent_ovld: nadm_went
//i-comp_ovld: nadm_spr
i-resistors: opory
//i-highcurrent: wys_rozr
i-security_aware: { czuwak soundinc: 20786_203e_light_ca_start.wav sounddec: 20786_203e_light_ca_stop.wav }
i-security_cabsignal: { shp soundinc: 20786_203e_light_shp_start.wav sounddec: 20786_203e_light_shp_stop.wav }
//i-reserve: rezerwa
i-departure_signal: odjazd
i-voltbrake: napnanast
i-radio: ir-wl
i-instrumentlight: podswietlenie
i-cablight: lampasufitowa

i-doorpermit_left: i-otwieranie_l
i-doorpermit_right: i-otwieranie_r
i-doors: i-zamykanie
i-door_blocked: i-blokada
doorleftpermit_sw: { bt-otwieranie_l mov -0.003 0 0.15 type: return soundinc: 20786_203E_button_ws_of_2_on.wav sounddec: 20786_203E_button_ws_of_2_of.wav }
doorrightpermit_sw: { bt-otwieranie_r mov -0.003 0 0.15 type: return soundinc: 20786_203E_button_ws_of_2_on.wav sounddec: 20786_203E_button_ws_of_2_of.wav }
doorallon_sw: { bt-otwieranie_centralne mov -0.003 0 0.15 soundinc: 20786_203E_button_ws_of_2_on.wav sounddec: 20786_203E_button_ws_of_2_of.wav }
dooralloff_sw: { bt-zamykanie mov -0.003 0 0.15 soundinc: 20786_203E_button_ws_of_2_on.wav sounddec: 20786_203E_button_ws_of_2_of.wav }
door_signalling_sw: { sw-blokada rot -0.125 0.125 0.1 soundinc: 26284_en57_hebelek_stabilny_2-on.wav sounddec: 26284_en57_hebelek_stabilny_2-off.wav }

pyscreen: timetable scripts/timetable
pyscreen: ekran_3000p scripts/srg3000p

cab0definition:
 -1.2 1.1 -0.15
  1.2 1.1  4.25
driver0pos:
0.0 2.8 2.0

cab2definition: 
 -1.2 1.1 -10.00
  1.2 1.1  -2.25 
driver2pos: 
0.0 2.8 -6.5

include misc/wyswietlacz_(p11).inc end