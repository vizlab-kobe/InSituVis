#pragma once
#include "../Util/InSituVis.h"
#include <kvs/OrthoSlice>
#include <kvs/PolygonRenderer>
#include <kvs/Bounds>


namespace local
{

class InSituVis : public Util::InSituVis
{
private:
    kvs::Real32 m_min_value;
    kvs::Real32 m_max_value;

public:
    InSituVis():
        m_min_value( 0.0f ),
        m_max_value( 0.0f )
    {
        this->setSize( 1024, 1024 );
        this->setOutputDirectoryName( "Output" );
        this->setOutputImageEnabled( true );
        this->setPipeline(
            [&] ( Util::InSituVis::Screen& screen, const Util::InSituVis::Volume& volume )
            {
                auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
                if ( !t.hasRange() )
                {
                    if ( kvs::Math::Equal( m_min_value, m_max_value ) )
                    {
                        t.setRange( volume.minValue(), volume.maxValue() );
                    }
                    else
                    {
                        t.setRange( m_min_value, m_max_value );
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

                if ( screen.scene()->hasObject( volume.name() + "ObjectY") )
                {
                    // Update the objects.
                    screen.scene()->replaceObject( volume.name() + "ObjectY", object_y );
                    screen.scene()->replaceObject( volume.name() + "ObjectZ", object_z );
                }
                else
                {
                    // Register the objects with renderer.
                    kvs::Light::SetModelTwoSide( true );
                    auto* renderer_y = new kvs::glsl::PolygonRenderer();
                    auto* renderer_z = new kvs::glsl::PolygonRenderer();
                    screen.registerObject( object_y, renderer_y );
                    screen.registerObject( object_z, renderer_z );
                    screen.registerObject( object_z, new kvs::Bounds() );
                }
            } );
    }

    void setMinMaxValues( const kvs::Real32 min_value, const kvs::Real32 max_value )
    {
        m_min_value = min_value;
        m_max_value = max_value;
    }
};

} // end of namspace local
