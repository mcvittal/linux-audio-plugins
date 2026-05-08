#pragma once

struct DpsParam  { const char* name; const char* range; };
struct DpsEffect {
    int         number;
    const char* tag;        // front-panel code  e.g. "Plat1"
    const char* name;       // full name          e.g. "Plate Reverb 1"
    const char* type;       // "4ch" / "2ch" / "M-P"
    bool        tap;        // supports TAP
    int         paramCount;
    DpsParam    params[8];
};

// Index 0 is unused; indices 1-45 match the effect number.
// All data sourced from the Effect Parameter Guide (© 1995 Sony).
static const DpsEffect kEffects[46] =
{
  { 0,"","","",false,0,{} }, // 0 – unused

  // ── 4ch Effects ────────────────────────────────────────────────────────────
  { 1,"Plat1","Plate Reverb 1","4ch",false,7,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-300 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 2,"Hall1","Hall Reverb 1","4ch",false,7,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-400 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 3,"Room1","Room Reverb 1","4ch",false,7,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-400 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 4,"3Dim1","3 Dimension 1","4ch",false,4,
    {{"Pan 1","L180-R180"},{"Pan 2","L180-R180"},
     {"Pan 3","L180-R180"},{"Pan 4","L180-R180"}}},

  { 5,"DcCHO","Deca Chorus","4ch",false,5,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"PreDelay","0-1200 ms"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 6,"ENS","Ensemble","4ch",false,6,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"PreDelay","0-1200 ms"},{"Mode","1, 2"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 7,"Rotry","Rotary Speaker","4ch",true,8,
    {{"Speed","Slow, Fast"},{"Depth","0-100"},
     {"Drive","0-100"},{"Noise","0-100"},
     {"Ambience","0-100"},{"Speed Ratio","-20-20"},
     {"Speaker BAL","0-100"},{"Effect BAL","0-100"}}},

  { 8,"Vocdr","Vocoder","4ch",false,6,
    {{"Voice Sens","0-100"},{"Inst Sens","0-100"},
     {"Sibilance","0-100"},{"Noise","0-100"},
     {"Voice Mix","0-100"},{"Vocoder Mix","0-100"}}},

  { 9,"Doplr","Doppler","4ch",true,8,
    {{"Speed L","0-100"},{"Speed R","0-100"},
     {"Distance L","0-100"},{"Distance R","0-100"},
     {"Direction L","L→R, R→L"},{"Direction R","L→R, R→L"},
     {"Ambience","0-100"},{"Trigger","Signal, Tap"}}},

  // ── 2ch Effects ────────────────────────────────────────────────────────────
  { 10,"Plat2","Plate Reverb 2","2ch",false,7,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-150 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 11,"Hall2","Hall Reverb 2","2ch",false,7,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-150 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 12,"Room2","Room Reverb 2","2ch",false,7,
    {{"Rev Time","0.12-20 s"},{"PreDelay","0-150 ms"},
     {"Size","0.6-1.4"},{"Spread","0.6-1.4"},
     {"Hi Damp","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 13,"3Dim2","3 Dimension 2","2ch",false,2,
    {{"Pan 1","L180-R180"},{"Pan 2","L180-R180"}}},

  { 14,"E/R","Early Reflection","2ch",false,7,
    {{"Type","1-4"},{"Level Mode","Dec, Fix, Inc"},
     {"PreDelay","0-150 ms"},{"Diffusion L","0-100"},
     {"Diffusion R","0-100"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 15,"StDLY","Stereo Delay","2ch",true,8,
    {{"DlyTime L","0-1360 ms"},{"DlyTime R","0-1360 ms"},
     {"Feedback L","-99-+99"},{"Feedback R","-99-+99"},
     {"LPF L","100 Hz-Thru"},{"LPF R","100 Hz-Thru"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 16,"PpDLY","Ping Pong Delay","2ch",true,8,
    {{"DlyTime L","0-1360 ms"},{"DlyTime R","0-1360 ms"},
     {"Feedback L","-99-+99"},{"Feedback R","-99-+99"},
     {"LPF","100 Hz-Thru"},{"Mode","Cross, Normal"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 17,"StPCH","Stereo Pitch Shifter","2ch",false,8,
    {{"Pitch","-2400-+2400"},{"PitchDly L","0-500 ms"},
     {"PitchDly R","0-500 ms"},{"Pitch FB L","-99-+99"},
     {"Pitch FB R","-99-+99"},{"Mode","Fast, Soft"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 18,"RvSFT","Reverse Shifter","2ch",false,5,
    {{"Pitch","-1200-+1200"},{"Length","20-650"},
     {"Feedback","-99-+99"},{"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 19,"StCHO","Stereo Chorus","2ch",false,8,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"LFO Form","Sin, Tri"},{"PreDelay","0-500 ms"},
     {"LPF","100 Hz-Thru"},{"Spread","0-20"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 20,"StFLN","Stereo Flanger","2ch",false,8,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"Feedback","-99-99"},{"PreDelay","0-500 ms"},
     {"LPF","100 Hz-Thru"},{"Spread","0-20"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 21,"StPHS","Stereo Phaser","2ch",false,8,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"Manual","0-100"},{"Resonance","-99-+99"},
     {"Cross Mix","0-100"},{"Spread","0-20"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 22,"StPAN","Stereo Panner","2ch",true,6,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"LFO Form","Sin, Tri"},{"Ch Phase","0-20"},
     {"Attack","0-100"},{"Init. Mode","On, Off"}}},

  { 23,"HsPAN","Haas Panner","2ch",true,6,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"LFO Form","Sin, Tri"},{"Ch Phase","0-20"},
     {"Attack","0-100"},{"Init. Mode","On, Off"}}},

  { 24,"Drivr","Driver","2ch",false,6,
    {{"Gain","0-100"},{"Level","0-100"},
     {"Color","1-6"},{"Tone","-10-+10"},
     {"Blend","0-100"},{"N.R.","0-100"}}},

  { 25,"EQ","3 Band Equalizer","2ch",false,8,
    {{"Low Gain","-24-+12 dB"},{"Low Freq","100 Hz-6.3 kHz"},
     {"Mid Gain","-24-+12 dB"},{"Mid Freq","100 Hz-20 kHz"},
     {"Q","0.25-4.0"},{"High Gain","-24-+12 dB"},
     {"High Freq","400 Hz-20 kHz"},{"Level","Link, Dual"}}},

  { 26,"Amp","Amp Simulator","2ch",false,4,
    {{"Amp Mode","F, B, M, J"},{"Mic","Front/Slant/Upper/On"},
     {"Level L","0-100"},{"Level R","0-100"}}},

  { 27,"Limit","Limiter","2ch",false,8,
    {{"Threshold","0-100"},{"Ratio","1:1-1:∞"},
     {"Release","0-100"},{"Level","0-100"},
     {"Bass","-24-+12 dB"},{"Treble","-24-+12 dB"},
     {"Range","Flat, 0-10"},{"Mode","Link, Dual"}}},

  { 28,"Comp","Compressor","2ch",false,8,
    {{"Sens","0-100"},{"Attack","1:1-1:∞"},
     {"Release","0-100"},{"Level","0-100"},
     {"Bass","-24-+12 dB"},{"Treble","-24-+12 dB"},
     {"Range","Flat, 0-10"},{"Mode","Link, Dual"}}},

  { 29,"Excit","Exciter","2ch",false,4,
    {{"Excite Gain","0-100"},{"Frequency","1-32"},
     {"Type","1, 2"},{"Level","0-100"}}},

  { 30,"Gate","Gate","2ch",false,7,
    {{"GateTime","0-1000 ms"},{"Threshold","0-100"},
     {"Attack","0-100"},{"Release","0-100"},
     {"PreDelay","0-500 ms"},{"MaskTime","0-100"},
     {"Source","Lch, L+Rch, Rch"}}},

  { 31,"Treml","Tremolo","2ch",true,6,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"LFO Form","Sin, Tri"},{"Ch Phase","0-20"},
     {"Attack","0-100"},{"Init. Mode","On, Off"}}},

  { 32,"Vibrt","Vibrato","2ch",true,6,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"LFO Form","Sin, Tri"},{"Ch Phase","0-20"},
     {"Attack","0-100"},{"Init. Mode","On, Off"}}},

  { 33,"Wah","Auto Wah","2ch",false,4,
    {{"Sens","-100-100"},{"Attack","0-50"},
     {"Release","0-50"},{"Mode","Narrow/Normal/Wide/Low"}}},

  { 34,"PtROL","Pitch Roller","2ch",true,8,
    {{"Pitch","-100-100"},{"Mode","Fast, Slow"},
     {"Attack","0-100"},{"MaskTime","0-100"},
     {"Trigger","Signal, Tap"},{"Threshold","0-100"},
     {"Direct Level","0-100"},{"Effect Level","0-100"}}},

  { 35,"VoCNL","Vocal Canceler","2ch",false,2,
    {{"Position","Lch-Rch"},{"Spread","0-100"}}},

  { 36,"Freez","Freeze","2ch",true,6,
    {{"Start Point","-100-1200"},{"Stop Point","-100-1200"},
     {"Loop Point","-100-1200"},{"REC Ready","Off, On"},
     {"Trigger","Signal, Tap"},{"Threshold","0-100"}}},

  // ── Mono-Pair Effects ──────────────────────────────────────────────────────
  { 37,"RV/DL","Reverb / Delay","M-P",false,8,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-150 ms"},
     {"Hi Damp","0-100"},{"Rev BAL","Direct-Effect"},
     {"DelayTime","0-500 ms"},{"Delay FB","-99-+99"},
     {"LPF","100 Hz-Thru"},{"Dly BAL","Direct-Effect"}}},

  { 38,"RV/CH","Reverb / Chorus","M-P",false,8,
    {{"Rev Time","0.3-50 s"},{"PreDelay","0-150 ms"},
     {"Hi Damp","0-100"},{"Rev BAL","Direct-Effect"},
     {"Rate","0-100"},{"Depth","0-100"},
     {"Chr PreDly","0-500 ms"},{"Chr BAL","Direct-Effect"}}},

  { 39,"CH/DL","Chorus / Delay","M-P",true,8,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"Chr PreDly","0-500 ms"},{"Chr BAL","Direct-Effect"},
     {"DelayTime","0-500 ms"},{"Delay FB","-99-+99"},
     {"LPF","100 Hz-Thru"},{"Dly BAL","Direct-Effect"}}},

  { 40,"CH/CH","Chorus / Chorus","M-P",false,8,
    {{"Rate L","0-100"},{"Depth L","0-100"},
     {"PreDelay L","0-500 ms"},{"Chr BAL L","Direct-Effect"},
     {"Rate R","0-100"},{"Depth R","0-100"},
     {"PreDelay R","0-500 ms"},{"Chr BAL R","Direct-Effect"}}},

  { 41,"CH/PT","Chorus / Pitch","M-P",false,8,
    {{"Rate","0-100"},{"Depth","0-100"},
     {"Chr PreDly","0-500 ms"},{"Chr BAL","Direct-Effect"},
     {"Pitch","-2400-+2400"},{"Pitch Dly","0-500 ms"},
     {"Pitch FB","-99-+99"},{"Pitch BAL","Direct-Effect"}}},

  { 42,"PT/PT","Pitch / Pitch","M-P",false,8,
    {{"Pitch L","-2400-+2400"},{"Pitch Dly L","0-500 ms"},
     {"Pitch FB L","-99-+99"},{"Pitch BAL L","Direct-Effect"},
     {"Pitch R","-2400-+2400"},{"Pitch Dly R","0-500 ms"},
     {"Pitch FB R","-99-+99"},{"Pitch BAL R","Direct-Effect"}}},

  { 43,"PT/DL","Pitch / Delay","M-P",false,8,
    {{"Pitch","-2400-+2400"},{"Pitch Dly","0-500 ms"},
     {"Pitch FB","-99-+99"},{"Pitch BAL","Direct-Effect"},
     {"DelayTime","0-500 ms"},{"Delay FB","-99-+99"},
     {"LPF","100 Hz-Thru"},{"Dly BAL","Direct-Effect"}}},

  { 44,"EQ/EQ","EQ / EQ","M-P",false,8,
    {{"LwGain L","-24-+12 dB"},{"MdGain L","-24-+12 dB"},
     {"MdFreq L","100 Hz-20 kHz"},{"HiGain L","-24-+12 dB"},
     {"LwGain R","-24-+12 dB"},{"MdGain R","-24-+12 dB"},
     {"MdFreq R","100 Hz-20 kHz"},{"HiGain R","-24-+12 dB"}}},

  { 45,"CP/CP","Compressor / Compressor","M-P",false,8,
    {{"Sens L","0-100"},{"Attack L","0-100"},
     {"Release L","0-100"},{"Level L","0-100"},
     {"Sens R","0-100"},{"Attack R","0-100"},
     {"Release R","0-100"},{"Level R","0-100"}}},
};
