#include <kvs/ColorImage>
#include <kvs/RGBColor>
#include <InSituVis/Lib/Viewpoint.h>

namespace InSituVis
{

inline void CameraFocusPredefinedControlledAdaptor::execRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;
    {
        kvs::Timer timer_rend;
        kvs::Timer timer_save;
        for (const auto& location : BaseClass::viewpoint().locations() )
        {
            // Draw and readback framebuffer
            // あなたが持っているワールド座標の注視点
            auto focus= isFocusPoint();

            const auto p0 = location.look_at - location.position; // 元々の視線ベクトル
            const auto p1 = focus - location.position;            // 新しい注視点へのベクトル
            const auto R = kvs::Quat::RotationQuaternion( p0, p1 );

            auto l = InSituVis::Viewpoint::Location(
                location.direction,
                location.position,
                kvs::Quat::Rotate( location.up_vector, R ), focus );
            l.index = location.index;
            l.look_at = focus;

            timer_rend.start();
            auto color_buffer = BaseClass::readback( l );
            timer_rend.stop();
            rend_time += m_rend_timer.time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( m_enable_output_image )
            {
                const auto size = this->outputImageSize( l );
                const auto width = size.x();
                const auto height = size.y();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( this->outputImageName( l ) );
            }
            timer_save.stop();
            save_time += m_save_timer.time( timer_save );
        }
    }
    m_rend_timer.stamp( rend_time );
    m_save_timer.stamp( save_time );
}

inline void CameraFocusPredefinedControlledAdaptor::estimateFocusPoint( 
    const kvs::ValueArray<float>& values ,
    const kvs::Vec3ui& dims)
{
    this->computeGradients(values,dims);
    std::cout<<"f point"<<std::endl;
    this->computeBlockEntropies(dims);
    std::cout<<"fo point"<<std::endl;
    this->printEntropies();
    std::cout<<"foc point"<<std::endl;
    this->setFocusPoint({0,0,0}); //注視点変更
}


// inline void CameraFocusPredefinedControlledAdaptor::updateFocusPoint( 
//     const kvs::ValueArray<float>& values ,
//     const kvs::Vec3ui& dims)
// {

//     m_gradients = this->computeGradients( values, dims );
//     this->computeBlockEntropies(dims);
//     // std::vector<BlockEntropy> sorted = blocks;
//     // std::sort(sorted.begin(), sorted.end(),
//     //             [](const BlockEntropy& a, const BlockEntropy& b) {
//     //                 return a.entropy > b.entropy;
//     //             });
    
// }

//ビンの数の変更に適用できるように要修正
inline size_t CameraFocusPredefinedControlledAdaptor::directionIndex( const kvs::Vector3f& g )
{
    float ax = std::fabs(g[0]);
    float ay = std::fabs(g[1]);
    float az = std::fabs(g[2]);

    if (ax >= ay && ax >= az)
    {
        return (g[0] >= 0.0f) ? 0 : 1;
    }
    else if (ay >= ax && ay >= az)
    {
        return (g[1] >= 0.0f) ? 2 : 3;
    }
    else
    {
        return (g[2] >= 0.0f) ? 4 : 5;
    }
}

//小領域ごとに勾配ベクトルのヒストグラムを作成 端は切り捨て
inline kvs::ValueArray<size_t> CameraFocusPredefinedControlledAdaptor::analyzeRegionDistribution(
    const kvs::Vec3ui& dims,
    const kvs::Vec3ui& region_min,
    const kvs::Vec3ui& region_max )
{
    kvs::ValueArray<size_t> histogram(6);
    histogram.fill(0);

    for (size_t z = region_min[2]; z < region_max[2]; ++z)
    {
        for (size_t y = region_min[1]; y < region_max[1]; ++y)
        {
            for (size_t x = region_min[0]; x < region_max[0]; ++x)
            {
                size_t idx = x + y * dims[0] + z * dims[0] * dims[1];
                const auto& g = m_gradients[idx];
                size_t dir = this->directionIndex(g);
                histogram[dir]++;
            }
        }
    }
    return histogram;
}

//数値データエントロピーを計算
inline float CameraFocusPredefinedControlledAdaptor::computeEntropy( const kvs::ValueArray<size_t>& histogram )
{
    size_t total = 0;
    for (size_t i = 0; i < histogram.size(); ++i) total += histogram[i];
    if (total == 0) return 0.0f;

    float H = 0.0f;
    for (size_t i = 0; i < histogram.size(); ++i)
    {
        if (histogram[i] > 0)
        {
            float p = static_cast<float>(histogram[i]) / static_cast<float>(total);
            H -= p * std::log2(p);
        }
    }
    return H;
}

