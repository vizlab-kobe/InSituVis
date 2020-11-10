    fvVectorMatrix UEqn
    (
        fvm::ddt(rho, U)
      + fvm::div(phi, U)
      + turbulence->divDevRhoReff(U)
     ==
 //       rho.dimensionedInternalField()*g
       parcels.SU(U)
      + fvOptions(rho, U)
    );


    UEqn.relax();

    fvOptions.constrain(UEqn);

    if (pimple.momentumPredictor())
    {
    /*	if(Debug)
    	{
    	Info<<"momentumPredictor"<<endl;
    	Info<<"U T p K dpdt mu"<<endl;
    	  forAll(mesh.cells(),cid)
	       {
    	        	Info<<U[cid]<<" "<<T[cid]<<" "<<p[cid]<<" "<<K[cid]<<tab<<dpdt[cid]<<tab<<thermo.mu()[cid] <<endl;
  	       }

    	}
    	*/
        solve(UEqn == -fvc::grad(p));
/*
     	if(Debug)
        	{
        	Info<<"momentumPredictor: after solve"<<endl;
        	  forAll(mesh.cells(),cid)
    	       {
        	        	Info<<U[cid]<<" "<<T[cid]<<" "<<p[cid]<<" "<<K[cid]<<tab<<dpdt[cid]<<tab <<thermo.mu()[cid]<<endl;
      	       }
        	}*/


        fvOptions.correct(U);
        K = 0.5*magSqr(U);
/*    	if(Debug)
        	{
        	Info<<"momentumPredictor: after estimate K"<<endl;
        	  forAll(mesh.cells(),cid)
    	       {
        	        	Info<<U[cid]<<" "<<T[cid]<<" "<<p[cid]<<" "<<K[cid]<<tab<<dpdt[cid]<<tab<<thermo.mu()[cid] <<endl;
      	       }
        	}*/
    }
