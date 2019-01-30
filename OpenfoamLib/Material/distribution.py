def main( new_array , old_array ):
    import numpy as np
    from scipy.stats import norm, entropy, gaussian_kde

    value1 = np.array( new_array )
    value2 = np.array( old_array )

    kde1 = gaussian_kde(value1)
    kde2 = gaussian_kde(value2)
    
    #y1 = kde1(np.linspace(3, 45))
    y1 = kde1(np.linspace(1, 50))
    #y2 = kde2(np.linspace(3, 45))
    y2 = kde2(np.linspace(1, 50))
 
    en = entropy( y1, y2 )
    
    if np.isinf( en ) or en != en:
        return 0.0
    else :
        return en
    
