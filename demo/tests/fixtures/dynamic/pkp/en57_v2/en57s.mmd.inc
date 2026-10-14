models: main/(p1).t3d#
animations: 4 8 0 0 2 2 -1
lowpolyinterior: lowpoly/(p2)
animwheelprefix: wheel
animdoorprefix: door
animpantrd1prefix: ramiedolne1_pant
animpantrd2prefix: ramiedolne2_pant
animpantrg1prefix: ramiegorne1_pant
animpantrg2prefix: ramiegorne2_pant
animpantslprefix: slizg_pant
pantfactors: -7.897 7.893 4.382 0.01
loads: 
{
passengers: loads/(p3)
}
endmodels

locations:
 doors: -4.49 both 4.32 both end
 bogies: -7.992 7.992 end
 tractionmotors: -7.992 7.992 end
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
include sounds/stukoty_s.inc end
include sounds/engine_en57.inc end
include sounds/brake_external.inc end
include sounds/brakesound_external.inc end
derail: derail.wav 200
small-compressor: s-compressor-start.wav,s-compressor.wav,s-compressor-stop.wav 20
include sounds/przetwornica_(p4).inc end
pantographup: pantup.wav
pantographdown: pantdown.wav
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
endsounds

internaldata:
rainsound: { sound0: rainsound_default.wav sound30: rain_normal.wav sound70: rain_heavy.wav amplitudefactor: 0.5 }
relay: { sound0: 2193_en57-stycznik_lin-wyl.wav sound1: 2193_en57-stycznik_lin-zal.wav sound3: 2193_en57-wal_poz-np.wav sound5: 2193_en57-wal_poz-np.wav sound7: 2193_en57-wal_poz-np.wav sound9: 2193_en57-wal_poz-np.wav sound11: 2193_en57-wal_poz-np.wav sound13: 2193_en57-wal_poz-np.wav sound15: 2193_en57-wal_poz-np.wav sound17: 2193_en57-wal_poz-np.wav sound2: 2193_en57-wal_poz-p.wav sound4: 2193_en57-wal_poz-p.wav sound6: 2193_en57-wal_poz-p.wav sound8: 2193_en57-wal_poz-p.wav sound10: 2193_en57-wal_poz-p.wav sound12: 2193_en57-wal_poz-p.wav sound14: 2193_en57-wal_poz-p.wav sound16: 2193_en57-wal_poz-p.wav sound18: 2193_en57-wal_poz-p.wav sounddec: 2193_en57-wal_poz-np.wav }
shuntfield: { sound1: 2193_en57-wal_poz-p.wav sound2: 2193_en57-wal_poz-np.wav sound3: 2193_en57-wal_poz-p.wav }
pneumaticrelay: pneumaticrelay.wav
 couplerattach: [ couplerattach.wav 697_hak_zakladanie.wav ]
 couplerdetach: 24150_pg_otwarcie_kurka.wav
couplerstretch: { soundmain: [ en57_tarcie_gum_2.wav en57_tarcie_gum.wav ] placement: internal }
bufferclamp: en57_bufferclamp.wav
startjolt: { soundmain: [ 697_en57_szarpniecie.wav 697_en57_szarpniecie-2.wav en57_startjoint_891.flac en57_startjoint_928.flac ] placement: general }

include sounds/brakesound_internal.inc end
slipperysound: brake-slipp.wav 2.0 0.2 
airsound: [1007]fvel6_brake_1.wav 5 -0.01 
airsound2: [1007]fvel6_unbrake_15db_1.wav 5 -0.01 
airsound3: 26284_en57_hamowanie_nagle.wav 10 -0.01  

include sounds/runningnoise_set(p7).inc end
include sounds/huntingnoise_(p8).inc end

mechspring: 100 0.02 0.2 0.2 0.03 0.15 0.02 0.01
soundproofing:
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 -1 -1 -1 -1 -1 -1
 0.5 1.0 0.5 0.65 0.65 0.65

cab0definition:
-1.2 1.1 2.25
-0.8 1.1 4.75
driver0angle:
165 0
driver0pos:
-1.0 2.8 3.4
cab0model: interior/szafann.t3d

// szafa WN

universal1: drzwi_szafa rot 0.25 0 0.2
battery_sw: { sw-baterie rot -0.5 0 0.2 soundinc: 20786_4e_odl_nozowy_ladowania_przetw_1.wav sounddec: 20786_4e_odl_nozowy_ladowania_przetw_2.wav }
pantcompressorvalve_sw: { sw-odciecie_wn rot -0.25 0 0.2 soundinc: 20786_et22_kurek_trujdrogowy.wav sounddec: 20786_et22_kurek_odciecia-ukladu_pneumatycznego.wav }
pantcompressor_sw: { bt-przycisk007 mov -0.007 0 0.2 soundinc: 20786_203e_button_2_on.wav sounddec: 20786_203e_button_2_of.wav }
pantfront_sw: { bt-pant1_up mov -0.007 0 0.2 sounddec: p_up.wav }
pantrear_sw: { bt-pant2_up mov -0.007 0 0.2 sounddec: p_up.wav }
pantfrontoff_sw: { bt-pant1_down mov -0.007 0 0.2 sounddec: p_down.wav }
pantrearoff_sw: { bt-pant2_down mov -0.007 0 0.2 sounddec: p_down.wav }
pantpress: ws-mano_patyki rot -0.3833 0 0
lvoltage: ws-woltomierznn rot -0.001835 0.11444 0.5
main_on_bt: { bt-ws001 mov 0 0 0.2 soundinc: relay_glosny.wav }
universal2: { sw-zwarcie_pwr rot 0.25 0 0.2 soundinc: 20786_203e_przel_pakiet_5.wav sounddec: 20786_203e_przel_pakiet_3.wav }
universal3: { sw-zal_sprezarki_pantografow rot 0.25 0 0.2 soundinc: 20786_203e_przel_pakiet_5.wav sounddec: 20786_203e_przel_pakiet_3.wav }
relayreset1_bt: { bt-roznicowy mov -0.007 0 0.2 soundinc: 20786_203e_button_2_on.wav sounddec: 20786_203e_button_2_of.wav }
relayreset2_bt: { bt-nadmiarowy_grzania mov -0.007 0 0.2 soundinc: 20786_203e_button_2_on.wav sounddec: 20786_203e_button_2_of.wav }
converter_sw: { przekaznik_przetwornicy mov 0 0 0.2 soundinc: relay_glosny.wav }
compressor_sw: { sw-rozrzad_sprezarki_z_tablicy rot 0.25 0 0.2 soundinc: 20786_203e_przel_pakiet_5.wav sounddec: 20786_203e_przel_pakiet_3.wav }
i-diff_relay: i-roznicowy

cab1definition: 
 -1.2 1.1  4.00
  1.2 1.1 10.25 
driver1pos: 
0.0 2.8 8.0

cab2definition: 
 -1.2 1.1 -10.25
  1.2 1.1  -5.50 
driver2pos: 
0.0 2.8 -8.5

include misc/wyswietlacz_(p9).inc end