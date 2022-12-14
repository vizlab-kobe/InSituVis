/*****************************************************************************/
/**
 *  @file   Viewpoint.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/Vector3>
#include <kvs/Value>
#include <kvs/Math>
#include <functional>
#include <vector>
#include <kvs/Quaternion>


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Viewpoint class.
 */
/*===========================================================================*/
class Viewpoint
{
private:
    static size_t MaxIndex() { return kvs::Value<size_t>::Max(); }

public:
    enum Direction
    {
        Uni, ///< uni-direction
        Omni, ///< omni-direction
        Adaptive ///< adaptive selection
    };

    struct Location
    {
        size_t index{ MaxIndex() }; ///< location index
        Direction direction{ Direction::Uni }; ///< view direction type
        kvs::Vec3 position{ 0, 0, 12 }; ///< viewpoint position in world coordinate
        kvs::Vec3 up_vector{ 0, 1, 0 };
        kvs::Vec3 look_at{ 0, 0, 0 }; ///< look-at position in world coordinate
        kvs::Quaternion rotation{ 1, 0, 0, 0 };

        Location(
            const kvs::Vec3& p,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            position( p ),
            look_at( l ) {}

        Location(
            const size_t i,
            const kvs::Vec3& p,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            index( i ),
            position( p ),
            look_at( l ) {}

        Location(
            const Direction d,
            const kvs::Vec3& p,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            direction( d ),
            position( p ),
            look_at( l ) {}

        Location(
            const size_t i,
            const Direction d,
            const kvs::Vec3& p,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            index( i ),
            direction( d ),
            position( p ),
            look_at( l ) {}

        Location(
            const Direction d,
            const kvs::Vec3& p,
            const kvs::Vec3& u,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            direction( d ),
            position( p ),
            up_vector( u ),
            look_at( l ) {}

        Location(
            const size_t i,
            const Direction d,
            const kvs::Vec3& p,
            const kvs::Vec3& u,
            const kvs::Quaternion& q,
            const kvs::Vec3& l = { 0, 0, 0 } ):
            index( i ),
            direction( d ),
            position( p ),
            up_vector( u ),
            look_at( l ),
            rotation( q ) {}
    };

    using Locations = std::vector<Location>;

private:
    Locations m_locations{}; ///< set of viewpoint locations

public:
    Viewpoint() = default;
    Viewpoint( const Location& location ) { this->set( location ); }
    virtual ~Viewpoint() = default;

    const Locations& locations() const { return m_locations; }
    size_t numberOfLocations() const { return m_locations.size(); }
    const Location& at( const size_t index ) const { return m_locations.at( index ); }
    void clear() { m_location.clear(); m_locations.shrink_to_fit(); }
    void set( const Location& location ) { this->clear(); this->add( location ); }
    void add( const Location& location )
    {
        if ( location.index < MaxIndex() )
        {
            m_locations.push_back( location );
        }
        else
        {
            auto l = location;
            l.index = m_locations.size();
            m_locations.push_back( l );
        }
    }
};

} // end of namespace InSituVis
