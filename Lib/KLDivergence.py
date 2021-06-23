import numpy as np
from scipy.stats import norm, entropy, gaussian_kde

def main( V_crr , V_prv ):
    KDE_crr = gaussian_kde( np.array( V_crr ) )
    KDE_prv = gaussian_kde( np.array( V_prv ) )

    P_crr = KDE_crr( np.linspace(3, 45) )
    P_prv = KDE_prv( np.linspace(3, 45) )

    D = entropy( P_crr, P_prv )

    if np.isinf( D ) or D != D:
        return 0.0
    else :
        return D
