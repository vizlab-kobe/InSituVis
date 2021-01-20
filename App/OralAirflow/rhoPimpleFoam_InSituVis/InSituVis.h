#pragma once
#include <kvs/OrthoSlice>
#include <kvs/Isosurface>
#include <kvs/PolygonObject>
#include <kvs/PolygonRenderer>
#include <kvs/PolygonImporter>
#include <kvs/Bounds>
#include <InSituVis/Lib/Adaptor.h>
#include <InSituVis/Lib/Viewpoint.h>
#include <InSituVis/Lib/DistributedViewpoint.h>


namespace local
{

class InSituVis : public ::InSituVis::mpi::Adaptor
{
    using BaseClass = ::InSituVis::mpi::Adaptor;
    using Viewpoint = ::InSituVis::DistributedViewpoint;
    using Volume = BaseClass::Volume;
    using Polygon = kvs::PolygonObject;
    using Screen = BaseClass::Screen;

public:
    static Pipeline OrthoSlicePipeline( const InSituVis& adaptor )
    {
        return [&] ( Screen& screen, const Volume& volume )
        {
            auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
            if ( !t.hasRange() )
            {
                const auto min_value = adaptor.minValue();
                const auto max_value = adaptor.maxValue();
                if ( kvs::Math::Equal( min_value, max_value ) )
                {
                    t.setRange( volume.minValue(), volume.maxValue() );
                }
                else
                {
                    t.setRange( min_value, max_value );
                }
            }

            // Create new slice objects.
            auto py = ( volume.minObjectCoord().y() + volume.maxObjectCoord().y() ) * 0.5f;
            auto ay = kvs::OrthoSlice::YAxis;
            auto* object_y = new kvs::OrthoSlice( &volume, py, ay, t );
            object_y->setName( volume.name() + "ObjectY");

            auto pz = ( volume.minObjectCoord().z() + volume.maxObjectCoord().z() ) * 0.5f;
            auto az = kvs::OrthoSlice::ZAxis;
            auto* object_z = new kvs::OrthoSlice( &volume, pz, az, t );
            object_z->setName( volume.name() + "ObjectZ");

            // Create new renderers.
            auto* renderer_y = new kvs::glsl::PolygonRenderer();
            renderer_y->setName( volume.name() + "RendererY");

            auto* renderer_z = new kvs::glsl::PolygonRenderer();
            renderer_z->setName( volume.name() + "RendererZ");

            kvs::Light::SetModelTwoSide( true );
            if ( screen.scene()->hasObject( volume.name() + "ObjectY") )
            {
                // Update the objects.
                screen.scene()->replaceObject( volume.name() + "ObjectY", object_y );
                screen.scene()->replaceObject( volume.name() + "ObjectZ", object_z );
                screen.scene()->replaceRenderer( volume.name() + "RendererY", renderer_y );
                screen.scene()->replaceRenderer( volume.name() + "RendererZ", renderer_z );
            }
            else
            {
                // Register the objects with renderer.
                screen.registerObject( object_y, renderer_y );
                screen.registerObject( object_z, renderer_z );
            }
        };
    }

    static Pipeline IsosurfacePipeline( const InSituVis& adaptor )
    {
        return [&] ( Screen& screen, const Volume& volume )
        {
            const auto min_value = adaptor.minValue();
            const auto max_value = adaptor.maxValue();

            auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
            if ( !t.hasRange() )
            {
                if ( kvs::Math::Equal( min_value, max_value ) )
                {
                    t.setRange( volume.minValue(), volume.maxValue() );
                }
                else
                {
                    t.setRange( min_value, max_value );
                }
            }

            // Create new object
            auto i = ( min_value + max_value ) * 0.5f;
            auto n = kvs::Isosurface::PolygonNormal;
            auto d = true;
            auto* object = new kvs::Isosurface( &volume, i, n, d, t );
            object->setName( volume.name() + "Object");

            // Create new renderer
            auto* renderer = new kvs::glsl::PolygonRenderer();
            renderer->setName( volume.name() + "Renderer");

            // Register object and renderer to screen
            kvs::Light::SetModelTwoSide( true );
            if ( screen.scene()->hasObject( volume.name() + "Object") )
            {
                // Update the objects.
                screen.scene()->replaceObject( volume.name() + "Object", object );
                screen.scene()->replaceRenderer( volume.name() + "Renderer", renderer );
            }
            else
            {
                // Register the objects with renderer.
                screen.registerObject( object, renderer );
            }
        };
    }

private:
    Viewpoint m_viewpoint;
    kvs::Real32 m_min_value;
    kvs::Real32 m_max_value;
    Polygon m_boundary_mesh;

public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        BaseClass( world, root ),
//        m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::OmniDir ),
//        m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::SingleDir ),
//        m_viewpoint( {3,3,3}, Viewpoint::CubicDist, Viewpoint::AdaptiveDir ),
        m_viewpoint( {3,3,3}, Viewpoint::SphericalDist, Viewpoint::SingleDir ),
        m_min_value( 0.0f ),
        m_max_value( 0.0f )
    {
        m_viewpoint.generate();

        this->setImageSize( 1024, 1024 );
        this->setOutputImageEnabled( true );
        this->setOutputSubImageEnabled( false );
        this->setTimeInterval( 5 );
        this->setViewpoint( m_viewpoint );
        this->setPipeline( OrthoSlicePipeline( *this ) );
//        this->setPipeline( IsosurfacePipeline( *this ) );
    }

    void exec( const kvs::UInt32 time_index )
    {
        if ( !BaseClass::screen().scene()->hasObject( "BoundaryMesh") )
        {
            if ( m_boundary_mesh.numberOfVertices() > 0 )
            {
                // Register the bounding box at the root rank.
                auto* object = new Polygon();
                object->shallowCopy( m_boundary_mesh );
                object->setName( "BoundaryMesh" );
                BaseClass::screen().registerObject( object, new kvs::Bounds() );
            }
        }

        BaseClass::exec( time_index );
    }

    void importBoundaryMesh( const std::string& filename )
    {
        if ( BaseClass::world().rank() == BaseClass::world().root () )
        {
            m_boundary_mesh = kvs::PolygonImporter( filename );
        }
    }

    void setMinMaxValues( const kvs::Real32 min_value, const kvs::Real32 max_value )
    {
        m_min_value = min_value;
        m_max_value = max_value;
    }

    kvs::Real32 minValue() const { return m_min_value; }
    kvs::Real32 maxValue() const { return m_max_value; }
};

} // end of namspace local
