maxcamerapos = camerainterval;
mincamerapos = maxcamerapos * -1;
if ( KLpattern == 1) nmulticamera = nmulticamera_low;else nmulticamera = nmulticamera_high;

for (int multicameracounter = 3; multicameracounter <= nmulticamera; multicameracounter += 2){
  int tmpmulti;
  int tmpcounter1;
  int tmpcounter2;
  int twotmp;
  /*
    アルゴリズムの説明:n×n×nで配置しているカメラを、3×3×3の表面、5×5×5の表面、...、の順に可視化を行なっている。つまり、n×n×nの立方体を複数の立方体の内包関係(箱の中に箱が入っているイメージ)によって求めている。これにより再帰呼び出しと同じ考え方で実行している。
    また、各立方体をそれぞれ3×3もしくは4×4に分け、どちらかのアルゴリズムに落とし込み、計算を行う。
   */
  //maxcameraposの8箇所でまず実行
  initialmulti = 0;
#include "vis.H"
  initialmulti = 1;
  for (int mdirection = 1; mdirection <=6; mdirection++) {
    tmpmulti = multicameracounter;
    for (tmpcounter2 = 1; 1; tmpcounter2*=2){//無限ループ
      twotmp = tmpcounter2^2;
      for(tmpcounter1 = 0; tmpcounter1 < twotmp; tmpcounter1++){
#include "multithree.H"
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
