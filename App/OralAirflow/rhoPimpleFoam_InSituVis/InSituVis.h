#pragma once
#include "../Util/InSituVis.h"
#include <kvs/OrthoSlice>
#include <kvs/PolygonRenderer>
#include <kvs/Bounds>


namespace local
{

class InSituVis : public Util::InSituVis
{
public:
    InSituVis( const MPI_Comm world = MPI_COMM_WORLD, const int root = 0 ):
        Util::InSituVis( world, root )
    {
        this->setSize( 1024, 1024 );
        this->setOutputDirectoryName( "Output", "Proc" );
        this->setOutputImageEnabled( true );
        this->setOutputSubImageEnabled( false );
        this->setOutputSubVolumeEnabled( false );
        this->setPipeline(
            [&] ( Util::InSituVis::Screen& screen, Util::InSituVis::Volume& volume )
            {
                // Create new slice objects.
                auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
                auto py = ( volume.minObjectCoord().y() + volume.maxObjectCoord().y() ) * 0.5f;
                auto ay = kvs::OrthoSlice::YAxis;
                auto* object_y = new kvs::OrthoSlice( &volume, py, ay, t );
                object_y->setName("ObjectY");

                auto pz = ( volume.minObjectCoord().z() + volume.maxObjectCoord().z() ) * 0.5f;
                auto az = kvs::OrthoSlice::ZAxis;
                auto* object_z = new kvs::OrthoSlice( &volume, pz, az, t );
                object_z->setName("ObjectZ");

                if ( screen.scene()->hasObject("ObjectY") )
                {
                    // Update the objects.
                    screen.scene()->replaceObject( "ObjectY", object_y );
                    screen.scene()->replaceObject( "ObjectZ", object_z );
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
};

} // end of namspace local
