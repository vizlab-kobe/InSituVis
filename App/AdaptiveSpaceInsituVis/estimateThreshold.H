int ret;
int prei = 0;
/*
  int interval = 1;
  double threshold = 0.0;*/
//プレシミュレーションの時のデータの読み込みをして、thresholdを決定する。
Info<< "Reading the data of presimulation...\n" << endl;

noutperKL = presim_number/step;
FILE *fp;
if(presim_number==10){
  fp = fopen("./presimulation/preentropy10.csv","r");
 }else if(presim_number==20){
  fp = fopen("./presimulation/preentropy20.csv","r");
 }else if(presim_number==30){
  fp = fopen("./presimulation/preentropy30.csv","r");
 }else if(presim_number==40){
  fp = fopen("./presimulation/preentropy40.csv","r");
 }else if(presim_number==50){
  fp = fopen("./presimulation/preentropy50.csv","r");
 }else if(presim_number==60){
  fp = fopen("./presimulation/preentropy60.csv","r");
 }else if(presim_number==70){
  fp = fopen("./presimulation/preentropy70.csv","r");
 }else if(presim_number==80){
  fp = fopen("./presimulation/preentropy80.csv","r");
 }else if(presim_number==90){
  fp = fopen("./presimulation/preentropy90.csv","r");
 }else if(presim_number==100){
  fp = fopen("./presimulation/preentropy100.csv","r");
 }else if(presim_number==110){
  fp = fopen("./presimulation/preentropy110.csv","r");
 }else if(presim_number==120){
  fp = fopen("./presimulation/preentropy120.csv","r");
 }else if(presim_number==130){
  fp = fopen("./presimulation/preentropy130.csv","r");
 }else if(presim_number==140){
  fp = fopen("./presimulation/preentropy140.csv","r");
 }else if(presim_number==150){
  fp = fopen("./presimulation/preentropy150.csv","r");
 }else if(presim_number==160){
  fp = fopen("./presimulation/preentropy160.csv","r");
 }else if(presim_number==170){
  fp = fopen("./presimulation/preentropy170.csv","r");
 }else if(presim_number==180){
  fp = fopen("./presimulation/preentropy180.csv","r");
 }else if(presim_number==190){
  fp = fopen("./presimulation/preentropy190.csv","r");
 }else if(presim_number==200){
  fp = fopen("./presimulation/preentropy200.csv","r");
 }else {
  Info<< "presim_number in config.H is wrong!!!!!\nPlease check the config.H" << endl;
  return 1;
 }
if(I_R<0.0)
  I_R=0;
if(I_R>1.0)
  I_R=1.0;
if(C_R<0.0)
  C_R=0;
if(C_R>1.0)
  C_R=1.0;
if((I_R*C_R==0)&&(I_R+C_R==0)){
  Info<< "I_R or C_R in config.H is wrong!!!!!\nPlease check the config.H" << endl;
  return 1;
 }
while ( ret = fscanf(fp, "%lf,", &inputentropy) != EOF){
  ordered_entropies[prei]=inputentropy;
  for(int ethr = prei; ethr >=0; ethr--){
    if(sorted_entropies[ethr]>inputentropy){
      sorted_entropies[ethr+1]=sorted_entropies[ethr];
    }else{
      sorted_entropies[ethr+1]=inputentropy;
    }
  }
  prei++;
  if(prei==20000/presim_number)//エラー回避
    break;
 }
int rating;
if(I_R==0){//コスト削減割合から閾値を決定
  if(vis_skip>noutperKL)
    rating=prei*(C_R);
  else
    rating=prei*(vis_skip/(vis_skip-1))*C_R;
 }
 else{//重要区間率から閾値を決定
   rating=prei*(1-I_R);
 }
rating = rating * 2/3;//RegionC=1/2RegionB
if(rating==0)rating++;//0のときは0を取るようにする
threshold = sorted_entropies[rating-1];

for(int judgeKL = 0; judgeKL < prei; judgeKL++){
  if(ordered_entropies[judgeKL]>threshold)
    presim_judgevis[judgeKL]=2;
  else if(judgeKL==0)//エラー回避
    presim_judgevis[judgeKL]=1;
  else if(ordered_entropies[judgeKL-1]>threshold)
    presim_judgevis[judgeKL]=3;
  else
    presim_judgevis[judgeKL]=1;
 }
Info<< "Finishing Reading the data of presimulation" << endl;

  

  
  







 
