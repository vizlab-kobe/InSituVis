if( mdirection == 1 || mdirection == 2){
  if(mdirection == 1)
    cameraposx = maxcamerapos;
  else
    cameraposx = maxcamerapos * -1;

  mthreefourminz = mincamerapos + (tmpcounter1 % tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxz = mthreefourminz + (maxcamerapos * 2 / tmpcounter2);
  mthreefourminy = mincamerapos + ((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxy = mthreefourminy + (maxcamerapos * 2 / tmpcounter2);
  if(tmpcounter1 % tmpcounter2 == 0){//2回実行するのを防いでいる。
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = mthreefourminy;
    image2_y = mthreefourmaxy;
    image1_z = mthreefourminz;
    image2_z = mthreefourminz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposz = mthreefourminz;
#include "vis.H"//左
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposz = mthreefourminz;
#include "vis.H"//左
    }
  }
  if(((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) == 0){//2回実行するのを防いでいる。
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = mthreefourminy;
    image2_y = mthreefourminy;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposy = mthreefourminy;
      cameraposz = (mthreefourminz * 2+ mthreefourmaxz) / 3;
#include "vis.H"//上
      cameraposy = mthreefourminy;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//上
    }
  }
  image1_x = maxcamerapos;
  image2_x = maxcamerapos;
  image1_y = mthreefourmaxy;
  image2_y = mthreefourmaxy;
  image1_z = mthreefourminz;
  image2_z = mthreefourmaxz;
#include "judge.H"  
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposy = mthreefourmaxy;
    cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//下
    cameraposy = mthreefourmaxy;
    cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//下
  }
  image1_x = maxcamerapos;
  image2_x = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_z = mthreefourmaxz;
  image2_z = mthreefourmaxz;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    cameraposz = mthreefourmaxz;
#include "vis.H"//右
    cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
    cameraposz = mthreefourmaxz;
#include "vis.H"//右
  }
  image1_x = maxcamerapos;
  image2_x = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_z = (mthreefourminz * 2 + mthreefourmaxz) / 3;
  image2_z = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image2_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//中央
    }
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image2_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//中央
    }
  }
  image1_x = maxcamerapos;
  image2_x = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_z = (mthreefourminz + mthreefourmaxz * 2) / 3;
  image2_z = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image2_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//中央
    }
    image1_x = maxcamerapos;
    image2_x = maxcamerapos;
    image1_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image2_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//中央
    }
  }  
 }
else if( mdirection == 3 || mdirection == 4){
   
  if(mdirection == 3)
    cameraposy = maxcamerapos;
  else
    cameraposy = maxcamerapos * -1;

  mthreefourminz = mincamerapos + (tmpcounter1 % tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxz = mthreefourminz + (maxcamerapos * 2 / tmpcounter2);
  mthreefourminx = mincamerapos + ((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxx = mthreefourminx + (maxcamerapos * 2 / tmpcounter2);
  if(tmpcounter1 % tmpcounter2 == 0){//2回実行するのを防いでいる。
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
    image1_z = mthreefourminz;
    image2_z = mthreefourminz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
      cameraposz = mthreefourminz;
#include "vis.H"//左
      cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
      cameraposz = mthreefourminz;
#include "vis.H"//左
    }
  }
  if(((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) == 0){//2回実行するのを防いでいる。
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = mthreefourminx;
    image2_x = mthreefourminx;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposx = mthreefourminx;
      cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//上
      cameraposx = mthreefourminx;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//上
    }
  }
  image1_y = maxcamerapos;
  image2_y = maxcamerapos;
  image1_x = mthreefourmaxx;
  image2_x = mthreefourmaxx;
  image1_z = mthreefourminz;
  image2_z = mthreefourmaxz;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposx = mthreefourmaxx;
    cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//下
    cameraposx = mthreefourmaxx;
    cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//下
  }
  image1_y = maxcamerapos;
  image2_y = maxcamerapos;
  image1_x = mthreefourminx;
  image2_x = mthreefourmaxx;
  image1_z = mthreefourmaxz;
  image2_z = mthreefourmaxz;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
    cameraposz = mthreefourmaxz;
#include "vis.H"//右
    cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
    cameraposz = mthreefourmaxz;
#include "vis.H"//右
  }
  image1_y = maxcamerapos;
  image2_y = maxcamerapos;
  image1_x = mthreefourminx;
  image2_x = mthreefourmaxx;
  image1_z = (mthreefourminz * 2 + mthreefourmaxz) / 3;
  image2_z = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
    image2_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposy = (mthreefourminx * 2 + mthreefourmaxx) / 3;
      cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//中央
    }
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
    image2_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
      cameraposz = (mthreefourminz * 2 + mthreefourmaxz) / 3;
