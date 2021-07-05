/*****************************************************************************/
/**
 *  @file   Viewpoint.h
 *  @author Naohisa Sakamoto
 */
/*****************************************************************************/
#pragma once
#include <kvs/Vector3>
#include <functional>
#include <vector>


namespace InSituVis
{

/*===========================================================================*/
/**
 *  @brief  Viewpoint class.
 */
/*===========================================================================*/
class Viewpoint
{
public:
    enum Direction
    {
        Uni, ///< uni-direction
        Omni, ///< omni-direction
        Adaptive ///< adaptive selection
    };

    struct Location
    {
        Direction direction{ Direction::Uni }; ///< view direction type
        kvs::Vec3 position{ 0, 0, 12 }; ///< viewpoint position in world coordinate
        kvs::Vec3 look_at{ 0, 0, 0 }; ///< look-at position in world coordinate
        Location( const Direction d, const kvs::Vec3& p, const kvs::Vec3& l = { 0, 0, 0 } ):
            direction( d ), position( p ), look_at( l ) {}
        Location( const kvs::Vec3& p, const kvs::Vec3& l = { 0, 0, 0 } ):
            direction( Uni ), position( p ), look_at( l ) {}
    };

    using Locations = std::vector<Location>;

private:
    Locations m_locations{}; ///< set of viewpoint locations

public:
    Viewpoint() = default;
    virtual ~Viewpoint() = default;
    Viewpoint( const Location& location ) { this->set( location ); }

    const Locations& locations() const { return m_locations; }
    size_t numberOfLocations() const { return m_locations.size(); }
    const Location& at( const size_t index ) const { return m_locations.at( index ); }
    void set( const Location& location ) { this->clear(); this->add( location ); }
    void add( const Location& location ) { m_locations.push_back( location ); }
    void clear() { m_locations.shrink_to_fit(); }
};

} // end of namespace InSituVis
