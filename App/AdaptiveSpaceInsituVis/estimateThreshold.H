if(estimatethreshold==1){
  //プレシミュレーションの時のデータの読み込みをして、thresholdを決定する。
  FILE *fp;
  fp = fopen("d.csv","r");
  fscanf(fp, "%lf,", &threshold);
  fscanf(fp, "%d,", &vis_skip);
  fscanf(fp, "%d", &noutperKL);
  noutperKL = noutperKL/step;
  estimatethreshold = 0;
 }
