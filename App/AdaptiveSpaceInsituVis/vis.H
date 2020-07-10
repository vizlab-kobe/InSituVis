#include <PBVR_u.h>
if ( config::multicamera == 0 || initialmulti == 1)
{
    //  PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time - noutperKL + i * step + 2, 15, 50, "./constant/triSurface/realistic-cfd3.stl",cameraposx,cameraposy,cameraposz,repetitions );
    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        cameraposx,
        cameraposy,
        cameraposz,
        config::repetitions );
}
else //multicamera == 1 && initialmulti == 0
{
    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        maxcamerapos,
        maxcamerapos,
        maxcamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        maxcamerapos,
        maxcamerapos,
        mincamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        maxcamerapos,
        mincamerapos,
        maxcamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        maxcamerapos,
        mincamerapos,
        mincamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        mincamerapos,
        maxcamerapos,
        maxcamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        mincamerapos,
        maxcamerapos,
        mincamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        mincamerapos,
        mincamerapos,
        maxcamerapos,
        config::repetitions );

    PBVR_u(
        data_set[i],
        mesh.nCells(),
        mesh.nPoints(),
        pointCoords,
        cellCoords,
        cellPoints,
        now_time + (i + 1 - config::noutperKL) * config::step,
        15,
        50,
        "./constant/triSurface/realistic-cfd3.stl",
        mincamerapos,
        mincamerapos,
        mincamerapos,
        config::repetitions );
}