//ブロックごとのエントロピー計算
inline void CameraFocusPredefinedControlledAdaptor::computeBlockEntropies(
    const kvs::Vec3ui& dims)
{
    //ブロックごとにループ
    for (size_t z = 0; z + block_size <= dims[2]; z += block_size)
    {
        for (size_t y = 0; y + block_size <= dims[1]; y += block_size)
        {
            for (size_t x = 0; x + block_size <= dims[0]; x += block_size)
            {
                kvs::Vec3ui region_min( x, y, z );
                kvs::Vec3ui region_max(
                    static_cast<kvs::UInt32>(std::min<size_t>(x + block_size, dims[0])),
                    static_cast<kvs::UInt32>(std::min<size_t>(y + block_size, dims[1])),
                    static_cast<kvs::UInt32>(std::min<size_t>(z + block_size, dims[2]))
                );

                auto hist = this->analyzeRegionDistribution(dims, region_min, region_max);
                float H = this->computeEntropy(hist);
                m_blockentorpy_list.push_back({ H, region_min, region_max, hist});
            }
        }
    }
}

//勾配ベクトルの計算
inline void CameraFocusPredefinedControlledAdaptor::computeGradients(
    const kvs::ValueArray<float>& values,
    const kvs::Vec3ui& dims )
{
    auto index = [&](int x, int y, int z) {
        return x + y * dims[0] + z * dims[0] * dims[1];
    };

    const size_t size = dims[0] * dims[1] * dims[2];
    // m_gradients.resize(size);
    std::cout<<dims[0]<<std::endl;
    std::cout<<dims[1]<<std::endl;
    std::cout<<dims[2]<<std::endl;
    for (int z = 0; z < dims[2]; ++z)
    {
        for (int y = 0; y < dims[1]; ++y)
        {
            for (int x = 0; x < dims[0]; ++x)
            {
                float dx, dy, dz;

                // x方向
                if (x > 0 && x < dims[0]-1)
                    dx = (values[index(x+1,y,z)] - values[index(x-1,y,z)]) * 0.5f;
                else if (x < dims[0]-1)
                    dx = values[index(x+1,y,z)] - values[index(x,y,z)];
                else
                    dx = values[index(x,y,z)] - values[index(x-1,y,z)];

                // y方向
                if (y > 0 && y < dims[1]-1)
                    dy = (values[index(x,y+1,z)] - values[index(x,y-1,z)]) * 0.5f;
                else if (y < dims[1]-1)
                    dy = values[index(x,y+1,z)] - values[index(x,y,z)];
                else
                    dy = values[index(x,y,z)] - values[index(x,y-1,z)];

                // z方向
                if (z > 0 && z < dims[2]-1)
                    dz = (values[index(x,y,z+1)] - values[index(x,y,z-1)]) * 0.5f;
                else if (z < dims[2]-1)
                    dz = values[index(x,y,z+1)] - values[index(x,y,z)];
                else
                    dz = values[index(x,y,z)] - values[index(x,y,z-1)];

                float len = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (len > 1e-12f)
                {
                    dx /= len;
                    dy /= len;
                    dz /= len;
                }

                // m_gradients[index(x,y,z)] = kvs::Vector3<float>(dx, dy, dz);
                // std::cout<<"/////"<<std::endl;
                m_gradients.push_back(kvs::Vector3<float>(dx, dy, dz));
                // std::cout <<  index(x,y,z) << "," << x <<"," << y <<"," << z << "," << dx <<"," << dy <<"," << dz << std::endl;

            }
        }
    }
    std::cout<<"for fin"<<std::endl;
}

//エントロピー出力
inline void CameraFocusPredefinedControlledAdaptor::printEntropies()
{
    // 降順にソート
    auto sorted = m_blockentorpy_list;
    std::sort(sorted.begin(), sorted.end(),
              [](const BlockEntropy& a, const BlockEntropy& b) {
                  return a.entropy > b.entropy;
              });

    //TOP3
    size_t topN = std::min<size_t>(10000000000, sorted.size());
    std::cout << "Top " << topN << " blocks by entropy:\n";
    size_t i;
    
    for (size_t i = 0; i < topN; ++i)
    {
        if (i == 0 || i == 599 || i == 299)
        {
            std::cout << i << ", " << sorted[i].entropy
            << ","
            << sorted[i].region_min[0] << "," << sorted[i].region_min[1] << "," << sorted[i].region_min[2]
            << ","
            << sorted[i].region_max[0]   << "," << sorted[i].region_max[1]   << "," << sorted[i].region_max[2]
            << ","
            << sorted[i].hist[0]   << "," << sorted[i].hist[1]   << "," << sorted[i].hist[2]
            << ","
            << sorted[i].hist[3]   << "," << sorted[i].hist[4]   << "," << sorted[i].hist[5] 
            << "\n";
            std::cout << sorted.size() << std::endl;
        }
    }
    // vis->setTmpMinCoord(kvs::Vec3(sorted[0].region_min[0],sorted[0].region_min[1],sorted[0].region_min[2]));
    // vis->setTmpMaxCoord(kvs::Vec3(sorted[0].region_max[0],sorted[0].region_max[1],sorted[0].region_max[2]));
}



} // end of namespace InSituVis

