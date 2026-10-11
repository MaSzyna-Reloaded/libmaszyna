// The fixtures' start-up line: the tracks, the wire and its power source of ep07.scn
// (scripts/cut-vehicle-fixture)

node -1 0 tdo_n5 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 36.1112  -2.5  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 136.111  0.0  //point 2
0
endtrack

node -1 0 start track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 136.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 236.111  0.0  //point 2
0
endtrack

node -1 0 tdo_n25 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 236.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 336.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n1 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 336.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 436.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n2 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 436.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 536.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n3 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 536.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 636.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n4 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 636.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 736.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n5 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 736.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 836.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n6 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 836.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 936.111  0.0  //point 2
0
endtrack

node -1 0 ep07_n7 track normal 100.0 1.435 0.15 25.0 20 0 flat vis
 rail_screw_used1 6 1435mm/tpbps-new2 0.2 0.5 1.1
0.396825 0.2 936.111  0.0  //point 1
0.0 0.0 0.0  //control vector 1
0.0 0.0 0.0  //control vector 2
0.396825 0.2 1036.1109999999999  0.0  //point 2
0
endtrack

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
10.1077 5.845 -72.2173
2.1656 5.845 -11.8011
10.1077 7.545 -72.2173
2.1656 7.545 -11.8011
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr1 3500 4500 0.01 cu 3.0 1
2.1656 5.845 -11.8011
0.0206653 5.80012 49.0872
2.1656 7.545 -11.8011
0.0206653 7.50012 49.0872
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.0206653 5.80012 49.0872
0.687223 5.8 100.247
0.0206653 7.50012 49.0872
0.687223 7.50012 100.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 100.247
0.087223 5.8 170.247
0.687223 7.5 100.247
0.087223 7.5 170.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 170.247
0.687223 5.8 240.247
0.087223 7.5 170.247
0.687223 7.5 240.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 240.247
0.087223 5.8 310.247
0.687223 7.5 240.247
0.087223 7.5 310.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 310.247
0.687223 5.8 380.247
0.087223 7.5 310.247
0.687223 7.5 380.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 380.247
0.087223 5.8 450.247
0.687223 7.5 380.247
0.087223 7.5 450.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 450.247
0.687223 5.8 520.247
0.087223 7.5 450.247
0.687223 7.5 520.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 520.247
0.087223 5.8 590.247
0.687223 7.5 520.247
0.087223 7.5 590.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 590.247
0.687223 5.8 660.247
0.087223 7.5 590.247
0.687223 7.5 660.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 660.247
0.087223 5.8 730.247
0.687223 7.5 660.247
0.087223 7.5 730.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 730.247
0.687223 5.8 800.247
0.087223 7.5 730.247
0.687223 7.5 800.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 800.247
0.087223 5.8 870.247
0.687223 7.5 800.247
0.087223 7.5 870.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 870.247
0.687223 5.8 940.247
0.087223 7.5 870.247
0.687223 7.5 940.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.687223 5.8 940.247
0.087223 5.8 1010.247
0.687223 7.5 940.247
0.087223 7.5 1010.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0  none traction pwr01 3500 4500 0.01 cu 3.0 1
0.087223 5.8 1010.247
0.687223 5.8 1080.247
0.087223 7.5 1010.247
0.687223 7.5 1080.247
0.4 4.0 3 0.04 vis
endtraction

node -1 0 pwr01 tractionpowersource 0 0 0 3400 0 0.01 2000 1.0 3 60.0 section end
