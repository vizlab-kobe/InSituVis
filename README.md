# InSituVis
InSituVis provides an in-situ visualization framework that enables easy and effective connection of numerical simulation solvers and visualization systems in a high-performance computing environment.

## Pre-requisities

The follwoing packages needs to be installed in order to use parallel visualization functionalities.
- [KVS](https://github.com/naohisas/KVS)
- OSMesa
- MPI

### KVS
KVS supports OSMesa and MPI needs to be installed.

1. Install OSMesa and MPI

    OSMesa and MPI need to be install before compile the KVS. Please refer to the followin URLs to install them.<br>
    - [OSMesa](https://github.com/naohisas/KVS/blob/develop/Source/SupportOSMesa/README.md)
    - [MPI](https://github.com/naohisas/KVS/blob/develop/Source/SupportMPI/README.md)

2. Get KVS source codes from the GitHub repository as follows:
    ```
    $ git clone https://github.com/naohisas/KVS.git
    ```

3. Modify kvs.conf as follows:
    ```
    $ cd KVS
    $ <modify kvs.conf>
    ```

    - Change the following items from 0(disable) to 1(enable).<br>
    ```
    KVS_ENABLE_OPENGL     = 1
    KVS_SUPPORT_MPI       = 1
    KVS_SUPPORT_OSMESA    = 1
    ```
    - Change the following items from 1(enable) to 0(disable).<br>
    ```
    KVS_SUPPORT_GLU       = 0
    KVS_SUPPORT_GLUT      = 0
    ```
    - Change the following items to 1 if needed. <br>
    ```
    KVS_ENABLE_OPENMP     = 1
    ```

4. Compile and install the KVS
    ```
    $ make
    $ make install
    ```

## Adaptor framework
InSituVis provides several "Adaptor" classes to connect simulation and visualization codes. A simple example of integration using ```InSituVis::Adaptor``` class is shown bellow.

1. Include some header files.
   ```cpp
   #include <InSituVis/Lib/Adaptor.h>   // for InSituVis::Adaptor class
   #include <InSituVis/Lib/Viewpoint.h> // for InSituVis::Viewpoint class
   ```
    - Set the include path to the directory where the InSituVis repository is cloned (downloaded). Also, the InSituVis is a header-only library and does not need to be compiled in advance.

2. Set input visualization parameters.
   ```cpp
   namespace Params
   {
   const auto ImageSize = kvs::Vec2ui{ 512, 512 }; // width x height
   const auto AnalysisInterval = 100; // analysis (visuaization) time interval
   const auto ViewPos = kvs::Vec3{ 7, 5, 6 }; // viewpoint position in world coordinate system
   const auto ViewDir = InSituVis::Viewpoint::Direction::Uni; // Uni or Omni
   const auto Viewpoint = InSituVis::Viewpoint{ { ViewDir, ViewPos } }; // viewpoint
   }
   ```
3. Set in-situ visualization adaptor.
   ```cpp
   InSituVis::Adaptor adaptor;
   adaptor.setImageSize( Params::ImageSize().x(), Params::ImageSize().y() );
   adaptor.setViewpoint( Params::Viewpoint );
   adaptor.setAnalysisInterval( Params::AnalysisInterval );
   ```

4. Create visualization pipeline using KVS modules. The following is an example of isosurface extraction.
    ```cpp
    // Isosurface extraction
    //    Scree:  using Screen = kvs::Screen
    //    Object: using Object = kvs::ObjectBase
    //    Volume: using Volume = kvs::StructuredVolumeObject
    auto isosurface = [] ( Screen& screen, const Object& object )
    {
        Volume volume; volume.shallowCopy( Volume::DownCast( object ) );

        // Setup a transfer function.
        const auto min_value = volume.minValue();
        const auto max_value = volume.maxValue();
        auto t = kvs::TransferFunction( cmap );
        t.setRange( min_value, max_value );

        // Create new object
        auto n = kvs::Isosurface::VertexNormal;
        auto d = true;
        auto i = kvs::Math::Mix( min_value, max_value, 0.5 );
        auto* o = new kvs::Isosurface( &volume, i, n, d, t );
        o->setName( "Isosurface" );

        // Register object and renderer to screen
        kvs::Light::SetModelTwoSide( true );
        if ( screen.scene()->hasObject( "Isosurface" ) )
        {
            // Update the objects.
            screen.scene()->replaceObject( "Isosurface", o );
        }
        else
        {
            // Bounding box.
            screen.registerObject( o, new kvs::Bounds() );

            // Register the objects with renderer.
            auto* r = new kvs::glsl::PolygonRenderer();
            r->setTwoSideLightingEnabled( true );
            screen.registerObject( o, r );
        }
   };
   ```
   
5. Set visualization data, which is called as volume object in KVS.
   ```cpp
   // Get simulation data, a grid resolution {dimx, dimy, dimz} and a value array {values},
   // from the solver by using get functions; GET_XXX().
   int dimx = GET_DIMX();
   int dimy = GET_DIMY();
   int dimz = GET_DIMZ();
   double* values = GET_VALUES();

   // Create visualization data (kvs::StructuredVolumeObject).
   Volume volume;
   volume.setVeclen( 1 );
   volume.setResolution( kvs::Vec3ui( dimx, dimy, dimz ) );
   volume.setValues( kvs::ValueArray<double>{ values, size_t( dimx * dimy * dimz ) } );
   volume.setGridTypeToUniform();
   volume.updateMinMaxValues();
   ```

6. Put the visualization data and execute the visualization pipeline.
   ```cpp
   adaptor.put( volume );
   adaptor.exec( {time_value, time_index} );
   ```
