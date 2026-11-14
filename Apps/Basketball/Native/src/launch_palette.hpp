#pragma once
struct LaunchPalette {
 unsigned accent,accentLight,accentDeep,base,raised,inset,rail,line,text,body,muted,dim,shadow;
};
static LaunchPalette launchPalette(const unsigned* colors){
 unsigned a=colors[0],surface=colors[1],base=blend(colors[2],0x080b12,.60f);
 return {a,blend(a,0xffffff,.3f),blend(a,base,.28f),base,
  blend(base,surface,.72f),blend(base,0x080b12,.26f),
  blend(blend(base,surface,.55f),a,.13f),blend(base,a,.24f),
  blend(0xf5f7ff,a,.055f),blend(0xcbd5e7,a,.08f),
  blend(0x929eb4,a,.12f),blend(base,a,.48f),blend(base,0x000000,.7f)};
}
