#include <kvs/ColorImage>
#include <kvs/RGBColor>
#include <InSituVis/Lib/Viewpoint.h>
#include <kvs/PointObject>
#include <kvs/SphereGlyph>

namespace InSituVis
{

inline void CameraFocusPredefinedControlledAdaptor::execRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;
    std::cout << "exr" << std::endl;
    {
        kvs::Timer timer_rend;
        kvs::Timer timer_save;
        for (const auto& location : BaseClass::viewpoint().locations() )
        {
            //ここで注視点をカメラに設定（未実装）
            auto* scene = BaseClass::screen().scene();
            auto* om = scene->objectManager();

            // const size_t nobjects = om->numberOfObjects();
            // for (size_t i = 0; i < nobjects; ++i)
            // {
            //     kvs::ObjectBase* obj = om->object(i);
            //     if (obj)
            //     {
            //         std::cout << "object[" << i << "] class: " 
            //                 << obj->name() << std::endl;
            //     }
            //     else
            //     {
            //         std::cout << "object[" << i << "] is null" << std::endl;
            //     }
            // }

            kvs::ObjectBase* obj = om->object( 4 ); //2個可視化オブジェクトを持っている必要がある
            kvs::Vec3 p = kvs::ObjectCoordinate( m_focus, obj ).toWorldCoordinate().position();
            // std::cout << p << "......." <<std::endl;
            auto updated_location = location;

            updated_location.look_at = p;
            BaseClass::screen().scene()->camera()->setPosition(
                updated_location.position,
                updated_location.look_at,
                updated_location.up_vector
            );
            
            // //zoomしたい
            auto t = 0.5;
            updated_location.position = ( 1 - t ) * location.position + t * p;

            this->pushMaxPositions(updated_location.position);
            this->pushMaxRotations(updated_location.rotation);
            this->pushMaxFocusPoints(updated_location.look_at);
            timer_rend.start();
            auto color_buffer = BaseClass::readback( updated_location );
            timer_rend.stop();
            rend_time += m_rend_timer.time( timer_rend );

            // Output framebuffer to image file
            timer_save.start();
            if ( m_enable_output_image )
            {
                const auto size = this->outputImageSize( updated_location );
                const auto width = size.x();
                const auto height = size.y();
                kvs::ColorImage image( width, height, color_buffer );
                image.write( this->outputFinalImageName( 10 ) );
            }
            timer_save.stop();
            save_time += m_save_timer.time( timer_save );
            
            //補間画像作成
            if ( this->isInitialStep() ) this->setIsInitialStep(false);
            else
            {
                this->createPath();
                this->popMaxPositions();
                this->popMaxRotations();
                this->popMaxFocusPoints();
                for ( size_t i = 0; i < 3; i++ )
                {
                    auto updated_location = location;
                    updated_location.look_at = this->focusPath().front();
                    // updated_location.position = this->path().front();
                    updated_location.position = ( 1 - t ) * location.position + t * updated_location.look_at;
                    BaseClass::screen().scene()->camera()->setPosition(
                        updated_location.position,
                        updated_location.look_at,
                        updated_location.up_vector
                    );
                    auto color_buffer = BaseClass::readback( updated_location );
                    if ( m_enable_output_image )
                    {
                        const auto size = this->outputImageSize( updated_location );
                        const auto width = size.x();
                        const auto height = size.y();
                        kvs::ColorImage image( width, height, color_buffer );
                        image.write( this->outputFinalImageName( i+1 ) );
                    }
                    this->focusPath().pop();
                    std::cout << "path size :" << focusPath().size() << std::endl;
                }
            }
            
        }
    }
    m_rend_timer.stamp( rend_time );
    m_save_timer.stamp( save_time );
}

inline std::string CameraFocusPredefinedControlledAdaptor::outputFinalImageName( const size_t level )
{
    const auto time = BaseClass::timeStep();
    const auto output_time = kvs::String::From( time, 6, '0' );
    const auto output_basename = BaseClass::outputFilename();
    const auto output_zoom_level = kvs::String::From( level, 6, '0' );
    const auto output_filename = output_basename + "_" + output_time + "_" + output_zoom_level;
    const auto filename = BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
    return filename;
}

