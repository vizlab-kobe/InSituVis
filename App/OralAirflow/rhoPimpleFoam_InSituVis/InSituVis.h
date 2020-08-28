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
                // Create a new object
                auto p = ( volume.minObjectCoord().z() + volume.maxObjectCoord().z() ) * 0.5f;
                auto a = kvs::OrthoSlice::ZAxis;
                auto t = kvs::TransferFunction( kvs::ColorMap::CoolWarm() );
                auto* object = new kvs::OrthoSlice( &volume, p, a, t );
                object->setName("Object");

                if ( screen.scene()->hasObject("Object") )
                {
                    // Update the object.
                    screen.scene()->replaceObject( "Object", object );
                }
                else
                {
                    // Register the object with renderer.
                    auto* renderer = new kvs::glsl::PolygonRenderer();
                    renderer->disableShading();
                    screen.registerObject( object, renderer );
                    screen.registerObject( object, new kvs::Bounds() );
                }
            } );
    }
};

} // end of namspace local
