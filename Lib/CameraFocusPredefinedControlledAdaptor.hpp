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

            //ここで注視点をカメラに設定（未実装）
            

            timer_rend.start();
            auto color_buffer = BaseClass::readback( location );
            timer_rend.stop();
            rend_time += m_rend_timer.time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( m_enable_output_image )
            {
                const auto size = this->outputImageSize( location );
                const auto width = size.x();
                const auto height = size.y();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( this->outputImageName( location ) );
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

    this->computeBlockEntropies(dims);

    //最大エントロピーを持つブロックを探す
    auto max_iter = std::max_element(
        m_blockentorpy_list.begin(),
        m_blockentorpy_list.end(),
        [](const BlockEntropy& a, const BlockEntropy& b) {
            return a.entropy < b.entropy;
        }
    );

    // ブロック中心(オブジェクト座標)をセット
    kvs::Vec3 object_focus(
        0.5f * (max_iter->region_min[0] + max_iter->region_max[0]),
        0.5f * (max_iter->region_min[1] + max_iter->region_max[1]),
        0.5f * (max_iter->region_min[2] + max_iter->region_max[2])
    );
    this->setFocusPoint(object_focus);
}


//ビンの数を変更できるように修正中
inline size_t CameraFocusPredefinedControlledAdaptor::directionIndex( const kvs::Vec3& g )
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
                kvs::Vec3ui region_max( x + block_size, y + block_size, z + block_size );

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
                m_gradients.push_back(kvs::Vec3(dx, dy, dz));
            }
        }
    }
}

} // end of namespace InSituVis