inline void CameraFocusPredefinedControlledAdaptor::createPath()
{
    std::queue<std::pair<float, kvs::Quaternion>> empty;
    this->path().swap( empty );
    
    std::queue<kvs::Vec3> empty_focus;     // add
    this->focusPath().swap( empty_focus ); // add

    const auto positions = this->maxPositions();
    const auto rotations = this->maxRotations();
    const auto focuspoints = this->maxFocusPoints();

    const size_t num_points = 3;
    
    for ( size_t i = 0; i < num_points; i++ )
    {
        const auto t = static_cast<float>( i + 1 ) / static_cast<float>( num_points + 1 );
        const auto rad = this->radiusInterpolation( positions[0].length(), positions[1].length(), t );
        const auto rot = kvs::Quat::SphericalLinearInterpolation( rotations[0],rotations[1], t, true, true );
        const std::pair<float, kvs::Quaternion> elem( rad, rot );
        this->path().push( elem );
        const auto f = ( 1.0f - t ) * focuspoints[0] + t * focuspoints[1]; // add
        this->focusPath().push( f );              // add
    }
}

inline float CameraFocusPredefinedControlledAdaptor::radiusInterpolation( const float r1, const float r2, const float t )
{
    return ( r2 -r1 ) * t * t * ( 3.0f - 2.0f * t ) + r1;
}

inline void CameraFocusPredefinedControlledAdaptor::CreatePointObject( const std::vector<BlockEntropy>& blocks )
{
    const size_t npoints = blocks.size();
    const auto object_name = "entropy_blocks";
    if (npoints == 0)
    {
        std::cout << "No high-entropy blocks to visualize.\n";

        // 以前のオブジェクトが残っていれば削除
        if ( BaseClass::screen().scene()->hasObject(object_name) )
        {
            BaseClass::screen().scene()->removeObject(object_name);
        }
        return;
    }

    const float min_value = 3.7f;  // 固定の最小エントロピー値
    const float max_value = 3.75f;  // 固定の最大エントロピー値

    // ---- 1. 座標配列を作成（ブロック中心） ----
    kvs::ValueArray<float> coords( 3 * npoints );
    for (size_t i = 0; i < npoints; ++i)
    {
        const auto& b = blocks[i];
        kvs::Vec3 center = 0.5f * (kvs::Vec3(b.region_min) + kvs::Vec3(b.region_max));
        coords[3*i + 0] = center.x();
        coords[3*i + 1] = center.y();
        coords[3*i + 2] = center.z();
    }

    // ---- 2. 色の値として entropy をそのまま ValueArray に入れる ----
    kvs::ValueArray<float> values( npoints );
    for (size_t i = 0; i < npoints; ++i)
    {
        values[i] = blocks[i].entropy;
    }

    // ---- 3. 色マッピング（固定範囲） ----
    auto cmap = kvs::ColorMap::CoolWarm();
    cmap.setRange( min_value, max_value );   // 固定範囲

    kvs::ValueArray<kvs::UInt8> colors( 3 * npoints );
    for (size_t i = 0; i < npoints; ++i)
    {
        const auto c = cmap.at( values[i] );
        colors[3*i+0] = c.r();
        colors[3*i+1] = c.g();
        colors[3*i+2] = c.b();
    }

    // ---- 4. 座標を出力（デバッグ） ----
    for (size_t i = 0; i < npoints; ++i)
    {
        std::cout << "Point " << i << ": ("
                  << coords[3*i+0] << ", "
                  << coords[3*i+1] << ", "
                  << coords[3*i+2] << ")"
                  << "  Entropy = " << values[i]
                  << std::endl;
    }
    const auto Xform = kvs::Xform(
        kvs::Vec3::Constant( 0 ), // translation
        kvs::Vec3::Constant( 1 ), // scaling
        kvs::Mat3::RotationY( 35 ) * kvs::Mat3::RotationX( -50 ) ); // rotation

    // ---- 5. KVS PointObject と SphereGlyph ----
    auto point = new kvs::PointObject();
    point->setCoords( coords );
    point->setColors( colors );
    point->setSize( 5.0 );
    point->updateMinMaxCoords();
    point->setName( object_name );
    point->multiplyXform( Xform );

    auto* glyph = new kvs::SphereGlyph();
    glyph->setNumberOfSlices( 20 );
    glyph->setNumberOfStacks( 20 );

    if ( BaseClass::screen().scene()->hasObject( object_name ) )
    {
        BaseClass::screen().scene()->replaceObject( object_name, point );
    }
    else
    {
        BaseClass::screen().registerObject( point, glyph );
    }
}


