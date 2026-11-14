#include <wincodec.h>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
static ComPtr<ID2D1Bitmap1> atlas;
static bool loadAtlas(){
 auto mod=GetModuleHandleW(nullptr);auto rs=FindResourceW(mod,MAKEINTRESOURCEW(101),RT_RCDATA);if(!rs)return false;
 auto bytes=(BYTE*)LockResource(LoadResource(mod,rs));auto len=SizeofResource(mod,rs);
 ComPtr<IWICImagingFactory> f;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> dec;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> conv;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f)))||
 FAILED(f->CreateStream(&stream))||FAILED(stream->InitializeFromMemory(bytes,len))||
 FAILED(f->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&dec))||
 FAILED(dec->GetFrame(0,&frame))||FAILED(f->CreateFormatConverter(&conv))||
 FAILED(conv->Initialize(frame.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 auto prop=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96);
 return SUCCEEDED(dc->CreateBitmapFromWicBitmap(conv.Get(),&prop,&atlas));
}
static void skin(){
 if(!atlas&&!loadAtlas())return;
 constexpr float x0=72,y0=51,sw=1393,sh=902;
 auto place=[&](float x,float y,float w,float h){return D2D1::RectF((x-x0)*W/sw,(y-y0)*H/sh,(x+w-x0)*W/sw,(y+h-y0)*H/sh);};
 auto blit=[&](float x,float y,float w,float h,float a,float b,float c,float d){
 dc->DrawBitmap(atlas.Get(),place(x,y,w,h),1,D2D1_INTERPOLATION_MODE_LINEAR,D2D1::RectF(a,b,a+c,b+d));};
 blit(x0,y0,sw,sh,x0,y0,sw,sh);
 // Runtime data uses the same local surface pixels as the surrounding panel.
 auto wipe=[&](float x,float y,float w,float h,float fromY){blit(x,y,w,h,x,fromY,w,2);};
 auto stamp=[&](float x,float y,float w,float h,std::wstring str,int format){auto r=place(x,y,w,h);T(str,format,r.left,r.top,r.right-r.left,r.bottom-r.top,C(0xf4f5ff));};
 auto swatch=[&](float x,float y,bool yes){blit(x,y,62,34,yes?700:1355,yes?341:346,62,34);};
 if(!armed)swatch(700,341,false);if(autoGuard)swatch(1355,346,true);
 if(!contestOutline)swatch(700,709,false);if(dribbleGlide)swatch(1355,708,true);
 int value=trim.load();
 if(value!=40){wipe(556,393,144,42,435);stamp(556,393,144,42,(value>=0?L"+":L"")+std::to_wstring(value)+L" ms",F_VAL);}
 // Move the release indicator relative to its reference calibration (+40).
 float norm=value<=40?(value+30.f)/70.f*.643f:.643f+(value-40.f)/20.f*.357f;
 if(value!=40){wipe(205,445,558,44,490);float x=209+551*norm;
 auto r=place(209,473,551,1);line(r.left,r.top,r.right,r.top,C(0xaab6dd),1);
 for(int i=0;i<=44;i++){auto t=place(209+551*i/44.f,473,0,-(i%4==0?12:6));line(t.left,t.top,t.left,t.bottom,C(0x9aa9cc),.65f);}
 float a=std::clamp(x-68.f,209.f,620.f);auto z=place(a,456,138,26);fillR({z.left,z.top,z.right-z.left,z.bottom-z.top},1,C(0xa1b3ff,.35f));strokeR({z.left,z.top,z.right-z.left,z.bottom-z.top},1,C(0xbdcaff,.65f));
 auto pt=place(x,453,0,32);line(pt.left,pt.top,pt.left,pt.bottom,C(0xffffff),1.2f);dot(pt.left,pt.top,4,C(0xffffff));}
 auto bar=[&](float x,float y,float width,float pct){wipe(x-9,y-11,width+18,23,y+16);auto a=place(x,y,width,0);auto b=a.left+(a.right-a.left)*pct;line(a.left,a.top,a.right,a.top,C(0x42475c),4);line(a.left,a.top,b,a.top,C(0x92a3ff),4);dot(b,a.top,4.8f,C(0xf5f6ff));};
 if(fabs(standOff-5)>.001){bar(847,545,404,standOff/10);wipe(1280,531,72,30,528);wchar_t n[16];swprintf_s(n,L"%.1f",standOff);stamp(1280,531,72,30,n,F_CTRL);}
 bar(212,817,544,contestCeiling/100.f);
 wipe(205,899,162,34,890);stamp(208,899,149,34,(livePing?L"Live ping  ":L"Est. ping  ")+std::to_wstring(pingMs.load())+L" ms",F_FOOT);
 if(!linked){auto a=place(190,915,0,0);dot(a.left,a.top,4,C(0x8c92a4));}
}
