#pragma once
#include <kvs/Vector3>
#include <functional>
#include <vector>


namespace InSituVis
{

class Viewpoint
{
public:
    enum DirType
    {
        SingleDir, ///< single direction
        OmniDir, ///< omni-direction
        AdaptiveDir ///< adaptive direction
    };

    struct Point
    {
        kvs::Vec3 position; ///< viewpoint position in world coordinate
        DirType dir_type; ///< view direction type
        Point( const kvs::Vec3& p, const DirType d ): position( p ), dir_type( d ) {}
    };

    using Points = std::vector<Point>;
    using Counter = std::function<size_t(void)>;
    using Locator = std::function<Point(const size_t index)>;

private:
    kvs::Vec3 m_look_at_point; ///< look-at point in world coordinate (ignored in the case of 'OmniDir')
    Points m_points; ///< set of viewpoints
    Counter m_counter; ///< counter for number of viewpoints
    Locator m_locator; ///< locator for viewpoint

public:
    Viewpoint(): m_look_at_point( 0, 0, 0 )
    {
    }

    Viewpoint( const kvs::Vec3& position, const DirType dir_type, const kvs::Vec3& lookat = { 0, 0, 0 } ):
        m_look_at_point( lookat )
    {
        this->setPoint( position, dir_type );
    }

    virtual ~Viewpoint() {}

    void setNumberOfPointsCounter( Counter counter )
    {
        m_counter = counter;
    }

    void setPointLocator( Locator locator )
    {
        m_locator = locator;
    }

    size_t numberOfPoints() const
    {
        return m_points.size() == 0 ? m_counter() : m_points.size();
    }

    Point point( const size_t index ) const
    {
        return m_points.size() == 0 ? m_locator( index ) : m_points[index];
    }

    const Points& points() const
    {
        return m_points;
    }

    const kvs::Vec3& lookAtPoint() const
    {
        return m_look_at_point;
    }

    void setLookAtPoint( const kvs::Vec3& point )
    {
        m_look_at_point = point;
    }

    void setPoint( const kvs::Vec3& position, const DirType dir_type )
    {
        this->clearPoints();
        this->addPoint( position, dir_type );
    }

    void addPoint( const kvs::Vec3& position, const DirType dir_type )
    {
        m_points.push_back( Point( position, dir_type ) );
    }

    void clearPoints()
    {
        m_points.shrink_to_fit();
    }

protected:
    size_t counter() const { return m_counter(); }
    Point locator( const size_t index ) const { return m_locator( index ); }
};

} // end of namespace InSituVis
