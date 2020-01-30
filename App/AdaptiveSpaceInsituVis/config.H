/*全ての場合に次の変数を設定してください*/
const int mode = 0; //実行モード,0:本番実行,1:プレシミュレーション実行

/*mode = 1 のときは以降の設定は不要です*/

/*mode = 0のときは、まず次の変数を設定してください*/
const int step = 1; //細かく可視化するときの可視化間隔(Δt')/ボリュームデータ出力間隔
int vis_skip = 1;  //粗さの粒度R
int estimatethreshold = 0;//閾値を推定するかどうか,0:推定しない(手打ちで決める),1:presimulationにより推定する。
const int multicamera = 0; //マルチカメラ設定,0:1台のカメラ,1:複数のカメラ。

/*アンサンブル数の設定*/
const size_t repetitions = 100;//アンサンブル数

/*estimatethreshold=0;プレシミュレーションを使用しないときは次の変数を設定してください*/
float threshold = 0; //閾値
int noutperKL = 5; //各KL情報量計算までに行うボリュームデータ出力回数
//noutperKL×step=ΔTとなる。

/*estimatethreshold=1;プレシミュレーションを使用するときは次の変数を設定してください*/
int presim_number = 10;//どのプレシミュレーション結果を使うかを決定、デフォルトでは10から200までの数字のうち10刻みにしか設定できない
float I_R = 0.5;//重要領域率I(0or0<I_R<=1で設定)
float C_R = 0;//可視化コスト削減割合C(0or0<C_R<=1で設定)
//注意!I_RとC_Rはいずれかのみを選択して設定してください。設定しない方は0にしてください。両方0もしくは両方0以外の場合はエラーになります。


/*multicamera=1のときは次の３つを設定してください。*/
const int depthmulti_high = 3; //マルチカメラ時、KL情報量が閾値以上の場合、x,y,z,方向に何回まで分割して詳細化するか、1,2,3のいずれかに設定すること
const int depthmulti_low = 2; //マルチカメラ時、KL情報量が閾値未満の場合、x,y,z方向に何回まで分割して詳細化するか、1,2,3のいずれかかつdepthmulti_high以下の数値にすること
const float multicamera_posABS = 5.0; //マルチカメラ時、カメラポジションのx,y,z,方向の座標の最大値および最小値の絶対値
