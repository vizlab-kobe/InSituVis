




#include <PSNR.h>
std::ostringstream ss,sa,sb,sc;
ss << std::setw(5) << std::setfill('0') << now_time;
sa << std::setw(3) << std::setfill('0') << image1_x;
sb << std::setw(3) << std::setfill('0') << image1_y;
sc << std::setw(3) << std::setfill('0') << image1_z;
std::string num = ss.str();
std::string numx = sa.str();
std::string numy = sb.str();
std::string numz = sc.str();
std::string name1 = "./Output/output_result_mix_pbvr_u_" + num + "_" + numx + "_" + numy + "_" + numz + ".bmp";



sa << std::setw(3) << std::setfill('0') << image2_x;
sb << std::setw(3) << std::setfill('0') << image2_y;
sc << std::setw(3) << std::setfill('0') << image2_z;
numx = sa.str();
numy = sb.str();
numz = sc.str();
std::string name2 = "./Output/output_result_mix_pbvr_u_" + num + "_" + numx + "_" + numy + "_" + numz + ".bmp";


judgevis = do_PSNR(name1,name2);




//PSNR(2,"aiueo");