#include "vis.H"//中央
    }
  }
  image1_y = maxcamerapos;
  image2_y = maxcamerapos;
  image1_x = mthreefourminx;
  image2_x = mthreefourmaxx;
  image1_z = (mthreefourminz + mthreefourmaxz * 2) / 3;
  image2_z = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
    image2_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//中央
    }
    image1_y = maxcamerapos;
    image2_y = maxcamerapos;
    image1_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
    image2_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
    image1_z = mthreefourminz;
    image2_z = mthreefourmaxz;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
      cameraposz = (mthreefourminz + mthreefourmaxz * 2) / 3;
#include "vis.H"//中央
    }
  }  
 }
 else{//mdirection ==5 or 6
   if(mdirection == 5)
     cameraposz = maxcamerapos;
   else
     cameraposz = maxcamerapos * -1;

  mthreefourminx = mincamerapos + (tmpcounter1 % tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxx = mthreefourminx + (maxcamerapos * 2 / tmpcounter2);
  mthreefourminy = mincamerapos + ((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) * (maxcamerapos * 2 / tmpcounter2);
  mthreefourmaxy = mthreefourminy + (maxcamerapos * 2 / tmpcounter2);
  if(tmpcounter1 % tmpcounter2 == 0){//2回実行するのを防いでいる。
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = mthreefourminy;
    image2_y = mthreefourmaxy;
    image1_x = mthreefourminx;
    image2_x = mthreefourminx;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposx = mthreefourminx;
#include "vis.H"//左
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposx = mthreefourminx;
#include "vis.H"//左
    }
  }
  if(((tmpcounter1 - (tmpcounter1 % tmpcounter2))/tmpcounter2) == 0){//2回実行するのを防いでいる。
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = mthreefourminy;
    image2_y = mthreefourminy;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }
    else{
      cameraposy = mthreefourminy;
      cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
#include "vis.H"//上
      cameraposy = mthreefourminy;
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
#include "vis.H"//上
    }
  }
  image1_z = maxcamerapos;
  image2_z = maxcamerapos;
  image1_y = mthreefourmaxy;
  image2_y = mthreefourmaxy;
  image1_x = mthreefourminx;
  image2_x = mthreefourmaxx;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposy = mthreefourmaxy;
    cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
#include "vis.H"//下
    cameraposy = mthreefourmaxy;
    cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
#include "vis.H"//下
  }
  image1_z = maxcamerapos;
  image2_z = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_x = mthreefourmaxx;
  image2_x = mthreefourmaxx;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }
  else{
    cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    cameraposx = mthreefourmaxx;
#include "vis.H"//右
    cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
    cameraposx = mthreefourmaxx;
#include "vis.H"//右
  }
  image1_z = maxcamerapos;
  image2_z = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
  image2_x = (mthreefourminx * 2 + mthreefourmaxx) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image2_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
#include "vis.H"//中央
    }
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image2_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposx = (mthreefourminx * 2 + mthreefourmaxx) / 3;
#include "vis.H"//中央
    }
  }
  image1_z = maxcamerapos;
  image2_z = maxcamerapos;
  image1_y = mthreefourminy;
  image2_y = mthreefourmaxy;
  image1_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
  image2_x = (mthreefourminx + mthreefourmaxx * 2) / 3;
#include "judge.H"
  if(judgevis == 0){
    ;//cpコマンドを実行する
  }else{
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image2_y = (mthreefourminy * 2 + mthreefourmaxy) / 3;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
#include "judge.H"
    if(judgevis == 0){
      ;//cpコマンドを実行する
    }else{
      cameraposy = (mthreefourminy * 2 + mthreefourmaxy) / 3;
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
#include "vis.H"//中央
    }
    image1_z = maxcamerapos;
    image2_z = maxcamerapos;
    image1_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image2_y = (mthreefourminy + mthreefourmaxy * 2) / 3;
    image1_x = mthreefourminx;
    image2_x = mthreefourmaxx;
#include "judge.H"
    if(judgevis == 0){
      ;
    }else{
      cameraposy = (mthreefourminy + mthreefourmaxy * 2) / 3;
      cameraposx = (mthreefourminx + mthreefourmaxx * 2) / 3;
#include "vis.H"//中央
    }
  }  
 }
