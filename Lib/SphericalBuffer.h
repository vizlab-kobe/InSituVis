#pragma once
#include <kvs/ValueArray>
#include <kvs/Vector2>
#include <kvs/CubicImage>
#include <array>


namespace InSituVis
{

template <typename PixelType>
class SphericalBuffer
{
public:
    using Direction = kvs::CubicImage::Direction;
    using Buffer = kvs::ValueArray<PixelType>;
    using Buffers = std::array<Buffer,Direction::NumberOfDirections>;

    static std::string DirectionName( const Direction dir ) { return kvs::CubicImage::DirectionName( dir ); }
    static kvs::Vec3 DirectionVector( const Direction dir ) { return kvs::CubicImage::DirectionVector( dir ); }
    static kvs::Vec3 UpVector( const Direction dir ) { return kvs::CubicImage::UpVector( dir ); }

private:
    size_t m_width;
    size_t m_height;
    Buffers m_buffers;

public:
    SphericalBuffer( const size_t width, const size_t height ):
        m_width( width ),
        m_height( height )
    {
    }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    size_t stitchedWidth() const { return m_width * 4; }
    size_t stitchedHeight() const { return m_height * 3; }

    void setBuffer( const Direction dir, const Buffer& buffer ) { m_buffers[dir] = buffer; }

    template <size_t N> // N: number of channels
    Buffer stitch()
    {
        const auto w = this->width();
        const auto h = this->height();
        const auto stitched_width = this->stitchedWidth();
        const auto stitched_height = this->stitchedHeight();
        Buffer stitched_buffer( stitched_width * stitched_height * N );
        stitched_buffer.fill(0);
        for ( size_t j = 0; j < stitched_height; j++ )
        {
            const float v = 1.0f - (float)j / ( stitched_height - 1 );
            const float theta = v * kvs::Math::pi;
            for ( size_t i = 0; i < stitched_width; i++ )
            {
                const float u = (float)i / ( stitched_width - 1 );
                const float phi = u * 2.0f * kvs::Math::pi;

                const float x = std::sin( phi ) * std::sin( theta ) * -1.0f;
                const float y = std::cos( theta );
                const float z = std::cos( phi ) * std::sin( theta ) * -1.0f;

                const float a = kvs::Math::Max(
                    kvs::Math::Abs( x ), kvs::Math::Abs( y ), kvs::Math::Abs( z ) );
                const float xa = x / a;
                const float ya = y / a;
                const float za = z / a;

                std::array<PixelType,N> pixel;
                if ( xa == 1 )
                {
                    const float si = kvs::Math::Abs( ( ( za + 1.0f ) / 2.0f - 1.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( ya + 1.0f ) / 2.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Right], {si, sj} );
                }
                else if ( xa == -1 )
                {
                    const float si = kvs::Math::Abs( ( ( za + 1.0f ) / 2.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( ya + 1.0f ) / 2.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Left], {si, sj} );
                }
                else if ( ya == 1 )
                {
                    const float si = kvs::Math::Abs( ( ( xa + 1.0f ) / 2.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( za + 1.0f ) / 2.0f - 1.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Bottom], {si, sj} );
                }
                else if ( ya == -1 )
                {
                    const float si = kvs::Math::Abs( ( ( xa + 1.0f ) / 2.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( za + 1.0f ) / 2.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Top], {si, sj} );
                }
                else if ( za == 1 )
                {
                    const float si = kvs::Math::Abs( ( ( xa + 1.0f ) / 2.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( ya + 1.0f ) / 2.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Front], {si, sj} );
                }
                else if ( za == -1 )
                {
                    const float si = kvs::Math::Abs( ( ( xa + 1.0f ) / 2.0f - 1.0f ) * ( w - 1 ) );
                    const float sj = kvs::Math::Abs( ( ( ya + 1.0f ) / 2.0f ) * ( h - 1 ) );
                    pixel = this->get_pixel<N>( m_buffers[Direction::Back], {si, sj} );
                }
                this->set_stitched_pixel<N>( pixel, kvs::Vec2ui(i, j), stitched_buffer );
            }
        }

        return stitched_buffer;
    }

private:
    template <size_t N>
    void set_stitched_pixel( const std::array<PixelType,N>& pixel, const kvs::Vec2ui& p, Buffer& stitched_buffer )
    {
        const size_t i = p[0];
        const size_t j = p[1];
        const auto width = this->stitchedWidth();
        for ( size_t offset = 0; offset < N; ++offset )
        {
            stitched_buffer[ N * ( j * width + i ) + offset ] = pixel[ offset ];
        }
    }

