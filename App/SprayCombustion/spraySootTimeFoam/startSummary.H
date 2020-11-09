Info<< "Total cylinder mass: " << fvc::domainIntegrate(rho).value() << endl;

OFstream logSummaryFile
(
    runTime.path()/("logSummary." + runTime.timeName() + ".dat")
);

logSummaryFile
    << "# CA" << "      p" << "        T" << "       u'" << endl;

scalar QALL=0.0; // accumulate dQ
