maxcamerapos = camerainterval;
mincamerapos = maxcamerapos * -1;
if ( KLpattern == 1) nmulticamera = nmulticamera_low;else nmulticamera = nmulticamera_high;

for (int multicameracounter = 3; multicameracounter <= nmulticamera; multicameracounter += 2){
  int tmpmulti;
  int tmpcounter1;
  int tmpcounter2;
  int twotmp;
 
  initialmulti = 0;
#include "vis.H"
  initialmulti = 1;
  for (int mdirection = 1; mdirection <=6; mdirection++) {
    tmpmulti = multicameracounter;
    for (tmpcounter2 = 1; 1; tmpcounter2*=2){//無限ループ
      twotmp = tmpcounter2^2;
      for(tmpcounter1 = 0; tmpcounter1 < twotmp; tmpcounter1++){
#include "multithree.H"//3×3×3のアルゴリズム
      }
      if(tmpmulti % 2 == 1){
          tmpmulti = (tmpmulti+1) / 2;
      }else{
          tmpmulti /=2;
      }
      if( tmpmulti == 2)
          break;//無限ループ回避
    }
  }
  maxcamerapos += camerainterval;
  mincamerapos -= camerainterval;
}