inline void CameraFocusPredefinedControlledAdaptor::estimateFocusPoint( 
    const kvs::ValueArray<float>& values ,
    const kvs::Vec3ui& dims)
{

    this->computeGradients(values,dims);

    this->computeBlockEntropies(dims);

    // const size_t N = 10;

    // // 全体コピー
    // std::vector<BlockEntropy> topN = m_blockentorpy_list;

    // // 上位 N 個を抽出
    // if (topN.size() > N)
    // {
    //     std::nth_element(
    //         topN.begin(),
    //         topN.begin() + N,
    //         topN.end(),
    //         [](const BlockEntropy& a, const BlockEntropy& b) {
    //             return a.entropy > b.entropy;
    //         }
    //     );

    //     topN.resize(N);
    // }

    // // 降順に揃える
    // std::sort(
    //     topN.begin(),
    //     topN.end(),
    //     [](const BlockEntropy& a, const BlockEntropy& b) {
    //         return a.entropy > b.entropy;
    //     }
    // );

    // // ---- ここでエントロピー出力 ----
    // std::cout << "Top " << N << " Entropy values:\n";
    // for (size_t i = 0; i < topN.size(); ++i)
    // {
    //     std::cout << i << ": " << topN[i].entropy << "\n";
    // }

    //最大エントロピーを持つブロックを探す
    auto max_iter = std::max_element(
        m_blockentorpy_list.begin(),
        m_blockentorpy_list.end(),
        [](const BlockEntropy& a, const BlockEntropy& b) {
            return a.entropy < b.entropy;
        }
    );

    // std::cout << "max ent : "  <<  max_iter->entropy << std::endl;

    // ブロック中心(オブジェクト座標)をセット
    kvs::Vec3 object_focus(
        0.5f * (max_iter->region_min[0] + max_iter->region_max[0]),
        0.5f * (max_iter->region_min[1] + max_iter->region_max[1]),
        0.5f * (max_iter->region_min[2] + max_iter->region_max[2])
    );
    this->setFocusPoint(object_focus);

    //

    const float threshold = 3.7f;

    std::vector<BlockEntropy> high_entropy_blocks;
    high_entropy_blocks.reserve(m_blockentorpy_list.size()); // 無駄な再確保を避ける

    for (const auto& b : m_blockentorpy_list)
    {
        if (b.entropy > threshold)
        {
            high_entropy_blocks.push_back(b);
        }
    }

    // this->CreatePointObject(high_entropy_blocks);

    m_blockentorpy_list.clear();
    m_gradients.clear();
}

//ヒストグラムver2
inline std::vector<kvs::Vec3> CameraFocusPredefinedControlledAdaptor::buildSphereDirections(size_t t)
{
    std::vector<kvs::Vec3> directions;

    for (int i = 0; i < t; ++i)
    {
        // 傾斜角 θ_i = πi / t
        double theta = M_PI * i / (t-1);

        // 方位角の分割数 aθi = floor( 2 t sinθ + 1 )
        int a_theta = static_cast<int>(std::floor(2.0 * (t-1) * std::sin(theta) + 1));

        for (int j = 0; j < a_theta; ++j)
        {
            // φ_j = 2πj / aθi
            double phi = 2.0 * M_PI * j / a_theta;

            // 球面座標 -> デカルト座標
            kvs::Vec3 v;
            v.x() = std::sin(theta) * std::cos(phi);
            v.y() = std::sin(theta) * std::sin(phi);
            v.z() = std::cos(theta);

            directions.push_back(v);
        }
    }

    return directions;
}

inline kvs::ValueArray<float> CameraFocusPredefinedControlledAdaptor::analyzeRegionDistribution(
    const kvs::Vec3ui& dims,
    const kvs::Vec3ui& region_min,
    const kvs::Vec3ui& region_max
)
{
    const auto& bins = buildSphereDirections(bin_size);
    const float cos_alpha = std::cos(alpha_deg * M_PI / 180.0);

    kvs::ValueArray<float> histogram(bins.size());
    histogram.fill(0.0f);

    for(size_t z = region_min[2]; z < region_max[2]; ++z)
    {
        for(size_t y = region_min[1]; y < region_max[1]; ++y)
        {
            for(size_t x = region_min[0]; x < region_max[0]; ++x)
            {
                const size_t idx = x + y*dims[0] + z*dims[0]*dims[1];
                const kvs::Vec3& n = m_gradients[idx];

                for (size_t b = 0; b < bins.size(); ++b)
                {
                    const float dot = n.x() * bins[b].x() + n.y() * bins[b].y() + n.z() * bins[b].z();
                    if (dot >= cos_alpha)
                    {
                        const float w = (dot - cos_alpha) / (1.0f - cos_alpha);
                        histogram[b] += w;
                    }
                }
            }
        }
    }

    return histogram;
}


//数値データエントロピーを計算
inline float CameraFocusPredefinedControlledAdaptor::computeEntropy( const kvs::ValueArray<float>& histogram )
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

