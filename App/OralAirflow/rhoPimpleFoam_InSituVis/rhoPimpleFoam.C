/*---------------------------------------------------------------------------* \
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    rhoPimpleFoam

Description
    Transient solver for laminar or turbulent flow of compressible fluids
    for HVAC and similar applications.

    Uses the flexible PIMPLE (PISO-SIMPLE) solution for time-resolved and
    pseudo-transient simulations.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "psiThermo.H"
#include "turbulenceModel.H"
#include "bound.H"
#include "pimpleControl.H"
#include "fvIOoptionList.H"

// rhoPimpleFoam_InSituVis: Headers
// {
#include <cfenv>
#include <kvs/mpi/Communicator>
#include <kvs/mpi/Logger>
#include <kvs/mpi/ImageCompositor>
#include <kvs/Timer>
#include <kvs/String>
#include <kvs/OffScreen>
#include <kvs/ExternalFaces>
#include <kvs/PolygonRenderer>
#include <kvs/Png>
#include "../Util/CreateOutputDirectory.h"
#include "../Util/CreateUnstructuredVolumeObject.h"
// }

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"

    pimpleControl pimple(mesh);

    #include "createFields.H"
    #include "createFvOptions.H"
    #include "initContinuityErrs.H"

    // rhoPimpleFoam_InSituVis: Parameter settings
    // {
    Foam::messageStream::level = 0; // Disable Foam::Info
    kvs::mpi::Communicator world( MPI_COMM_WORLD );
    kvs::mpi::Logger logger( world );
    const kvs::Indent indent(4);
    const int root = 0;
    const int size = world.size();
    const int rank = world.rank();
    const size_t image_width = 512;
    const size_t image_height = 512;
    const bool depth_testing = true;
    const bool output_sim = false;
    const bool output_volume = false;
    const bool output_image = true;
    const bool output_sub_image = true;
    // }

    // rhoPimpleFoam_InSituVis: Create output directories
    // {
    const std::string output_base_dirname( "Output" );
    const auto output_dirname = Util::CreateOutputDirectory( world, output_base_dirname, "Proc" );
    if ( output_dirname.empty() )
    {
        logger( root ) << "ERROR: " << "Cannot create output directory." << std::endl;
        world.abort();
    }
    // }

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // rhoPimpleFoam_InSituVis: Output messages
    // {
    // Info<< "\nStarting time loop\n" << endl;
    const auto start_time = runTime.startTime().value();
    const auto start_time_index = runTime.startTimeIndex();
    const auto end_time = runTime.endTime().value();
    const auto end_time_index = static_cast<int>( end_time / runTime.deltaT().value() );
    logger( root ) << std::endl;
    logger( root ) << "STARTING TIME LOOP" << std::endl;
    logger( root ) << indent << "Start time and index: " << start_time << ", " << start_time_index << std::endl;
    logger( root ) << indent << "End time and index: " << end_time << ", " << end_time_index << std::endl;
    logger( root ) << std::endl;
    // }

    // rhoPimpleFoam_InSituVis: Setup image compositor
    // {
    kvs::mpi::ImageCompositor compositor( world );
    compositor.initialize( image_width, image_height, depth_testing );
    // }

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        // Info<< "Time = " << runTime.timeName() << nl << endl;
        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        logger( root ) << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        logger( root ) << indent << "T: " << current_time << std::endl;
        logger( root ) << indent << "End T: " << end_time << std::endl;
        logger( root ) << indent << "Delta T: " << runTime.deltaT().value() << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Start timer
        // {
        kvs::Timer timer;
        timer.start();
        // }

        if (pimple.nCorrPIMPLE() <= 1)
        {
            #include "rhoEqn.H"
        }

        // --- Pressure-velocity PIMPLE corrector loop
        while (pimple.loop())
        {
            #include "UEqn.H"
            #include "EEqn.H"

            // --- Pressure corrector loop
            while (pimple.correct())
            {
                #include "pEqn.H"
            }

            if (pimple.turbCorr())
            {
                turbulence->correct();
            }
        }

        if ( output_sim ) runTime.write();

        // rhoPimpleFoam_InSituVis: Stop timer
        // {
        timer.stop();
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto ts = timer.sec();
        const auto Ts = kvs::String::ToString( ts, 4 );
        logger( root ) << indent << "Processing Times:" << std::endl;
        logger( root ) << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;
        //logger( root ) << indent.nextIndent().nextIndent() << "Number of nodes: " << mesh.nPoints() << std::endl;
        //logger( root ) << indent.nextIndent().nextIndent() << "Number of cells: " << mesh.nCells() << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Import mesh and field
        // {
        timer.start();
        //auto* volume = Util::CreateUnstructuredVolumeObject( mesh, p ); // p: pressure
        auto* volume = Util::CreateUnstructuredVolumeObject( mesh, U ); // U: velocity (magnitude)
        auto min_coord = volume->minObjectCoord();
        auto max_coord = volume->maxObjectCoord();
        auto min_value = volume->minValue();
        auto max_value = volume->maxValue();
        world.allReduce( min_coord[0], min_coord[0], MPI_MIN );
        world.allReduce( min_coord[1], min_coord[1], MPI_MIN );
        world.allReduce( min_coord[2], min_coord[2], MPI_MIN );
        world.allReduce( max_coord[0], max_coord[0], MPI_MAX );
        world.allReduce( max_coord[1], max_coord[1], MPI_MAX );
        world.allReduce( max_coord[2], max_coord[2], MPI_MAX );
        world.allReduce( min_value, min_value, MPI_MIN );
        world.allReduce( max_value, max_value, MPI_MAX );
        volume->setMinMaxObjectCoords( min_coord, max_coord );
        volume->setMinMaxExternalCoords( min_coord, max_coord );
        volume->setMinMaxValues( min_value, max_value );
        timer.stop();
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto ti = timer.sec();
        const auto Ti = kvs::String::ToString( ti, 4 );
        logger( root ) << indent.nextIndent() << "Import: " << Ti << " s" << std::endl;
        //volume->print( logger( root ), indent.nextIndent().nextIndent() );
        // }

        // rhoPimpleFoam_InSituVis: Output KVSML
        timer.start();
        std::ostringstream output_index; output_index << std::setw(5) << std::setfill('0') << current_time_index;
        const std::string output_basename("output");
        //const std::string output_filename = output_basename + "_" + current_time + ".kvsml";
        const std::string output_filename = output_basename + "_" + output_index.str() + ".kvsml";
        if ( output_volume ) volume->write( output_dirname + output_filename, false );
        timer.stop();

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto to = timer.sec();
        const auto To = kvs::String::ToString( to, 4 );
        logger( root ) << indent.nextIndent() << "Write: " << To << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Rendering volume
        // {
        fenv_t fe;
        std::feholdexcept( &fe );
        timer.start();
        auto* object = new kvs::ExternalFaces( volume );
        auto* renderer = new kvs::glsl::PolygonRenderer();
        delete volume;
        kvs::OffScreen screen;
        //screen.setBackgroundColor( kvs::RGBColor::Black() );
        screen.setSize( image_width, image_height );
        screen.registerObject( object, renderer );
        screen.draw();
        timer.stop();
        std::feupdateenv( &fe );
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto tv = timer.sec();
        const auto Tv = kvs::String::ToString( tv, 4 );
        logger( root ) << indent.nextIndent() << "Visualization: " << Tv << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Drawback and output rendering image
        // {
        timer.start();
        auto image = screen.capture();
        if ( output_sub_image ) image.write( output_dirname + kvs::File( output_filename ).baseName() + ".bmp" );
        timer.stop();
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto td = timer.sec();
        const auto Td = kvs::String::ToString( td, 4 );
        logger( root ) << indent.nextIndent() << "Drawback: " << Td << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Image composition
        // {
        timer.start();
        auto color_buffer = screen.readbackColorBuffer();
        auto depth_buffer = screen.readbackDepthBuffer();
        compositor.run( color_buffer, depth_buffer );
        timer.stop();
        // }

        // rhoPimpleFoam_InSituVis: Output composite image
        // {
        if ( output_image )
        {
            world.barrier();
            kvs::Png composed_image( image_width, image_height, color_buffer );
            composed_image.write( output_base_dirname + "/" + kvs::File( output_filename ).baseName() + ".png" );
        }
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto tt = ts + ti + to + tv + td;
        const auto Tt = kvs::String::ToString( tt, 4 );
        logger( root ) << indent.nextIndent() << "---" << std::endl;
        logger( root ) << indent.nextIndent() << "Total: " << Tt << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        // Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        //     << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        //     << nl << endl;
        const auto elapsed_time = runTime.elapsedCpuTime();
        logger( root ) << std::endl;
        logger( root ) << "Elapsed Time: " << elapsed_time << " s" << std::endl;
        logger( root ) << std::endl;
        // }
    }

    // rhoPimpleFoam_InSituVis: Destory image compositor
    // {
    compositor.destroy();
    // }

    //Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