    template <size_t N>
    std::array<PixelType,N> get_pixel( const Buffer& buffer, const kvs::Vec2& p )
    {
        std::array<PixelType,N> pixel;
        for ( size_t offset = 0; offset < N; ++offset )
        {
            pixel[ offset ] = this->get_value<N>( buffer, p, offset );
        }
        return pixel;
    }

    template <size_t N>
    PixelType get_value( const Buffer& buffer, const kvs::Vec2& p, const size_t offset )
    {
        const float x = p.x();
        const float y = p.y();
        const size_t x0 = kvs::Math::Floor(x);
        const size_t y0 = kvs::Math::Floor(y);
        const size_t x1 = x0 + ( m_width - 1 > x0 ? 1 : 0 );
        const size_t y1 = y0 + ( m_height - 1 > y0 ? 1 : 0 );
        const float xratio = x - x0;
        const float yratio = y - y0;

        const auto p0 = buffer[ N * ( y0 * m_width + x0 ) + offset ];
        const auto p1 = buffer[ N * ( y0 * m_width + x1 ) + offset ];
        const auto p2 = buffer[ N * ( y1 * m_width + x0 ) + offset ];
        const auto p3 = buffer[ N * ( y1 * m_width + x1 ) + offset ];

        const auto zero = PixelType(0);
        const auto one = PixelType(1);
        const auto d = p0 * ( one - xratio ) + p2 * xratio;
        const auto e = p1 * ( one - xratio ) + p3 * xratio;
        return kvs::Math::Clamp( d * ( one - yratio ) + e * yratio, zero, one );
    }
};

template <>
template <size_t N>
inline kvs::UInt8 SphericalBuffer<kvs::UInt8>::get_value(
    const kvs::ValueArray<kvs::UInt8>& buffer,
    const kvs::Vec2& p,
    const size_t offset )
{
    const float x = p.x();
    const float y = p.y();
    const size_t x0 = kvs::Math::Floor(x);
    const size_t y0 = kvs::Math::Floor(y);
    const size_t x1 = x0 + ( m_width - 1 > x0 ? 1 : 0 );
    const size_t y1 = y0 + ( m_height - 1 > y0 ? 1 : 0 );
    const float xratio = x - x0;
    const float yratio = y - y0;

    const float p0 = static_cast<float>( buffer[ N * ( y0 * m_width + x0 ) + offset ] ) / 255.0f;
    const float p1 = static_cast<float>( buffer[ N * ( y0 * m_width + x1 ) + offset ] ) / 255.0f;
    const float p2 = static_cast<float>( buffer[ N * ( y1 * m_width + x0 ) + offset ] ) / 255.0f;
    const float p3 = static_cast<float>( buffer[ N * ( y1 * m_width + x1 ) + offset ] ) / 255.0f;

    const float d = p0 * ( 1.0f - xratio ) + p2 * xratio;
    const float e = p1 * ( 1.0f - xratio ) + p3 * xratio;
    const float v = kvs::Math::Clamp( d * ( 1.0f - yratio ) + e * yratio, 0.0f, 1.0f );
    return kvs::Math::Round( v * 255.0f );
}

} // end of namespace InSituVis
