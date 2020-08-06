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
#include "../Util/Importer.h"
#include "../Util/OutputDirectory.h"
#include "Visualization.h"
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
    Util::OutputDirectory output_dir( "Output", "Proc_" );
    if ( !output_dir.create( world ) )
    {
        logger( root ) << "ERROR: " << "Cannot create output directory." << std::endl;
        world.abort();
    }
    const auto output_base_dirname = output_dir.baseDirectoryName();
    const auto output_dirname = output_dir.name();
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

        // rhoPimpleFoam_InSituVis: Output loop information
        // {
        // Info<< "Time = " << runTime.timeName() << nl << endl;
        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        logger( root ) << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        logger( root ) << indent << "T: " << current_time << std::endl;
        logger( root ) << indent << "End T: " << end_time << std::endl;
        logger( root ) << indent << "Delta T: " << runTime.deltaT().value() << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Start simulation
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

        // rhoPimpleFoam_InSituVis: End simulation
        // {
        timer.stop();
        const auto ts = timer.sec();
        const auto Ts = kvs::String::From( ts, 4 );
        logger( root ) << indent << "Processing Times:" << std::endl;
        logger( root ) << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Import mesh and field
        // {
        timer.start();
        auto* volume = new Util::Importer( world, mesh, U );
        timer.stop();
        const auto ti = timer.sec();
        const auto Ti = kvs::String::From( ti, 4 );
        logger( root ) << indent.nextIndent() << "Import: " << Ti << " s" << std::endl;
        //volume->print( logger( root ), indent.nextIndent().nextIndent() );
        // }

        // rhoPimpleFoam_InSituVis: Write volume data
        timer.start();
        //const std::string output_number = current_time;
        const std::string output_number = kvs::String::From( current_time_index, 5, '0' );
        const std::string output_basename("output");
        const std::string output_filename = output_basename + "_" + output_number + ".kvsml";
        if ( output_volume ) volume->write( output_dirname + output_filename, false );
        timer.stop();
        const auto tv = timer.sec();
        const auto Tv = kvs::String::From( tv, 4 );
        logger( root ) << indent.nextIndent() << "Write volume: " << Tv << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Render volume data
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
        const auto tr = timer.sec();
        const auto Tr = kvs::String::From( tr, 4 );
        logger( root ) << indent.nextIndent() << "Visualization: " << Tr << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Write rendering image
        // {
        timer.start();
        if ( output_sub_image )
        {
            auto image = screen.capture();
            image.write( output_dirname + kvs::File( output_filename ).baseName() + ".bmp" );
        }
        timer.stop();
        const auto tw = timer.sec();
        const auto Tw = kvs::String::From( tw, 4 );
        logger( root ) << indent.nextIndent() << "Write sub-image: " << Tw << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Image composition
        // {
        timer.start();
        auto color_buffer = screen.readbackColorBuffer();
        auto depth_buffer = screen.readbackDepthBuffer();
        compositor.run( color_buffer, depth_buffer );
        if ( output_image )
        {
            world.barrier();
            kvs::Png composed_image( image_width, image_height, color_buffer );
            composed_image.write( output_base_dirname + "/" + kvs::File( output_filename ).baseName() + ".png" );
        }
        timer.stop();
        const auto tc = timer.sec();
        const auto Tc = kvs::String::From( tc, 4 );
        logger( root ) << indent.nextIndent() << "Composition: " << Tc << " s" << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        const auto tt = ts + ti + tv + tr + tw + tc;
        const auto Tt = kvs::String::From( tt, 4 );
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
